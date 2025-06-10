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

const sleep = (delay: number) => new Promise((resolve) => setTimeout(resolve, delay))

const constraintsAudio: any = {
    audio: {
        echoCancellation: false, // disabling audio processing
        autoGainControl: false,
        noiseSuppression: false,
        latency: 0.02, //20ms
        sampleRate: 48000,
        deviceId: undefined,
    },
    video: false,
}

const constraintsVideo: any = {
    audio: false,
    video: {
        width: 1280,
        height: 720,
        deviceId: undefined,
    },
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
        function(error) {
            console.warn('setRemoteDescription: %s', error.toString())
        }
    )
}

function pc_offer() {
    const offerOptions = {
        iceRestart: false,
    }
    pc?.createOffer(offerOptions)
        .then(function(desc) {
            console.log('got local description: %s', desc.type)

            pc?.setLocalDescription(desc).then(
                () => { },
                function(error) {
                    console.log('setLocalDescription: %s', error.toString())
                }
            )
        })
        .catch(function(error) {
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
    setTimeout(pc_stats, 5000, pc)
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

    pc.ontrack = function(event) {
        const track = event.track
        console.log('got remote track: kind=%s', track.kind)
        const stream = event.streams[0]


        if (track.kind == 'audio') {
            const audio: HTMLAudioElement | null = document.querySelector('audio#live')

            if (!audio) {
                return
            }

            pc_stats(pc)

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

            try {
                video.play()
            } catch (e) {
                console.log("Error video play", e)
            }
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
    audiostream?.getAudioTracks()[0].stop()

    try {
        audiostream = await navigator.mediaDevices.getUserMedia(constraintsAudio)
    } catch (e) {
        console.error('pc_media_audio: permission denied...', e)
        Error.error('Microphone permission denied!')
        return
    }
}


async function pc_media_video() {
    videostream?.getVideoTracks()[0].stop()

    for (let i = 0; i < 5; i++) {
        try {
            Error.errorVideo('')
            videostream = await navigator.mediaDevices.getUserMedia(constraintsVideo)
            return
        } catch (e) {
            console.error('pc_media_video/error: ', e)
            Error.errorVideo('Camera: ' + e)
            if (e instanceof DOMException) {
                /* Sometimes camera is not ready yet, since stream is not stopped
                 * synchronously, so retry after timeout */
                if (e.name === 'NotReadableError') {
                    await sleep(200)
                    continue
                }
            }
            return
        }
    }
}

async function pc_screen() {
    screenstream?.getVideoTracks()[0].stop()
    const gdmOptions = {
        video: true,
        audio: false,
    }
    try {
        screenstream = await navigator.mediaDevices.getDisplayMedia(gdmOptions)
    } catch (e) {
        console.error('pc_screen: ', e)
        Error.error('Screen: ' + e)
    }
}

async function pc_replace_tracks(audio_track: MediaStreamTrack | null, video_track: MediaStreamTrack | null) {

    const audio = pc?.getSenders().find((s) => s.track?.kind === 'audio')
    const video = pc?.getSenders().find((s) => s.track?.kind === 'video')

    if (!audio || !video) {
        console.log('pc_replace_tracks: no active audio or video tracks found')
        return
    }

    if (audio_track && video_track) {
        await Promise.all([audio.replaceTrack(audio_track), video.replaceTrack(video_track)])
        console.log('pc_replace_tracks: audio and video')
        return
    }

    if (audio_track) {
        await Promise.all([audio.replaceTrack(audio_track)])
        console.log('pc_replace_tracks: audio')
        return
    }

    if (video_track) {
        await Promise.all([video.replaceTrack(video_track)])
        console.log('pc_replace_tracks: video')
        return
    }
}

async function audio_output(device: string | undefined) {
    let audio_out: any = document.querySelector("audio#live");

    if (
        typeof audio_out === "undefined" ||
        typeof audio_out?.setSinkId === "undefined"
    ) {
        console.log(
            "webrtc: audio element not found or setSinkId not supported"
        );
        return;
    }

    await audio_out?.setSinkId(device);
    console.log("webrtc: changed output");
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
    audio_output_id: ref<string | undefined>(undefined),
    video_input_id: useStorage('video_input_id', undefined as string | undefined),
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
        pc_setup()
        this.state.value = WebrtcState.ICEGatheringRelay
        const audio: HTMLAudioElement | null = document.querySelector('audio#live')
        const audio_return = audio?.play()

        audio_return?.catch(e => {
            Error.errorAudio(true)
            console.log("Error audio play", e)
        })
    },

    async update_avdevices() {
        Webrtc.deviceInfos.value = await navigator.mediaDevices.enumerateDevices()
    },

    async init_avdevices() {
        let audio_available = false
        let video_available = false
        await pc_media_audio()
        await pc_media_video()
        this.deviceInfos.value = await navigator.mediaDevices.enumerateDevices()

        this.deviceInfos.value.forEach((device) => {
            console.log(`${device.kind}: ${device.label} id = ${device.deviceId}`)
            if (device.kind === 'audiooutput' && !this.audio_output_id.value) {
                this.audio_output_id.value = device.deviceId
                this.change_audio_out()
                return
            }

            if (this.audio_input_id.value) {
                if (this.audio_input_id.value == device.deviceId) {
                    audio_available = true
                }
            }
            if (this.video_input_id.value) {
                if (this.video_input_id.value == device.deviceId) {
                    video_available = true
                }
            }
        })

        if (audio_available)
            this.change_audio()
        else
            this.audio_input_id.value = audiostream?.getAudioTracks()[0].getSettings().deviceId

        if (!video_available)
            this.video_input_id.value = undefined

        useEventListener(navigator!.mediaDevices, 'devicechange', Webrtc.update_avdevices)
    },

    async change_audio() {
        constraintsAudio.audio.deviceId = { exact: this.audio_input_id.value }
        await pc_media_audio()
        this.audio_mute(this.audio_muted.value)
        if (audiostream && this.state.value >= WebrtcState.ReadySpeaking)
            await pc_replace_tracks(audiostream.getAudioTracks()[0], null)
        console.log('audio changed')
    },

    async change_audio_out() {
        await audio_output(this.audio_output_id.value)
    },

    async change_video(): Promise<MediaStream | null> {
        screenstream?.getVideoTracks()[0].stop()

        if (this.video_select.value === 'Disabled') {
            this.video_mute(true)
            videostream?.getVideoTracks()[0].stop()
            return null
        }

        if (this.video_select.value === 'Screen') {
            videostream?.getVideoTracks()[0].stop()
            await pc_screen()
            this.videostream.value = screenstream
            return screenstream
        }

        if (this.video_resolution.value === '720p') {
            constraintsVideo.video.width = 1280
            constraintsVideo.video.height = 720
            updateBandwidthRestriction(2000)
        }

        if (this.video_resolution.value === '360p') {
            constraintsVideo.video.width = 640
            constraintsVideo.video.height = 360
            updateBandwidthRestriction(1000)
        }

        if (this.video_resolution.value === '360pl') {
            constraintsVideo.video.width = 640
            constraintsVideo.video.height = 360
            updateBandwidthRestriction(400)
        }

        if (this.video_input_id.value !== 'undefined')
            constraintsVideo.video.deviceId = { exact: this.video_input_id.value }

        await pc_media_video()
        if (!this.video_input_id.value) {
            this.video_input_id.value = videostream?.getVideoTracks()[0].getSettings().deviceId
        }

        this.videostream.value = videostream
        if (videostream === null)
            return null

        Avdummy.stopDrawLoop()

        console.log('video changed', constraintsVideo)
        return videostream
    },

    async change_echo() {
        constraintsAudio.audio.echoCancellation = this.echo.value
        console.log('echo changed', constraintsAudio)
        await this.change_audio()
    },

    async join() {
        if (this.video_select.value === 'Disabled') {
            this.video_mute(true)
        } else {
            this.video_mute(false)
        }

        if (this.state.value < WebrtcState.ReadySpeaking) {
            if (this.video_select.value === 'Screen' && audiostream && screenstream) {
                await pc_replace_tracks(audiostream.getAudioTracks()[0], screenstream.getVideoTracks()[0])
                this.audio_mute(false)
                this.video_mute(false)
            }
            else if (videostream && audiostream) {
                await pc_replace_tracks(audiostream.getAudioTracks()[0], videostream.getVideoTracks()[0])
                this.audio_mute(false)
                this.video_mute(false)
            }
            else if (audiostream) {
                await pc_replace_tracks(audiostream.getAudioTracks()[0], null)
                this.audio_mute(false)
            }
            this.state.value = WebrtcState.ReadySpeaking
        }
    },

    async audio_mute(mute: boolean) {
        if (!State.user.value.speaker) mute = true

        audiostream?.getAudioTracks().forEach((track) => {
            track.enabled = !mute
            this.audio_muted.value = mute
            api.audio(!mute)
        })
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
        videostream?.getVideoTracks()[0].stop()
        screenstream?.getVideoTracks()[0].stop()
        audiostream?.getAudioTracks()[0].stop()
        this.video_select.value = 'Disabled'

        pc?.close()
        pc = null
        this.state.value = WebrtcState.Offline
    },
}
