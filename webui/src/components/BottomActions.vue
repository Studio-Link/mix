<template>
  <div class="fixed inset-x-0 bottom-0">
    <div class="bg-gray-600">
      <div class="mx-auto py-1 px-3 sm:px-6 lg:px-8">
        <div class="flex items-center flex-wrap justify-center space-x-16">
          <div v-if="Webrtc.state.value >= WebrtcState.Listening">
            <button
              ref="hand"
              class="text-gray-300 hover:bg-gray-700 hover:text-white group block px-2 py-2 text-base font-medium rounded-md"
              @click="hand_clicked()"
            >
              <svg
                class="h-9 w-9 mx-auto"
                xmlns="http://www.w3.org/2000/svg"
                fill="none"
                viewBox="0 0 24 24"
                stroke="currentColor"
              >
                <path
                  stroke-linecap="round"
                  stroke-linejoin="round"
                  stroke-width="2"
                  d="M7 11.5V14m0-2.5v-6a1.5 1.5 0 113 0m-3 6a1.5 1.5 0 00-3 0v2a7.5 7.5 0 0015 0v-5a1.5 1.5 0 00-3 0m-6-3V11m0-5.5v-1a1.5 1.5 0 013 0v1m0 0V11m0-5.5a1.5 1.5 0 013 0v3m0 0V11"
                />
              </svg>
              <!-- <div class="text-gray-300 text-sm text-center">Hand</div> -->
            </button>
          </div>
          <div v-if="Webrtc.state.value >= WebrtcState.Listening">
            <button
              ref="settings"
              @click="settings_clicked()"
              class="text-gray-300 hover:bg-gray-700 hover:text-white group block px-2 py-2 text-base font-medium rounded-md"
            >
              <svg
                class="h-10 w-10"
                xmlns="http://www.w3.org/2000/svg"
                fill="none"
                viewBox="0 0 24 24"
                stroke-width="1.5"
                stroke="currentColor"
              >
                <path
                  stroke-linecap="round"
                  stroke-linejoin="round"
                  d="M9.594 3.94c.09-.542.56-.94 1.11-.94h2.593c.55 0 1.02.398 1.11.94l.213 1.281c.063.374.313.686.645.87.074.04.147.083.22.127.324.196.72.257 1.075.124l1.217-.456a1.125 1.125 0 011.37.49l1.296 2.247a1.125 1.125 0 01-.26 1.431l-1.003.827c-.293.24-.438.613-.431.992a6.759 6.759 0 010 .255c-.007.378.138.75.43.99l1.005.828c.424.35.534.954.26 1.43l-1.298 2.247a1.125 1.125 0 01-1.369.491l-1.217-.456c-.355-.133-.75-.072-1.076.124a6.57 6.57 0 01-.22.128c-.331.183-.581.495-.644.869l-.213 1.28c-.09.543-.56.941-1.11.941h-2.594c-.55 0-1.02-.398-1.11-.94l-.213-1.281c-.062-.374-.312-.686-.644-.87a6.52 6.52 0 01-.22-.127c-.325-.196-.72-.257-1.076-.124l-1.217.456a1.125 1.125 0 01-1.369-.49l-1.297-2.247a1.125 1.125 0 01.26-1.431l1.004-.827c.292-.24.437-.613.43-.992a6.932 6.932 0 010-.255c.007-.378-.138-.75-.43-.99l-1.004-.828a1.125 1.125 0 01-.26-1.43l1.297-2.247a1.125 1.125 0 011.37-.491l1.216.456c.356.133.751.072 1.076-.124.072-.044.146-.087.22-.128.332-.183.582-.495.644-.869l.214-1.281z"
                />
                <path stroke-linecap="round" stroke-linejoin="round" d="M15 12a3 3 0 11-6 0 3 3 0 016 0z" />
              </svg>
            </button>
          </div>
          <div>
            <button
              v-if="Webrtc.state.value < WebrtcState.Listening"
              ref="play"
              class="text-gray-300 hover:bg-gray-700 hover:text-white group items-center px-2 py-2 text-base font-medium rounded-md block"
              title="Join as listener"
              :class="{ 'animate-pulse': Webrtc.state.value == WebrtcState.Connecting }"
              @click="listen()"
            >
              <svg class="h-14 w-14 mx-auto" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 20 20" fill="currentColor">
                <path
                  fill-rule="evenodd"
                  d="M10 18a8 8 0 100-16 8 8 0 000 16zM9.555 7.168A1 1 0 008 8v4a1 1 0 001.555.832l3-2a1 1 0 000-1.664l-3-2z"
                  clip-rule="evenodd"
                />
              </svg>
              <div v-if="Webrtc.state.value == WebrtcState.Offline" class="text-gray-300 text-sm text-center">
                Press to listen
              </div>
              <div v-if="Webrtc.state.value == WebrtcState.Connecting" class="text-gray-300 text-sm text-center">
                Connecting...
              </div>
            </button>
          </div>
          <div>
            <button
              @click="chat_clicked()"
              ref="chat"
              title="Chat"
              class="text-gray-300 hover:bg-gray-700 hover:text-white group block px-2 py-2 text-base font-medium rounded-md"
            >
              <svg
                class="h-9 w-9 mx-auto"
                xmlns="http://www.w3.org/2000/svg"
                fill="none"
                viewBox="0 0 24 24"
                stroke="currentColor"
              >
                <path
                  stroke-linecap="round"
                  stroke-linejoin="round"
                  stroke-width="2"
                  d="M8 12h.01M12 12h.01M16 12h.01M21 12c0 4.418-4.03 8-9 8a9.863 9.863 0 01-4.255-.949L3 20l1.395-3.72C3.512 15.042 3 13.574 3 12c0-4.418 4.03-8 9-8s9 3.582 9 8z"
                />
              </svg>
              <!-- <div class="text-gray-300 text-sm text-center">Chat</div> -->
            </button>
          </div>
        </div>
      </div>
    </div>
    <SettingsModal />
  </div>
</template>

<script setup lang="ts">
import { Webrtc, WebrtcState } from '../webrtc'
import { ref } from 'vue'
import { Users } from '../ws/users'
import SettingsModal from '../components/SettingsModal.vue'

const hand = ref<HTMLButtonElement>()
const play = ref<HTMLButtonElement>()
const chat = ref<HTMLButtonElement>()
const settings = ref<HTMLButtonElement>()

function listen() {
  Webrtc.listen()
}

function hand_clicked() {
  /* Webrtc.speak() */
}

async function settings_clicked() {
  Users.settings_active.value = true
}

function chat_clicked() {
  Users.chat_active.value = !Users.chat_active.value
}
</script>
