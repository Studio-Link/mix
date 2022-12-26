import { createApp } from 'vue'
import './index.css'
import App from './App.vue'
import router from './router'
import './console'

const app = createApp(App)

app.use(router)

app.mount('#app')
