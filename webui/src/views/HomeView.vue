<template>
  <div class="flex">
    <StudioNav />
    <div class="container mx-auto mb-32 sm:mb-24">
      <div class="flex items-center fixed right-0 top-0 m-3 space-x-2 text-xs z-40">
        <RecButton />
        <div>
          <button
            title="Chat"
            :class="[Users.chat_active.value ? 'text-gray-400 hover:text-gray-600' : 'fadeout text-gray-100 bg-gray-600 hover:text-white']"
            class="group block px-1 py-1 text-base font-medium rounded-md"
            @click="chat_clicked()"
          >
            <ChatBubbleOvalLeftEllipsisIcon v-if="!Users.chat_active.value" class="h-8 w-8 mx-auto" />
            <XMarkIcon v-if="Users.chat_active.value" class="h-8 w-8 mx-auto" />
          </button>
        </div>
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
    </div>
  </div>
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
import StudioNav from '../components/StudioNav.vue'
import { Users } from '../ws/users'
import { ChatBubbleOvalLeftEllipsisIcon, XMarkIcon } from '@heroicons/vue/24/outline'

onMounted(() => {
  api.websocket()
})
function chat_clicked() {
  Users.chat_active.value = !Users.chat_active.value
}
</script>
