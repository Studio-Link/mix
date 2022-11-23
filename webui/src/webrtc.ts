import api from './api'
import { ref } from 'vue'

let pc: RTCPeerConnection
const constraints = {
    audio: {
        echoCancellation: false, // disabling audio processing
        autoGainControl: false,
        noiseSuppression: false,
        latency: 0.02, //20ms
        sampleRate: 48000,
    },
    video: { width: 1280, height: 720 },
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

const black = ({ width = 480, height = 360 } = {}) => {
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

            // console.log(desc.sdp)

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
    const av = AVSilence()
    av.getTracks().forEach((track) => pc.addTrack(track, av))

    pc_offer()
}

async function pc_media() {
    let avstream: MediaStream
    try {
        avstream = await navigator.mediaDevices.getUserMedia(constraints)
    } catch (e) {
        console.error('pc_media: permission denied...', e)
        return
    }

    await pc_replace_tracks(avstream.getAudioTracks()[0], avstream.getVideoTracks()[0])

    // let deviceInfos = await navigator.mediaDevices.enumerateDevices()
    // pc_devices(deviceInfos)
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
    Speaking,
}

export const Webrtc = {
    state: ref(WebrtcState.Offline),
    errorText: ref(''),
    listen() {
        pc_setup()
        this.state.value = WebrtcState.Connecting
    },
    speak() {
        pc_media()
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
