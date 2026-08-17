import Cropper from 'cropperjs'
import { ref } from 'vue'

let videoStream: MediaStream | undefined
let cropper: Cropper | undefined
let hvideo: HTMLVideoElement | null

function getRoundedCanvas(sourceCanvas: HTMLCanvasElement | undefined) {
    if (!sourceCanvas)
        return
    const canvas = document.createElement('canvas')
    const ctx = canvas.getContext('2d')
    const width = sourceCanvas.width
    const height = sourceCanvas.height

    canvas.width = width
    canvas.height = height
    if (ctx) {
        ctx.imageSmoothingEnabled = true
        ctx.drawImage(sourceCanvas, 0, 0, width, height)
        ctx.globalCompositeOperation = 'destination-in'
        ctx.beginPath()
        ctx.arc(width / 2, height / 2, Math.min(width, height) / 2, 0, 2 * Math.PI, true)
        ctx.fill()
    }
    return canvas;
}
const constraintsVideo: any = {
    audio: false,
    video: {
        deviceId: undefined,
    },
}

export default {
    picture: ref<string | undefined>(),
    preview: ref(false),
    deviceInfos: ref<MediaDeviceInfo[] | undefined>([]),
    deviceId: ref<string | undefined>(undefined),

    video(video: HTMLVideoElement | null) {
        hvideo = video
    },

    async start() {
        if (!navigator.mediaDevices?.getUserMedia) {
            console.error('webcam: getUserMedia unavailable (insecure origin?)')
            return
        }

        /* stop the previous stream first: iOS Safari only allows one camera
         * stream at a time and silently tears the old one down otherwise */
        videoStream?.getTracks().forEach((t) => t.stop())
        videoStream = undefined

        try {
            /* 'ideal' so a stale stored id falls back to the default camera
             * instead of rejecting with OverconstrainedError */
            constraintsVideo.video.deviceId = this.deviceId.value
                ? { ideal: this.deviceId.value }
                : undefined
            videoStream = await navigator.mediaDevices.getUserMedia(constraintsVideo)
        } catch (e) {
            console.error(`An error occurred: ${e}`)
        }
        this.deviceId.value = videoStream?.getVideoTracks()[0]?.getSettings().deviceId
        if (hvideo && videoStream) {
            hvideo.srcObject = videoStream
            /* play() rejects, it does not throw */
            hvideo.play().catch((e) => console.log('webcam: play', e))
        }

        this.picture.value = undefined
        this.preview.value = false
        this.deviceInfos.value = await navigator.mediaDevices.enumerateDevices()
    },

    stop() {
        videoStream?.getTracks().forEach((t) => t.stop())
        videoStream = undefined
    },

    takePicture(canvas: HTMLCanvasElement | null, video: HTMLVideoElement | null) {
        if (!video || !canvas)
            return

        this.preview.value = true
        const context = canvas.getContext('2d')
        const width = 512
        const height = 512 * video.videoHeight / video.videoWidth

        canvas.setAttribute('width', String(width))
        canvas.setAttribute('height', String(height))

        context?.drawImage(video!, 0, 0, width, height)
        cropper = new Cropper(canvas)
        const selection = cropper?.getCropperSelection()
        if (selection) {
            selection.initialAspectRatio = 1
            selection.initialCoverage = 0.8
        }
    },

    async savePicture() {
        const selection = cropper?.getCropperSelection()
        if (!selection)
            return
        const croppedCanvas = await selection.$toCanvas()
        const roundedCanvas = getRoundedCanvas(croppedCanvas)
        this.picture.value = roundedCanvas?.toDataURL('image/png')
        cropper?.destroy()
    },
}
