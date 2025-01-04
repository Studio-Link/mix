import Cropper from 'cropperjs'
import 'cropperjs/dist/cropper.css'
import { ref } from 'vue'

let videoStream: MediaStream | undefined
let cropper: Cropper | undefined
let hvideo: HTMLVideoElement | null

function getRoundedCanvas(sourceCanvas: HTMLCanvasElement | undefined) {
    if (!sourceCanvas)
        return
    const canvas = document.createElement('canvas');
    const ctx = canvas.getContext('2d');
    const width = sourceCanvas.width;
    const height = sourceCanvas.height;

    canvas.width = width;
    canvas.height = height;
    if (ctx) {
        ctx.imageSmoothingEnabled = true;
        ctx.drawImage(sourceCanvas, 0, 0, width, height);
        ctx.globalCompositeOperation = 'destination-in';
        ctx.beginPath();
        ctx.arc(width / 2, height / 2, Math.min(width, height) / 2, 0, 2 * Math.PI, true);
        ctx.fill();
    }
    return canvas;
}

export default {
    picture: ref<string | undefined>(),
    preview: ref(false),

    video(video: HTMLVideoElement | null) {
        hvideo = video
    },

    start() {
        navigator.mediaDevices
            .getUserMedia({
                video: true,
                audio: false,
            })
            .then((stream) => {
                videoStream = stream
                if (hvideo) hvideo.srcObject = stream
                hvideo?.play()
            })
            .catch((err) => {
                console.error(`An error occurred: ${err}`)
            })

        this.picture.value = undefined
        this.preview.value = false
    },

    stop() {
        videoStream?.getVideoTracks()[0].stop()
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
        cropper = new Cropper(canvas, { aspectRatio: 1 })
    },

    savePicture() {
        const croppedCanvas = cropper?.getCroppedCanvas()
        const roundedCanvas = getRoundedCanvas(croppedCanvas)
        this.picture.value = roundedCanvas?.toDataURL('image/png')
        cropper?.destroy()
    },
}
