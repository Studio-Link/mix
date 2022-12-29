<template>
  <div class="fixed left-0 top-0 m-3 space-x-2 text-xs">
    <button
      :class="{
        'bg-red-700 hover:bg-red-600': !Users.record.value,
        'bg-gray-600 hover:bg-gray-500': Users.record.value,
        hidden: !Users.record.value && !api.is_host(),
      }"
      class="inline-flex items-center rounded border border-transparent px-2.5 py-1.5 font-semibold text-white focus:outline-none focus:ring-2 focus:ring-gray-500 focus:ring-offset-2 uppercase"
      @click="api.record_switch()"
      :disabled="!api.is_host()"
    >
      <span v-if="Users.record.value" class="font-mono">REC {{ Users.record_timer.value }}</span>
      <span v-if="!Users.record.value" class="font-mono">Record</span>
    </button>
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
import { Users } from '../ws/users'
import api from '../api'
import { onMounted } from 'vue'

onMounted(() => {
  api.websocket()
})
</script>
