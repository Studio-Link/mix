import { ref } from 'vue'
import router from './router'

export const Error = {
    text: ref(''),

    fatal(msg: string) {
        this.text.value = msg
        router.push({ name: 'FatalError' })
    },

    error(msg: string) {
        this.text.value = msg
    },

    reset() {
        this.text.value = ''
    },
}
