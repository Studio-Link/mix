import api from './api'
import { State } from './ws/state'
import { ref, watch } from 'vue'
import adapter from 'webrtc-adapter'
import { Error } from './error'
import { useEventListener, useStorage } from '@vueuse/core'
import { Avdummy } from './avdummy'

// --- Private Webrtc API ---
let pc: RTCPeerConnection | null = null
let audiostream: MediaStream | null = null
let videostream: MediaStream | null = null
let screenstream: MediaStream | null = null
let sdp_media_0 = 0
let sdp_media_1 = 0
let stats_timer: ReturnType<typeof setTimeout> | null = null

/* getDisplayMedia() has to be started from inside the click handler, so the
 * resulting promise is parked here until change_video() picks it up. */
let screen_request: Promise<MediaStream | null> | null = null

/* change_video() can be triggered from several watchers at once. Overlapping
 * getUserMedia() calls on the same device fail with NotReadableError, so the
 * calls are serialized through this promise chain. */
let video_queue: Promise<MediaStream | null> = Promise.resolve(null)

const sleep = (delay: number) => new Promise((resolve) => setTimeout(resolve, delay))

const MEDIA_RETRIES = 5

const resolutions: Record<string, { width: number; height: number; bandwidth: number }> = {
    '720p': { width: 1280, height: 720, bandwidth: 2000 },
    '360p': { width: 640, height: 360, bandwidth: 1000 },
    '360pl': { width: 640, height: 360, bandwidth: 400 },
}

const configuration: RTCConfiguration = {
    bundlePolicy: 'balanced',

    iceTransportPolicy: 'relay',
    iceServers: [
        {
            urls: 'turn:167.235.37.175:3478',
            username: 'turn200301',
            credential: 'choh4zeem3foh1',
        },
    ],


    /* default on Firefox/Chrome but needed by Safari */
    rtcpMuxPolicy: 'require'
}


type MediaKind = 'Microphone' | 'Camera' | 'Screen'

/* getUserMedia() fails for a lot of reasons that are not "permission denied".
 * Telling a user their permission was denied when their microphone is simply
 * busy in another application sends them to the wrong settings page. */
function media_error_message(kind: MediaKind, e: unknown): string {
    /* Note: Firefox's OverconstrainedError is not a DOMException instance, so
     * discriminate on .name rather than with instanceof. */
    const name = (e as DOMException)?.name ?? ''
    const device = kind.toLowerCase()

    switch (name) {
        case 'NotAllowedError':
        case 'PermissionDeniedError':
            return `${kind} access was blocked. Allow it in your browser's site settings and try again.`
        case 'NotFoundError':
        case 'DevicesNotFoundError':
            return `No ${device} found. Please connect one and try again.`
        case 'NotReadableError':
        case 'TrackStartError':
            return `Your ${device} is in use by another application. Close it and try again.`
        case 'OverconstrainedError':
        case 'ConstraintNotSatisfiedError':
            return `The selected ${device} is no longer available, falling back to the default device.`
        case 'SecurityError':
            return `${kind} access requires a secure (https) connection.`
        case 'AbortError':
            return `Your ${device} could not be started. Please try again.`
        case 'InvalidAccessError':
            return `${kind} sharing has to be started directly from a button click.`
        default:
            return `${kind} error: ${(e as Error)?.message || String(e)}`
    }
}

/* navigator.mediaDevices is undefined on insecure origins, which otherwise
 * surfaces as an opaque "Cannot read properties of undefined" TypeError. */
function media_supported(): boolean {
    /* lib.dom types these as always present, they are not on http origins */
    if ((navigator as any).mediaDevices?.getUserMedia)
        return true

    Error.error(
        window.isSecureContext === false
            ? 'Microphone and camera access requires a secure (https) connection.'
            : 'This browser does not support microphone and camera access.'
    )
    return false
}

export function has_display_media(): boolean {
    return !!(navigator as any).mediaDevices?.getDisplayMedia
}

function stop_stream(stream: MediaStream | null): null {
    /* getTracks() so an empty track list cannot throw and no track is leaked */
    stream?.getTracks().forEach((track) => track.stop())

    return null
}

function drop_device_id(constraints: MediaStreamConstraints): boolean {
    const track = (typeof constraints.video === 'object' ? constraints.video : null)
        ?? (typeof constraints.audio === 'object' ? constraints.audio : null)

    if (!track || track.deviceId === undefined)
        return false

    delete track.deviceId
    return true
}

async function get_media(constraints: MediaStreamConstraints, kind: MediaKind): Promise<MediaStream | null> {
    if (!media_supported())
        return null

    for (let i = 0; i < MEDIA_RETRIES; i++) {
        try {
            return await navigator.mediaDevices.getUserMedia(constraints)
        } catch (e) {
            const name = (e as DOMException)?.name

            /* Sometimes the device is not ready yet, since the previous stream is
             * not stopped synchronously, so retry after timeout */
            if (name === 'NotReadableError' && i < MEDIA_RETRIES - 1) {
                await sleep(50 * (i || 1))
                continue
            }

            /* Stored device ids go stale (Safari rotates them every session,
             * Chrome after site data is cleared) - retry on the default device */
            if (name === 'OverconstrainedError' && drop_device_id(constraints)) {
                console.warn(`get_media/${kind}: stored device gone, using default`)
                continue
            }

            console.error(`get_media/${kind}:`, e)
            throw e
        }
    }

    return null
}

function audio_constraints(deviceId: string | undefined, echoCancellation: boolean): MediaStreamConstraints {
    const audio: MediaTrackConstraints = {
        echoCancellation, // disabling audio processing
        autoGainControl: false,
        noiseSuppression: false,
        sampleRate: 48000,
        /* not in lib.dom's MediaTrackConstraints, honoured by Chrome */
        ...({ latency: 0.02 } as any), //20ms
    }

    /* 'ideal' rather than 'exact': a stale id degrades to the default device
     * instead of rejecting with OverconstrainedError */
    if (deviceId)
        audio.deviceId = { ideal: deviceId }

    return { audio, video: false }
}

function video_constraints(deviceId: string | undefined, width: number, height: number): MediaStreamConstraints {
    const video: MediaTrackConstraints = {
        width: { ideal: width },
        height: { ideal: height },
    }

    if (deviceId)
        video.deviceId = { ideal: deviceId }

    return { audio: false, video }
}


/* Limits bandwidth in in [kbps] */
async function updateBandwidthRestriction(bandwidth: number) {

    if ((adapter.browserDetails.browser === 'chrome' ||
        adapter.browserDetails.browser === 'safari' ||
        (adapter.browserDetails.browser === 'firefox' &&
            adapter.browserDetails.version! >= 64)) &&
        'RTCRtpSender' in window &&
        'setParameters' in window.RTCRtpSender.prototype) {

        const sender = pc?.getSenders().find((s) => s.track?.kind === 'video')
        if (!sender)
            return

        const parameters = sender.getParameters();
        if (!parameters.encodings) {
            parameters.encodings = [{}];
        }
        if (bandwidth === 0) {
            delete parameters.encodings[0].maxBitrate;
        } else {
            parameters.encodings[0].maxBitrate = bandwidth * 1000;
        }
        await sender.setParameters(parameters)
        return;
    }
}

function handle_answer(descr: any) {
    if (!descr) return

    console.log("remote description: type='%s'", descr.type)

    pc?.setRemoteDescription(descr).then(
        () => {
            console.log('set remote description -- success')
            Webrtc.state.value = WebrtcState.Listening
        },
        function (error) {
            console.warn('setRemoteDescription: %s', error.toString())
        }
    )
}

function pc_offer() {
    const offerOptions = {
        iceRestart: false,
    }
    pc?.createOffer(offerOptions)
        .then(function (desc) {
            console.log('got local description: %s', desc.type)

            pc?.setLocalDescription(desc).then(
                () => { },
                function (error) {
                    console.log('setLocalDescription: %s', error.toString())
                }
            )
        })
        .catch(function (error) {
            console.log('Failed to create session description: %s', error.toString())
        })
}


const QualityLimitationReasons = {
    none: 0,
    bandwidth: 1,
    cpu: 2,
    other: 3,
};

async function pc_stats(pc: RTCPeerConnection | null) {

    if (!pc || pc.connectionState === 'closed')
        return

    const state = pc.connectionState

    const stats = await pc.getStats()

    // @ts-ignore
    const values = [...stats.values()].filter(
        (v) =>
            ["peer-connection", "inbound-rtp", "outbound-rtp", "remote-inbound-rtp"].indexOf(v.type) !== -1,
    );

    let data = "";
    const sentTypes = new Set();


    values.forEach((value: any) => {
        const type = value.type.replace(/-/g, "_");
        const labels: any = [];
        const metrics: any = [];

        if (value.type === "peer-connection") {
            labels.push(`state="${state}"`);
            metrics.push(["browser_version", adapter.browserDetails.version]);
        }

        labels.push(`browser="${adapter.browserDetails.browser}"`);

        Object.entries(value).forEach(([key, v]: any) => {
            if (typeof v === "number") {
                metrics.push([key, v]);
            } else if (typeof v === "object") {
                Object.entries(v).forEach(([subkey, subv]) => {
                    if (typeof subv === "number") {
                        metrics.push([`${key}_${subkey}`, subv]);
                    }
                });
            } else if (
                key === "qualityLimitationReason" &&
                QualityLimitationReasons[v as keyof typeof QualityLimitationReasons] !== undefined
            ) {
                metrics.push([key, QualityLimitationReasons[v as keyof typeof QualityLimitationReasons]]);
            } else {
                labels.push(`${key}="${v}"`);
            }
        });

        metrics.forEach(([key, v]: any) => {
            const name = `${type}_${key.replace(/-/g, "_")}`;
            let typeDesc = "";

            if (!sentTypes.has(name)) {
                typeDesc = `# TYPE ${name} gauge\n`;
                sentTypes.add(name);
            }
            data += `${typeDesc}${name}{${labels.join(",")}} ${v}\n`;
        });
    });

    await api.rtc_stats(data)

    /* keep the handle so hangup() can cancel the chain */
    stats_timer = setTimeout(pc_stats, 5000, pc)
}

function pc_stats_start(pc: RTCPeerConnection | null) {
    /* ontrack fires per track - only ever run one stats chain */
    if (stats_timer !== null)
        return
    stats_timer = setTimeout(pc_stats, 0, pc)
}

function pc_stats_stop() {
    if (stats_timer === null)
        return
    clearTimeout(stats_timer)
    stats_timer = null
}


async function pc_setup() {
    console.log('browser: ', adapter.browserDetails.browser, adapter.browserDetails.version)
    pc = new RTCPeerConnection(configuration)
    sdp_media_0 = 0
    sdp_media_1 = 0

    pc.onicecandidate = async (event) => {
        console.log('webrtc/icecandidate: ' + event.candidate?.type + ' IP: ' + event.candidate?.candidate, event)
        if (event.candidate?.type != "relay")
            return

        if (event.candidate?.component != "rtp")
            return

        if (event.candidate?.sdpMLineIndex === 0)
            sdp_media_0 = 1

        if (event.candidate?.sdpMLineIndex === 1)
            sdp_media_1 = 1

        if (Webrtc.state.value == WebrtcState.ICEGatheringRelay && sdp_media_0 && sdp_media_1) {
            Webrtc.state.value = WebrtcState.ICEOffering
            const resp = await api.sdp_offer(pc!.localDescription)
            if (resp?.ok) handle_answer(await resp.json())
        }
    }

    pc.ontrack = function (event) {
        const track = event.track
        console.log('got remote track: kind=%s', track.kind)
        const stream = event.streams[0]


        if (track.kind == 'audio') {
            const audio: HTMLAudioElement | null = document.querySelector('audio#live')

            if (!audio) {
                return
            }

            pc_stats_start(pc)

            console.log('received remote audio stream')

            try {
                audio.srcObject = stream
            } catch (e) {
                console.log("Error attaching audio stream to element", e)
            }

            const audio_return = audio.play()

            audio_return.catch(e => {
                Error.errorAudio(true)
                console.log("Error audio play", e)
            })
        }

        if (track.kind == 'video') {
            const video: HTMLVideoElement | null = document.querySelector('video#live')

            if (!video) {
                return
            }

            console.log('received remote video stream')

            try {
                video.srcObject = stream
            } catch (e) {
                console.log("Error attaching video stream to element", e)
            }

            /* play() rejects, it does not throw - a try/catch never sees the
             * autoplay error and it escapes as an unhandled rejection */
            video.play().catch(e => {
                console.log("Error video play", e)
            })
        }
    }

    pc.onicegatheringstatechange = async () => {
        console.log('webrtc/iceGatheringState: ' + pc?.iceGatheringState)
        switch (pc?.iceGatheringState) {
            case 'new':
                /* gathering is either just starting or has been reset */
                break
            case 'gathering':
                /* gathering has begun or is ongoing */
                break
            case 'complete':
                break
        }
    }

    pc.onsignalingstatechange = () => {
        console.log('webrtc/signalingState: ' + pc?.signalingState)
    }

    pc.onicecandidateerror = (event: any) => {
        console.log('ICE Candidate Error: ' + event.errorCode + ' ' + event.errorText + ' ' + event.url)
    }

    await Avdummy.init()
    Avdummy.stream?.getTracks().forEach((track) => pc?.addTrack(track, Avdummy.stream!))

    pc_offer()
}

async function pc_media_audio() {
    audiostream = stop_stream(audiostream)

    try {
        audiostream = await get_media(
            audio_constraints(Webrtc.audio_input_id.value, Webrtc.echo.value),
            'Microphone'
        )
    } catch (e) {
        Error.error(media_error_message('Microphone', e))
    }
}


async function pc_media_video(width: number, height: number) {
    videostream = stop_stream(videostream)
    /* null it out: leaving the stopped stream behind means join() later hands
     * an ended track to replaceTrack(), which throws InvalidStateError */

    try {
        Error.errorVideo('')
        videostream = await get_media(
            video_constraints(Webrtc.video_input_id.value, width, height),
            'Camera'
        )
    } catch (e) {
        Error.errorVideo(media_error_message('Camera', e))
    }
}

/* Must be called synchronously from a click handler. Safari ties transient
 * user activation to the current task, so calling getDisplayMedia() from a
 * timer or after an await rejects with InvalidAccessError. */
export function pc_screen_request(): Promise<MediaStream | null> {
    if (!has_display_media()) {
        Error.error('Screen sharing is not supported by this browser.')
        screen_request = Promise.resolve(null)
        return screen_request
    }

    screen_request = navigator.mediaDevices
        .getDisplayMedia({ video: true, audio: false })
        .catch((e) => {
            /* the user closing the picker is not an error worth reporting */
            if ((e as DOMException)?.name !== 'NotAllowedError')
                Error.error(media_error_message('Screen', e))
            else
                console.log('pc_screen: cancelled by user')
            return null
        })

    return screen_request
}

async function pc_screen() {
    screenstream = stop_stream(screenstream)

    const pending = screen_request ?? pc_screen_request()
    screen_request = null

    screenstream = await pending

    /* browsers show their own "Stop sharing" control - follow it */
    screenstream?.getVideoTracks()[0]?.addEventListener('ended', () => {
        if (Webrtc.video_select.value === 'Screen')
            Webrtc.video_select.value = 'Disabled'
    })
}

async function pc_replace_tracks(audio_track: MediaStreamTrack | null, video_track: MediaStreamTrack | null) {

    const audio = pc?.getSenders().find((s) => s.track?.kind === 'audio')
    const video = pc?.getSenders().find((s) => s.track?.kind === 'video')

    if (!audio || !video) {
        console.log('pc_replace_tracks: no active audio or video tracks found')
        return
    }

    /* replaceTrack() throws InvalidStateError on an already ended track */
    if (audio_track?.readyState === 'ended') {
        console.warn('pc_replace_tracks: audio track already ended')
        audio_track = null
    }

    if (video_track?.readyState === 'ended') {
        console.warn('pc_replace_tracks: video track already ended')
        video_track = null
    }

    try {
        const pending: Promise<void>[] = []

        if (audio_track) pending.push(audio.replaceTrack(audio_track))
        if (video_track) pending.push(video.replaceTrack(video_track))

        if (!pending.length)
            return

        await Promise.all(pending)
        console.log('pc_replace_tracks:', audio_track ? 'audio' : '', video_track ? 'video' : '')
    } catch (e) {
        console.error('pc_replace_tracks:', e)
    }
}

async function audio_output(device: string | undefined) {
    if (!device)
        return

    /* querySelector() returns null, never undefined */
    const audio_out = document.querySelector<HTMLAudioElement>('audio#live')

    /* setSinkId is unsupported on Safari and was disabled by default on
     * Firefox until 116 */
    if (!audio_out || typeof (audio_out as any).setSinkId !== 'function') {
        console.log('webrtc: audio element not found or setSinkId not supported')
        return
    }

    try {
        await (audio_out as any).setSinkId(device)
        console.log('webrtc: changed output')
    } catch (e) {
        /* rejects with NotAllowedError without output device permission */
        console.warn('webrtc: setSinkId failed', e)
        Error.error('Could not switch the speaker: ' + ((e as Error)?.message ?? String(e)))
    }
}

/* Device ids are not stable identifiers, so fall back to matching on the
 * label before giving up on a remembered device. */
function resolve_device(
    devices: MediaDeviceInfo[],
    kind: MediaDeviceKind,
    id: string | undefined,
    label: string | undefined
): string | undefined {
    const list = devices.filter((d) => d.kind === kind && d.deviceId)

    if (id && list.some((d) => d.deviceId === id))
        return id

    if (label)
        return list.find((d) => d.label === label)?.deviceId

    return undefined
}

async function change_video_now(): Promise<MediaStream | null> {
    const select = Webrtc.video_select.value

    if (select !== 'Screen') {
        screenstream = stop_stream(screenstream)
    }

    if (select === 'Disabled') {
        Webrtc.video_mute(true)
        videostream = stop_stream(videostream)
        Webrtc.videostream.value = null
        return null
    }

    if (select === 'Screen') {
        videostream = stop_stream(videostream)

        await pc_screen()
        Webrtc.videostream.value = screenstream

        /* picker cancelled or unsupported - do not leave the UI on "Screen" */
        if (!screenstream)
            Webrtc.video_select.value = 'Disabled'

        return screenstream
    }

    const res = resolutions[Webrtc.video_resolution.value] ?? resolutions['720p']
    await updateBandwidthRestriction(res.bandwidth)
    await pc_media_video(res.width, res.height)

    const track = videostream?.getVideoTracks()[0]
    if (track) {
        /* remember what we actually got, not what we asked for */
        Webrtc.video_input_id.value = track.getSettings().deviceId ?? Webrtc.video_input_id.value
        Webrtc.video_input_label.value = track.label
    }

    Webrtc.videostream.value = videostream
    if (!videostream)
        return null

    Avdummy.stopDrawLoop()

    console.log('video changed', res)
    return videostream
}


export enum WebrtcState {
    Error = 0,
    Offline,
    ICEGatheringRelay,
    ICEOffering,
    Listening,
    ReadySpeaking,
    Speaking,
}

// --- Public Webrtc API ---
export const Webrtc = {
    state: ref(WebrtcState.Offline),
    deviceInfos: ref<MediaDeviceInfo[] | undefined>([]),
    audio_input_id: useStorage('audio_input_id', undefined as string | undefined),
    audio_input_label: useStorage('audio_input_label', undefined as string | undefined),
    audio_output_id: ref<string | undefined>(undefined),
    video_input_id: useStorage('video_input_id', undefined as string | undefined),
    video_input_label: useStorage('video_input_label', undefined as string | undefined),
    video_select: ref<string>('Disabled'),
    video_resolution: ref<string>('720p'),
    audio_muted: ref(true),
    echo: ref(false),
    video_muted: ref(true),
    videostream: ref(<MediaStream | null>(null)),

    init() {
        watch(this.state, () => { console.log("WebrtcState:", WebrtcState[this.state.value]) })
    },
    async listen() {
        if (this.state.value != WebrtcState.Offline)
            return

        /* set before pc_setup() so the first relay candidate cannot race the
         * state transition and lose the offer */
        this.state.value = WebrtcState.ICEGatheringRelay
        await pc_setup()

        const audio: HTMLAudioElement | null = document.querySelector('audio#live')
        const audio_return = audio?.play()

        audio_return?.catch(e => {
            Error.errorAudio(true)
            console.log("Error audio play", e)
        })
    },

    async update_avdevices() {
        if (!media_supported())
            return
        Webrtc.deviceInfos.value = await navigator.mediaDevices.enumerateDevices()
    },

    async init_avdevices() {
        if (!media_supported())
            return

        await pc_media_audio()

        /* do not prompt for the camera when the user is joining audio only */
        if (this.video_select.value !== 'Disabled')
            await this.change_video()

        /* labels and ids are only exposed after permission was granted */
        this.deviceInfos.value = await navigator.mediaDevices.enumerateDevices()

        const audio_track = audiostream?.getAudioTracks()[0]
        const audio_id = audio_track?.getSettings().deviceId

        /* the remembered id may be stale while the label still matches */
        const wanted_audio = resolve_device(
            this.deviceInfos.value, 'audioinput',
            this.audio_input_id.value, this.audio_input_label.value
        )

        if (wanted_audio && wanted_audio !== audio_id) {
            this.audio_input_id.value = wanted_audio
            await this.change_audio()
        } else if (audio_track) {
            this.audio_input_id.value = audio_id ?? undefined
            this.audio_input_label.value = audio_track.label
        }

        if (!this.audio_output_id.value) {
            /* Safari does not enumerate audiooutput devices at all */
            const out = this.deviceInfos.value.find((d) => d.kind === 'audiooutput' && d.deviceId)
            if (out) {
                this.audio_output_id.value = out.deviceId
                await this.change_audio_out()
            }
        }

        const video_id = videostream?.getVideoTracks()[0]?.getSettings().deviceId
        const wanted_video = resolve_device(
            this.deviceInfos.value, 'videoinput',
            this.video_input_id.value, this.video_input_label.value
        )

        if (this.video_select.value !== 'Disabled' && wanted_video && wanted_video !== video_id) {
            this.video_input_id.value = wanted_video
            await this.change_video()
        } else if (!wanted_video && this.video_select.value === 'Disabled') {
            this.video_input_id.value = undefined
            this.video_input_label.value = undefined
        }

        useEventListener(navigator.mediaDevices, 'devicechange', Webrtc.update_avdevices)
    },

    async change_audio() {
        await pc_media_audio()

        const track = audiostream?.getAudioTracks()[0]
        if (track) {
            this.audio_input_id.value = track.getSettings().deviceId ?? this.audio_input_id.value
            this.audio_input_label.value = track.label
        }

        this.audio_mute(this.audio_muted.value)

        if (track && this.state.value >= WebrtcState.ReadySpeaking)
            await pc_replace_tracks(track, null)

        console.log('audio changed')
    },

    async change_audio_out() {
        await audio_output(this.audio_output_id.value)
    },

    /* Serialized: concurrent getUserMedia() on one device fails with
     * NotReadableError, and the shared constraint object used to be mutated
     * by whichever caller ran last. */
    async change_video(): Promise<MediaStream | null> {
        video_queue = video_queue.catch(() => null).then(change_video_now)
        return video_queue
    },

    async change_echo() {
        console.log('echo changed', this.echo.value)
        await this.change_audio()
    },

    async join() {
        if (this.video_select.value === 'Disabled') {
            this.video_mute(true)
        } else {
            this.video_mute(false)
        }

        if (this.state.value < WebrtcState.ReadySpeaking) {
            const audio_track = audiostream?.getAudioTracks()[0] ?? null
            const video_track = this.video_select.value === 'Screen'
                ? screenstream?.getVideoTracks()[0] ?? null
                : videostream?.getVideoTracks()[0] ?? null

            if (audio_track || video_track) {
                await pc_replace_tracks(audio_track, video_track)
                if (audio_track) this.audio_mute(false)
                if (video_track) this.video_mute(false)
            }

            this.state.value = WebrtcState.ReadySpeaking
        }
    },

    async audio_mute(mute: boolean) {
        if (!State.user.value.speaker) mute = true

        audiostream?.getAudioTracks().forEach((track) => {
            track.enabled = !mute
        })

        /* was inside the forEach: without a track the mute state and the
         * server never learned about the change */
        this.audio_muted.value = mute
        api.audio(!mute)
    },

    async video_mute(mute: boolean, local?: boolean) {
        var stream = videostream

        if (this.video_select.value === 'Disabled')
            mute = true

        if (!State.user.value.speaker) mute = true

        if (this.video_select.value === 'Screen')
            stream = screenstream

        if (!mute && stream) {
            pc_replace_tracks(null, stream.getVideoTracks()[0])
            Avdummy.stopDrawLoop()
        }
        else {
            pc_replace_tracks(null, Avdummy.getVideoTrack())
            Avdummy.refresh()
        }

        if (!local) {
            this.video_muted.value = mute
            api.video(!mute)
        }
    },

    hangup() {
        videostream = stop_stream(videostream)
        screenstream = stop_stream(screenstream)
        audiostream = stop_stream(audiostream)
        this.videostream.value = null
        this.video_select.value = 'Disabled'

        pc_stats_stop()
        Avdummy.close()

        pc?.close()
        pc = null
        this.state.value = WebrtcState.Offline
    },
}
