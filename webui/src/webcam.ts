import Cropper from 'cropperjs'
import 'cropperjs/dist/cropper.css'
import { ref } from 'vue'

let videoStream: MediaStream | undefined
let cropper: Cropper | undefined
let hvideo: HTMLVideoElement | null

export default {
    picture: ref<string | undefined>(),
    preview: ref(false),

    video(video: HTMLVideoElement | null) {
        hvideo = video
    },

    start() {
        navigator.mediaDevices
            .getUserMedia({
                video: {
                    width: { max: 640 } /* Workaround for big images */
                }, audio: false
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
        this.preview.value = true
        const context = canvas?.getContext('2d')
        const width = video?.videoWidth
        const height = video?.videoHeight

        canvas?.setAttribute('width', String(width))
        canvas?.setAttribute('height', String(height))

        context?.drawImage(video!, 0, 0, width!, height!)
        cropper = new Cropper(canvas!, { aspectRatio: 1 })
    },

    savePicture() {
        this.picture.value = cropper?.getCroppedCanvas().toDataURL('image/png')
        cropper?.destroy()
    },
}
