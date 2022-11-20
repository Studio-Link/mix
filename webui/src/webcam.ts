import Cropper from 'cropperjs'
import 'cropperjs/dist/cropper.css'
import {ref} from 'vue'

let videoStream: MediaStream | undefined
let cropper:Cropper | undefined

export default {
    picture: ref<string | undefined>(),
    start(video: HTMLVideoElement | null) {
        navigator.mediaDevices
            .getUserMedia({ video: true, audio: false })
            .then((stream) => {
                videoStream = stream
                if (video) video.srcObject = stream
                video?.play()
            })
            .catch((err) => {
                console.error(`An error occurred: ${err}`)
            })

            this.picture.value = undefined
    },

    stop() {
        videoStream?.getVideoTracks()[0].stop()
    },

    takePicture(canvas: HTMLCanvasElement | null, video: HTMLVideoElement | null) {
        const context = canvas?.getContext('2d')
        const width = video?.videoWidth
        const height = video?.videoHeight

        canvas?.setAttribute('width', String(width))
        canvas?.setAttribute('height', String(height))

        context?.drawImage(video!, 0, 0, width!, height!)

        // pictureData = canvas?.toDataURL('image/png')

        cropper = new Cropper(canvas!, {aspectRatio: 1})
    },

    savePicture() {
        this.picture.value = cropper?.getCroppedCanvas().toDataURL('image/png')
        cropper?.destroy()
    },
}
