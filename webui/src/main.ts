import { createApp } from 'vue'
import './index.css'
import App from './App.vue'
import router from './router'
import adapter from 'webrtc-adapter'

console.log('browser: ', adapter.browserDetails.browser, adapter.browserDetails.version)

const app = createApp(App)

app.use(router)

app.mount('#app')
