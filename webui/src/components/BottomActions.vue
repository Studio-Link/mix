<template>
  <div class="fixed inset-x-0 bottom-0">
    <div class="bg-gray-600">
      <div class="mx-auto py-1 px-3 sm:px-6 lg:px-8">
        <div class="flex items-center flex-wrap justify-center space-x-16">
          <div v-if="Webrtc.state.value >= WebrtcState.Listening">
            <button
              ref="hand"
              :class="{ 'animate-pulse': hand_status }"
              class="text-gray-300 hover:bg-gray-700 hover:text-white group block px-2 py-2 text-base font-medium rounded-md"
              @click="hand_clicked()"
            >
              <HandRaisedIcon class="h-9 w-9 mx-auto" />
            </button>
          </div>
          <div v-if="Webrtc.state.value >= WebrtcState.Listening">
            <button
              ref="settings"
              @click="settings_clicked()"
              class="text-gray-300 hover:bg-gray-700 hover:text-white group block px-2 py-2 text-base font-medium rounded-md"
            >
              <Cog6ToothIcon class="h-10 w-10" />
            </button>
          </div>
          <div>
            <button
              @click="logout_clicked()"
              ref="logout"
              title="Logout"
              class="text-gray-300 hover:bg-gray-700 hover:text-white group block px-2 py-2 text-base font-medium rounded-md"
            >
              <ArrowLeftOnRectangleIcon class="h-9 w-9 mx-auto" />
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
              <PlayCircleIcon class="h-14 w-14 mx-auto" />
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
              <ChatBubbleOvalLeftEllipsisIcon class="h-9 w-9 mx-auto" />
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
import api from '../api'
import SettingsModal from '../components/SettingsModal.vue'
import {
  HandRaisedIcon,
  Cog6ToothIcon,
  ChatBubbleOvalLeftEllipsisIcon,
  ArrowLeftOnRectangleIcon,
} from '@heroicons/vue/24/outline'
import { PlayCircleIcon } from '@heroicons/vue/24/solid'

const hand = ref<HTMLButtonElement>()
const play = ref<HTMLButtonElement>()
const chat = ref<HTMLButtonElement>()
const settings = ref<HTMLButtonElement>()
const hand_status = ref(false)

function listen() {
  Webrtc.listen()
}

function hand_clicked() {
  hand_status.value = !hand_status.value
  api.hand(hand_status.value)
}

async function settings_clicked() {
  Users.settings_active.value = true
}

function chat_clicked() {
  Users.chat_active.value = !Users.chat_active.value
}

function logout_clicked() {
  api.logout()
}
</script>
