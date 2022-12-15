<template>
  <div class="fixed inset-x-0 bottom-0">
    <div class="bg-gray-600">
      <div class="mx-auto py-1 px-3 sm:px-6 lg:px-8">
        <div class="flex items-center justify-between">
          <!-- Logout button -->
          <div>
            <button
              ref="logout"
              title="Logout"
              class="text-gray-300 hover:bg-gray-700 hover:text-white group block px-2 py-2 text-base font-medium rounded-md"
              @click="logout_clicked()"
            >
              <ArrowLeftOnRectangleIcon class="h-8 w-8 mx-auto" />
            </button>
          </div>
          <!-- Button group centered -->
          <div class="flex space-x-1 sm:space-x-4">
            <!-- Hand button -->
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
            <!-- Audio Mute -->
            <div v-if="Webrtc.state.value >= WebrtcState.Listening">
              <button
                v-if="Webrtc.audio_muted.value"
                class="text-gray-300 hover:bg-gray-700 hover:text-white group block px-2 py-2 text-base font-medium rounded-md"
                @click="mic_clicked(false)"
              >
                <svg
                  aria-hidden="true"
                  focusable="false"
                  data-prefix="fas"
                  data-icon="microphone-slash"
                  class="h-10 w-10 mx-auto"
                  role="img"
                  xmlns="http://www.w3.org/2000/svg"
                  viewBox="0 0 640 512"
                >
                  <path
                    fill="currentColor"
                    d="M383.1 464l-39.1-.0001v-33.77c20.6-2.824 39.98-9.402 57.69-18.72l-43.26-33.91c-14.66 4.65-30.28 7.179-46.68 6.144C245.7 379.6 191.1 317.1 191.1 250.9V247.2L143.1 209.5l.0001 38.61c0 89.65 63.97 169.6 151.1 181.7v34.15l-40 .0001c-17.67 0-31.1 14.33-31.1 31.1C223.1 504.8 231.2 512 239.1 512h159.1c8.838 0 15.1-7.164 15.1-15.1C415.1 478.3 401.7 464 383.1 464zM630.8 469.1l-159.3-124.9c15.37-25.94 24.53-55.91 24.53-88.21V216c0-13.25-10.75-24-23.1-24c-13.25 0-24 10.75-24 24l-.0001 39.1c0 21.12-5.559 40.77-14.77 58.24l-25.72-20.16c5.234-11.68 8.493-24.42 8.493-38.08l-.001-155.1c0-52.57-40.52-98.41-93.07-99.97c-54.37-1.617-98.93 41.95-98.93 95.95l0 54.25L38.81 5.111C34.41 1.673 29.19 0 24.03 0C16.91 0 9.839 3.158 5.12 9.189c-8.187 10.44-6.37 25.53 4.068 33.7l591.1 463.1c10.5 8.203 25.57 6.328 33.69-4.078C643.1 492.4 641.2 477.3 630.8 469.1z"
                  ></path>
                </svg>
                <span class="sr-only">Microphone muted</span>
              </button>
              <button
                v-if="!Webrtc.audio_muted.value"
                class="text-gray-300 hover:bg-gray-700 hover:text-white group block px-2 py-2 text-base font-medium rounded-md"
                @click="mic_clicked(true)"
              >
                <svg
                  aria-hidden="true"
                  focusable="false"
                  data-prefix="fas"
                  data-icon="microphone"
                  class="h-10 w-10 mx-auto"
                  role="img"
                  xmlns="http://www.w3.org/2000/svg"
                  viewBox="0 0 384 512"
                >
                  <path
                    fill="currentColor"
                    d="M192 352c53.03 0 96-42.97 96-96v-160c0-53.03-42.97-96-96-96s-96 42.97-96 96v160C96 309 138.1 352 192 352zM344 192C330.7 192 320 202.7 320 215.1V256c0 73.33-61.97 132.4-136.3 127.7c-66.08-4.169-119.7-66.59-119.7-132.8L64 215.1C64 202.7 53.25 192 40 192S16 202.7 16 215.1v32.15c0 89.66 63.97 169.6 152 181.7V464H128c-18.19 0-32.84 15.18-31.96 33.57C96.43 505.8 103.8 512 112 512h160c8.222 0 15.57-6.216 15.96-14.43C288.8 479.2 274.2 464 256 464h-40v-33.77C301.7 418.5 368 344.9 368 256V215.1C368 202.7 357.3 192 344 192z"
                  ></path>
                </svg>
                <span class="sr-only">Microphone enabled</span>
              </button>
            </div>
            <!-- Video Mute -->
            <div v-if="Webrtc.state.value >= WebrtcState.Listening">
              <button
                v-if="Webrtc.video_muted.value"
                class="text-gray-300 hover:bg-gray-700 hover:text-white group block px-2 py-2 text-base font-medium rounded-md"
                @click="video_clicked(false)"
              >
                <VideoCameraSlashIcon class="h-9 w-9" />
                <span class="sr-only">Video disabled</span>
              </button>
              <button
                v-if="!Webrtc.video_muted.value"
                class="text-gray-300 hover:bg-gray-700 hover:text-white group block px-2 py-2 text-base font-medium rounded-md"
                @click="video_clicked(true)"
              >
                <VideoCameraIcon class="h-9 w-9" />
                <span class="sr-only">Video enabled</span>
              </button>
            </div>
            <div v-if="Webrtc.state.value >= WebrtcState.ReadySpeaking">
              <button
                ref="settings"
                class="text-gray-300 hover:bg-gray-700 hover:text-white group block px-2 py-2 text-base font-medium rounded-md"
                @click="settings_clicked()"
              >
                <Cog6ToothIcon class="h-10 w-10" />
              </button>
            </div>
            <!-- Play button -->
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
          </div>
          <!-- Chat button -->
          <div>
            <button
              ref="chat"
              title="Chat"
              class="text-gray-300 hover:bg-gray-700 hover:text-white group block px-2 py-2 text-base font-medium rounded-md"
              @click="chat_clicked()"
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
import { Users } from '../ws/users'
import api from '../api'
import SettingsModal from '../components/SettingsModal.vue'
import {
  HandRaisedIcon,
  Cog6ToothIcon,
  ChatBubbleOvalLeftEllipsisIcon,
  ArrowLeftOnRectangleIcon,
  VideoCameraIcon,
  VideoCameraSlashIcon,
} from '@heroicons/vue/24/outline'
import { PlayCircleIcon } from '@heroicons/vue/24/solid'

const hand_status = Users.hand_status

function listen() {
  Webrtc.listen()
  if (Users.speaker_status.value) Users.settings_active.value = true
}

function hand_clicked() {
  hand_status.value = !hand_status.value
  api.hand(hand_status.value)
}

function mic_clicked(mute: boolean) {
  if (Webrtc.state.value < WebrtcState.ReadySpeaking) {
    Users.settings_active.value = true
  } else Webrtc.audio_mute(mute)
}

function video_clicked(mute: boolean) {
  if (Webrtc.state.value < WebrtcState.ReadySpeaking || Webrtc.video_input_id.value === 'disabled') {
    Users.settings_active.value = true
  } else Webrtc.video_mute(mute)
}

async function settings_clicked() {
  Users.settings_active.value = true
}

function chat_clicked() {
  Users.chat_active.value = !Users.chat_active.value
}

function logout_clicked() {
  api.logout(true)
}
</script>
