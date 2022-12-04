<template>
  <div class="flex">
    <main class="w-full pb-24">
      <video id="live" class="mx-auto px-4 mt-2" width="426" height="240" playsinline autoplay muted></video>
      <audio id="live" autoplay></audio>
      <Speakers />
      <Listeners />
    </main>
    <Chat />
  </div>
  <BottomActions v-if="!Webrtc.errorText.value" />
</template>

<script setup lang="ts">
import Speakers from '../components/Speakers.vue'
import Listeners from '../components/Listeners.vue'
import BottomActions from '../components/BottomActions.vue'
import Chat from '../components/Chat.vue'
import { Webrtc } from '../webrtc'
import api from '../api'
import { onMounted } from 'vue'

onMounted(() => {
  api.websocket()
})
</script>
