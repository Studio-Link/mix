<template>
  <div class="fixed right-0 top-0 m-3 space-x-2 text-xs z-10">
    <RecButton />
  </div>
  <div class="flex">
    <main class="w-full pb-24">
      <WebrtcVideo />
      <audio id="live" autoplay></audio>
      <Speakers />
      <Listeners />
    </main>
    <Chat />
  </div>
  <BottomActions />
</template>

<script setup lang="ts">
import WebrtcVideo from '../components/WebrtcVideo.vue'
import Speakers from '../components/Speakers.vue'
import Listeners from '../components/Listeners.vue'
import BottomActions from '../components/BottomActions.vue'
import Chat from '../components/Chat.vue'
import RecButton from '../components/RecButton.vue'
import api from '../api'
import { onMounted } from 'vue'

onMounted(() => {
  api.websocket()
})
</script>
