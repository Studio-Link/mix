import { ref } from 'vue'
import router from './router'

export const Error = {
    text: ref(''),
    video: ref(''),
    audio: ref(false),

    fatal(msg: string) {
        this.text.value = msg
        router.push({ name: 'FatalError' })
    },

    error(msg: string) {
        this.text.value = msg
    },

    errorVideo(msg: string) {
        this.video.value = msg
    },

    errorAudio(enable: boolean) {
        this.audio.value = enable
    },

    reset() {
        this.text.value = ''
        this.video.value = ''
    },
}
