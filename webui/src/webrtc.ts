import api from './api'
import { ref } from 'vue'

let pc: RTCPeerConnection

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
let silence = () => {
    let ctx = new AudioContext()
    let oscillator = ctx.createOscillator()
    let dst = ctx.createMediaStreamDestination()
    oscillator.connect(dst)
    oscillator.start()
    return Object.assign(dst.stream.getAudioTracks()[0], { enabled: false })
}

let black = ({ width = 480, height = 360 } = {}) => {
    let canvas = Object.assign(document.createElement('canvas'), { width, height })
    //Chrome workaround: needs canvas frame change to start webrtc rtp
    canvas.getContext('2d')?.fillRect(0, 0, width, height)
    setTimeout(() => { canvas.getContext('2d')?.fillRect(0, 0, width, height) }, 2000);
    let stream = canvas.captureStream()
    return Object.assign(stream.getVideoTracks()[0], { enabled: false })
}

let AVSilence = (...args: any) => new MediaStream([black(...args), silence()])
/** End AVdummy **/

function handle_answer(descr: any) {
    if (!descr)
        return

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
            let audio: HTMLAudioElement | null = document.querySelector('audio#live')

            if (!audio) {
                return
            }

            if (audio.srcObject !== event.streams[0]) {
                audio.srcObject = event.streams[0]
                console.log('received remote audio stream')
            }
        }

        if (track.kind == 'video') {
            let video: HTMLVideoElement | null = document.querySelector('video#live')

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
                await api.connect()
                let response = await api.sdp(pc.localDescription)
                handle_answer(await response?.json())
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
    let video_audio = AVSilence()
    video_audio.getTracks().forEach((track) => pc.addTrack(track, video_audio))

    pc_offer()
}

function pc_media() {
}

export enum WebrtcState {
    Error = 0,
    Offline,
    Connecting,
    Listening,
    Speaking
}

export const Webrtc = {
    state: ref(WebrtcState.Offline),
    errorText: ref(""),
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
    error_reset()
    {
        this.errorText.value = ""
        this.state.value = WebrtcState.Offline
    }
}
