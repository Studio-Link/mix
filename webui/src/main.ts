import { createApp } from 'vue'
import './index.css'
import App from './App.vue'
import router from './router'
import ButtonPrimary from "./components/ButtonPrimary.vue";
import ButtonSecondary from "./components/ButtonSecondary.vue";
import './shortcuts'
import "typeface-roboto-mono";

const app = createApp(App)

app.component("ButtonPrimary", ButtonPrimary);
app.component("ButtonSecondary", ButtonSecondary);

app.use(router)

app.mount('#app')
