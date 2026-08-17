import api from './api'
import { State } from './ws/state'

const width = 1280
const height = 720
const canvas = Object.assign(document.createElement('canvas'), { width, height })
const ctx = canvas.getContext('2d')
const image = new Image()
const DRAW_MAX = 150

let drawLoopIsRunning = false
let drawCount = 0

/* One shared AudioContext: Chrome hard-caps a page at ~6 of them, so a new
 * one per listen()/hangup() cycle eventually throws. */
let actx: AudioContext | null = null

function draw() {
    if (!ctx)
        return

    ctx.fillStyle = "black";
    ctx.fillRect(0, 0, width, height)
    ctx.font = "48px serif";
    ctx.textAlign = "center"
    ctx.fillStyle = "gray";
    ctx.fillText(State.user.value.name, width / 2, height / 2 + image.height / 2);
    if (image.complete && image.naturalWidth)
        ctx.drawImage(image, width / 2 - (image.width / 2), (height / 2 - image.height / 2) - 48)
}

function drawLoop() {
    let wait = 500
    if (drawCount++ >= DRAW_MAX) {
        drawLoopIsRunning = false
        return
    }

    drawLoopIsRunning = true

    draw()

    setTimeout(() => {
        drawLoop()
    }, wait)
}

const black = () => {
    const stream = canvas.captureStream()
    if (!ctx)
        return stream.getVideoTracks()[0]

    ctx.fillRect(0, 0, width, height)

    const start = () => {
        drawCount = 0
        //Chrome workaround: needs canvas frame change to start webrtc rtp
        drawLoop()
    }

    image.src = '/avatars/' + api.user_id() + '.png'
    image.onload = start
    /* without this a missing avatar means the canvas never changes, and on
     * Chrome the dummy video track then never starts sending RTP */
    image.onerror = () => {
        console.warn('avdummy: avatar image could not be loaded')
        start()
    }

    return stream.getVideoTracks()[0]
}

const silence = () => {
    if (!actx || actx.state === 'closed')
        actx = new AudioContext()

    /* created outside a user gesture the context starts suspended */
    if (actx.state === 'suspended')
        actx.resume().catch((e) => console.log('avdummy: audiocontext resume', e))

    const oscillator = actx.createOscillator()
    const dst = actx.createMediaStreamDestination()
    oscillator.connect(dst)
    oscillator.start()
    return Object.assign(dst.stream.getAudioTracks()[0], { enabled: false })
}

export const Avdummy = {
    stream: <null | MediaStream>null,

    async init() {
        /* reuse while the tracks are still live */
        if (this.stream?.getTracks().every((t) => t.readyState === 'live'))
            return

        this.stream = new MediaStream([black(), silence()])
    },

    getVideoTrack(): MediaStreamTrack | null {
        return this.stream?.getVideoTracks()[0] ?? null
    },

    getAudioTrack(): MediaStreamTrack | null {
        return this.stream?.getAudioTracks()[0] ?? null
    },

    async refresh() {
        draw()
        if (!drawLoopIsRunning) {
            drawCount = DRAW_MAX - 10
            drawLoop()
        }
    },

    async stopDrawLoop() {
        drawCount = DRAW_MAX
    },

    close() {
        this.stopDrawLoop()
        this.stream?.getTracks().forEach((track) => track.stop())
        this.stream = null
        actx?.close().catch((e) => console.log('avdummy: audiocontext close', e))
        actx = null
    },
}
