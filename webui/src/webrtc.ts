import api from './api'
import { Users } from './ws/users'
import { ref } from 'vue'
import adapter from 'webrtc-adapter'
import { Error } from './error'
import { useEventListener, useStorage } from '@vueuse/core'

// --- Private Webrtc API ---
let pc: RTCPeerConnection | null = null
let avdummy: MediaStream
let audiostream: MediaStream | null = null
let videostream: MediaStream | null = null
let screenstream: MediaStream | null = null

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
            urls: 'turn:195.201.63.86:3478',
            username: 'turn200301',
            credential: 'choh4zeem3foh1',
        },
    ],


    /* default on Firefox/Chrome but needed by Safari */
    rtcpMuxPolicy: 'require'
}

/** Start AVdummy **/
const silence = () => {
    const ctx = new AudioContext()
    const oscillator = ctx.createOscillator()
    const dst = ctx.createMediaStreamDestination()
    oscillator.connect(dst)
    oscillator.start()
    return Object.assign(dst.stream.getAudioTracks()[0], { enabled: false })
}

function drawLoop(ctx: CanvasRenderingContext2D, image: HTMLImageElement, width: number, height: number, cnt: number) {
    if (cnt > 20) 
        return

    cnt = cnt + 1
    setTimeout(() => {
        ctx.fillStyle = "black";
        ctx.fillRect(0, 0, width, height)
        ctx.font = "48px serif";
        ctx.textAlign = "center"
        ctx.fillStyle = "gray";
        ctx.fillText(api.session().user_name, width / 2, height / 2 + image.height / 2);
        ctx.drawImage(image, width / 2 - (image.width / 2), (height / 2 - image.height / 2) - 48)
        drawLoop(ctx, image, width, height, cnt)
    }, 100)
}

const black = ({ width = 1280, height = 720 } = {}) => {
    const canvas = Object.assign(document.createElement('canvas'), { width, height })
    const ctx = canvas.getContext('2d')
    const stream = canvas.captureStream()
    if (!ctx)
        return stream.getVideoTracks()[0]

    ctx.fillRect(0, 0, width, height)

    const image = new Image()
    image.src = '/avatars/' + api.session().user_id + '.png'
    image.onload = () => {
        //Chrome workaround: needs canvas frame change to start webrtc rtp
        drawLoop(ctx, image, width, height, 0)
    }
    return stream.getVideoTracks()[0]
}

const AVSilence = (...args: any) => new MediaStream([black(...args), silence()])
/** End AVdummy **/

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

function pc_setup() {
    console.log('browser: ', adapter.browserDetails.browser, adapter.browserDetails.version)
    pc = new RTCPeerConnection(configuration)

    pc.onicecandidate = (event) => {
        console.log('webrtc/icecandidate: ' + event.candidate?.type + ' IP: ' + event.candidate?.candidate)
    }

    pc.ontrack = function (event) {
        const track = event.track
        console.log('got remote track: kind=%s', track.kind)

        if (track.kind == 'audio') {
            const audio: HTMLAudioElement | null = document.querySelector('audio#live')

            if (!audio) {
                return
            }

            if (audio.srcObject !== event.streams[0]) {
                audio.srcObject = event.streams[0]
                console.log('received remote audio stream')
                audio.play()
            }
        }

        if (track.kind == 'video') {
            const video: HTMLVideoElement | null = document.querySelector('video#live')

            if (!video) {
                return
            }

            if (video.srcObject !== event.streams[0]) {
                video.srcObject = event.streams[0]
                console.log('received remote video stream')
                video.play()
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
                const resp = await api.sdp_offer(pc.localDescription)
                if (resp?.ok) handle_answer(await resp.json())
                break
        }
    }

    pc.onsignalingstatechange = () => {
        console.log('webrtc/signalingState: ' + pc?.signalingState)
    }

    pc.onicecandidateerror = (event: any) => {
        console.log('ICE Candidate Error: ' + event.errorCode + ' ' + event.errorText + ' ' + event.url)
    }

    avdummy = AVSilence()
    avdummy.getTracks().forEach((track) => pc?.addTrack(track))

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

    try {
        videostream = await navigator.mediaDevices.getUserMedia(constraintsVideo)
    } catch (e) {
        videostream = null
        console.error('pc_media_video: permission denied...', e)
        Error.error('Camera permission denied!')
        return
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
    } catch (err) {
        console.error('webrtc_video/setup_screen: ' + err)
        Error.error('Screen permission denied!')
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
    Connecting,
    Listening,
    ReadySpeaking,
    Speaking,
}

// --- Public Webrtc API ---
export const Webrtc = {
    state: ref(WebrtcState.Offline),
    deviceInfosAudio: ref<MediaDeviceInfo[] | undefined>([]),
    deviceInfosVideo: ref<MediaDeviceInfo[] | undefined>([]),
    audio_input_id: useStorage('audio_input_id', undefined as string | undefined),
    audio_output_id: ref<string | undefined>(undefined),
    video_input_id: ref<string | undefined>(undefined),
    video_select: ref<string>('Disabled'),
    video_resolution: ref<string>('720p'),
    audio_muted: ref(true),
    echo: ref(false),
    video_muted: ref(true),

    listen() {
        pc_setup()
        this.state.value = WebrtcState.Connecting
        const audio: HTMLAudioElement | null = document.querySelector('audio#live')
        audio?.play()
    },

    async update_avdevices() {
        Webrtc.deviceInfosAudio.value = await navigator.mediaDevices.enumerateDevices()
        Webrtc.deviceInfosVideo.value = Webrtc.deviceInfosAudio.value
    },

    async init_avdevices() {
        let available = false
        await pc_media_audio()
        this.deviceInfosAudio.value = await navigator.mediaDevices.enumerateDevices()

        this.deviceInfosAudio.value.forEach((device) => {
            console.log(`${device.kind}: ${device.label} id = ${device.deviceId}`)
            if (device.kind === 'audiooutput')
                this.audio_output_id.value = 'default'

            if (this.audio_input_id.value) {
                if (this.audio_input_id.value == device.deviceId) {
                    available = true
                }
            }
        })

        if (available)
            this.change_audio()
        else
            this.audio_input_id.value = audiostream?.getAudioTracks()[0].getSettings().deviceId

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
            updateBandwidthRestriction(500)
        }

        if (this.video_input_id.value !== 'undefined')
            constraintsVideo.video.deviceId = { exact: this.video_input_id.value }

        await pc_media_video()
        if (!this.video_input_id.value) {
            this.deviceInfosVideo.value = await navigator.mediaDevices.enumerateDevices()
            this.video_input_id.value = videostream?.getVideoTracks()[0].getSettings().deviceId
        }
        if (videostream === null)
            return null

        console.log('video changed', constraintsVideo)
        return videostream
    },

    async change_echo() {
        constraintsAudio.audio.echoCancellation = this.echo.value
        console.log('echo changed', constraintsAudio)
        await this.change_audio()
    },

    videostream() {
        if (this.video_select.value === 'Screen')
            return screenstream
        return videostream
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

    audio_mute(mute: boolean) {
        if (!Users.speaker_status.value) mute = true

        audiostream?.getAudioTracks().forEach((track) => {
            track.enabled = !mute
            this.audio_muted.value = mute
            api.audio(!mute)
        })
    },

    video_mute(mute: boolean, local?: boolean) {
        var stream = videostream

        if (this.video_select.value === 'Disabled')
            mute = true

        if (!Users.speaker_status.value) mute = true

        if (this.video_select.value === 'Screen')
            stream = screenstream

        if (!mute && stream)
            pc_replace_tracks(null, stream.getVideoTracks()[0])
        else {
            avdummy = AVSilence()
            pc_replace_tracks(null, avdummy.getVideoTracks()[0])
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
        pc?.close()
        pc = null
        this.state.value = WebrtcState.Offline
    },
}
