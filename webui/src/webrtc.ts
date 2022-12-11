import api from './api'
import { ref } from 'vue'
import adapter from 'webrtc-adapter'

// --- Private Webrtc API ---
let pc: RTCPeerConnection
let avdummy: MediaStream
let avstream: MediaStream | null = null
let screenstream: MediaStream | null = null

const constraints: any = {
    audio: {
        echoCancellation: false, // disabling audio processing
        autoGainControl: false,
        noiseSuppression: false,
        latency: 0.02, //20ms
        sampleRate: 48000,
        deviceId: undefined
    },
    video: {
        width: 1280, height: 720,
        deviceId: undefined
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

const black = ({ width = 640, height = 360 } = {}) => {
    const canvas = Object.assign(document.createElement('canvas'), { width, height })
    //Chrome workaround: needs canvas frame change to start webrtc rtp
    canvas.getContext('2d')?.fillRect(0, 0, width, height)
    setTimeout(() => {
        canvas.getContext('2d')?.fillRect(0, 0, width, height)
    }, 2000)
    const stream = canvas.captureStream()
    return Object.assign(stream.getVideoTracks()[0], { enabled: false })
}

const AVSilence = (...args: any) => new MediaStream([black(...args), silence()])
/** End AVdummy **/

function handle_answer(descr: any) {
    if (!descr) return

    console.log("remote description: type='%s'", descr.type)

    pc.setRemoteDescription(descr).then(
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
    pc.createOffer(offerOptions)
        .then(function(desc) {
            console.log('got local description: %s', desc.type)

            pc.setLocalDescription(desc).then(
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

function pc_setup() {
    console.log('browser: ', adapter.browserDetails.browser, adapter.browserDetails.version)
    pc = new RTCPeerConnection(configuration)

    pc.onicecandidate = (event) => {
        console.log('webrtc/icecandidate: ' + event.candidate?.type + ' IP: ' + event.candidate?.candidate)
    }

    pc.ontrack = function(event) {
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
        console.log('webrtc/iceGatheringState: ' + pc.iceGatheringState)
        switch (pc.iceGatheringState) {
            case 'new':
                /* gathering is either just starting or has been reset */
                break
            case 'gathering':
                /* gathering has begun or is ongoing */
                break
            case 'complete':
                const resp = await api.sdp(pc.localDescription)
                if (resp?.ok) handle_answer(await resp.json())
                break
        }
    }

    pc.onsignalingstatechange = () => {
        console.log('webrtc/signalingState: ' + pc.signalingState)
    }

    pc.onicecandidateerror = (event: any) => {
        console.log('ICE Candidate Error: ' + event.errorCode + ' ' + event.errorText + ' ' + event.url)
    }

    /* Add dummy tracks */
    avdummy = AVSilence()
    avdummy.getTracks().forEach((track) => pc.addTrack(track, avdummy))

    pc_offer()
}

async function pc_media() {

    avstream?.getVideoTracks()[0].stop();
    avstream?.getAudioTracks()[0].stop();

    try {
        avstream = await navigator.mediaDevices.getUserMedia(constraints)
    } catch (e) {
        console.error('pc_media: permission denied...', e)
        Webrtc.errorText.value = "Microphone/Camera permission denied!"
        return
    }


}

async function pc_screen() {
    const gdmOptions = {
        video: true,
        audio: false,
    };
    try {
        screenstream = await navigator.mediaDevices.getDisplayMedia(
            gdmOptions
        );
    } catch (err) {
        console.error("webrtc_video/setup_screen: " + err);
    }
}

async function pc_replace_tracks(audio_track: MediaStreamTrack, video_track: MediaStreamTrack | null) {
    const audio = pc.getSenders().find((s) => s.track?.kind === 'audio')
    const video = pc.getSenders().find((s) => s.track?.kind === 'video')

    if (!audio || !video) {
        console.log('pc_replace_tracks: no audio or video tracks found')
        return
    }

    if (video_track) {
        await Promise.all([audio.replaceTrack(audio_track), video.replaceTrack(video_track)])
        console.log('pc_replace_tracks: audio and video')

        return
    }

    await Promise.all([audio.replaceTrack(audio_track)])
    console.log('pc_replace_tracks: audio')
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
    errorText: ref(''),
    deviceInfos: ref<MediaDeviceInfo[] | undefined>([]),
    audio_input_id: ref<string | undefined>(undefined),
    audio_output_id: ref<string | undefined>(undefined),
    video_input_id: ref<string | undefined>(undefined),
    audio_muted: ref(true),
    echo: ref(false),
    video_muted: ref(true),

    listen() {
        pc_setup()
        this.state.value = WebrtcState.Connecting
        const audio: HTMLAudioElement | null = document.querySelector('audio#live')
        audio?.play()
    },

    async init_avdevices(): Promise<MediaStream | null> {
        await pc_media()

        this.deviceInfos.value = await navigator.mediaDevices.enumerateDevices()

        this.deviceInfos.value.forEach((device) => {
            console.log(`${device.kind}: ${device.label} id = ${device.deviceId}`)
        })

        this.video_input_id.value = avstream?.getVideoTracks()[0].getSettings().deviceId
        this.audio_input_id.value = avstream?.getAudioTracks()[0].getSettings().deviceId

        this.state.value = WebrtcState.ReadySpeaking

        return avstream
    },

    async change_audio(): Promise<MediaStream | null> {
        constraints.audio.deviceId = { exact: this.audio_input_id.value }
        await pc_media()
        this.mic_mute(this.audio_muted.value)
        console.log("audio changed")
        return avstream
    },

    async change_video(): Promise<MediaStream | null> {
        if (this.video_input_id.value === "disabled") {
            this.video_mute(true)
            return null
        }
        if (this.video_input_id.value === "screen") {
            await pc_screen()
            this.video_mute(false)
            return screenstream
        }
        constraints.video.deviceId = { exact: this.video_input_id.value }
        await pc_media()
        this.video_mute(false)
        console.log("video changed", constraints)
        return avstream
    },

    async change_echo() {
        constraints.audio.echoCancellation = this.echo.value
        console.log("echo changed", constraints)
        await pc_media()
        return avstream
    },

    async join() {
        if (avstream === null) return

        if (this.video_input_id.value === "disabled") {
            this.video_mute(true)
            await pc_replace_tracks(avstream.getAudioTracks()[0], avdummy.getVideoTracks()[0])
        }
        else if (this.video_input_id.value === "screen" && screenstream !== null) {
            this.video_mute(false)
            await pc_replace_tracks(avstream.getAudioTracks()[0], screenstream.getVideoTracks()[0])
        }
        else {
            this.video_mute(false)
            await pc_replace_tracks(avstream.getAudioTracks()[0], avstream.getVideoTracks()[0])
        }

        this.mic_mute(false)
    },

    mic_mute(mute: boolean) {
        if (!api.is_speaker())
            mute = true

        avstream?.getAudioTracks().forEach((track) => {
            track.enabled = !mute
            this.audio_muted.value = mute
            api.audio(!mute)
        });
    },

    video_mute(mute: boolean) {
        if (!api.is_speaker())
            mute = true

        avstream?.getVideoTracks().forEach((track) => {
            track.enabled = !mute
            this.video_muted.value = mute
            api.video(!mute)
        });
    },

    logout() {
        avstream?.getVideoTracks()[0].stop();
        avstream?.getAudioTracks()[0].stop();
    },

    error(msg: string) {
        this.errorText.value = msg
        this.state.value = WebrtcState.Error
    },

    error_reset() {
        this.errorText.value = ''
        this.state.value = WebrtcState.Offline
    },
}
