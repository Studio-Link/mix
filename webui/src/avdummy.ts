import api from './api'

const silence = () => {
    const ctx = new AudioContext()
    const oscillator = ctx.createOscillator()
    const dst = ctx.createMediaStreamDestination()
    oscillator.connect(dst)
    oscillator.start()
    return Object.assign(dst.stream.getAudioTracks()[0], { enabled: false })
}

function drawLoop(ctx: CanvasRenderingContext2D | null, image: HTMLImageElement, width: number, height: number, cnt: number) {
    if (cnt++ > 20 || !ctx)
        return

    setTimeout(() => {
        ctx.fillStyle = "black";
        ctx.fillRect(0, 0, width, height)
        ctx.font = "48px serif";
        ctx.textAlign = "center"
        ctx.fillStyle = "gray";
        ctx.fillText(api.session().user_name, width / 2, height / 2 + image.height / 2);
        ctx.drawImage(image, width / 2 - (image.width / 2), (height / 2 - image.height / 2) - 48)
        drawLoop(ctx, image, width, height, cnt)
    }, 10 * cnt * cnt)
}

const width = 1280
const height = 720
const canvas = Object.assign(document.createElement('canvas'), { width, height })
const ctx = canvas.getContext('2d')
const image = new Image()

const black = () => {
    const stream = canvas.captureStream()
    if (!ctx)
        return stream.getVideoTracks()[0]

    ctx.fillRect(0, 0, width, height)

    image.src = '/avatars/' + api.session().user_id + '.png'
    image.onload = () => {
        //Chrome workaround: needs canvas frame change to start webrtc rtp
        drawLoop(ctx, image, width, height, 0)
    }
    return stream.getVideoTracks()[0]
}

export const Avdummy = {
    stream: <null | MediaStream>null,

    async init() {
        this.stream = new MediaStream([black(), silence()])
    },

    getVideoTrack(): MediaStreamTrack | null {
        return this.stream?.getVideoTracks()[0] ?? null
    },

    getAudioTrack(): MediaStreamTrack | null {
        return this.stream?.getAudioTracks()[0] ?? null
    },

    async refresh() {
        drawLoop(ctx, image, width, height, 10)
    }
}
