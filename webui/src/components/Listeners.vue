<template>
  <div class="mt-2">
    <div class="max-w-7xl mx-auto px-4 text-center sm:px-6 lg:px-8 border-t pt-2">
        <div class="space-y-5 sm:mx-auto sm:max-w-xl sm:space-y-4 lg:max-w-5xl">
          <h2 v-if="Users.listeners.value?.length" class="text-xl font-medium tracking-tight">Audience</h2>
        </div>
        <ul
          class="mx-auto grid grid-cols-4 gap-x-4 gap-y-6 sm:grid-cols-4 md:gap-x-5 lg:max-w-5xl lg:gap-x-8 lg:gap-y-12 xl:grid-cols-6"
        >
          <li v-for="item in Users.listeners.value" :key="item.id">
            <div class="space-y-4">
              <div class="group">
                <div>
                  <picture class="inline-block relative">
                    <source type="image/webp" :srcset="'/avatars/' + item.id + '.webp'" />
                    <img
                      class="mx-auto h-16 w-16 rounded-full lg:w-20 lg:h-20 hover:ring-2 ring-gray-500"
                      :src="'/avatars/' + item.id + '.png'"
                      alt="Avatar Image"
                    />
                    <span
                      v-if="item.webrtc"
                      title="Listening"
                      class="absolute bottom-0 right-0 bg-green-700 rounded-lg text-gray-200 p-0.5"
                    >
                      <SpeakerWaveIcon class="h-4 w-4" />
                    </span>
                    <span
                      v-if="!item.webrtc"
                      title="Not listening"
                      class="absolute bottom-0 right-0 bg-red-700 rounded-lg text-gray-200 p-0.5"
                    >
                      <SpeakerXMarkIcon class="h-4 w-4" />
                    </span>
                    <span v-show="item.hand" class="absolute -top-1 -right-1 bg-indigo-600 rounded-lg text-gray-200">
                      <svg
                        class="h-6 w-6"
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
                    </span>
                  </picture>
                </div>
                <button
                  v-if="Users.host_status.value"
                  @click="api.speaker(item.id)"
                  type="button"
                  class="hidden group-hover:inline-flex items-center rounded-md border border-transparent bg-indigo-600 px-2.5 py-1.5 text-xs font-bold text-white shadow-sm hover:bg-indigo-700 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
                >
                  <MicrophoneIcon class="-ml-0.5 mr-1 h-4 w-4" aria-hidden="true" />On Stage
                </button>
                <div class="space-y-2">
                  <div class="text-sm font-medium">
                    <h3>{{ item.name }}</h3>
                  </div>
                </div>
              </div>
            </div>
          </li>
        </ul>
    </div>
  </div>
</template>

<script setup lang="ts">
import { Users } from '../ws/users'
import api from '../api'
import { SpeakerWaveIcon, SpeakerXMarkIcon, MicrophoneIcon } from '@heroicons/vue/24/outline'
</script>
