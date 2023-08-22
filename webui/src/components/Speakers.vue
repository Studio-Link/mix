<template>
  <div class="mt-6">
    <div class="max-w-7xl mx-auto px-4 text-center sm:px-6 lg:px-8">
      <div class="space-y-8 sm:space-y-10">
        <ul
          class="mx-auto grid grid-cols-3 gap-x-4 gap-y-6 sm:grid-cols-3 md:grid-cols-3 md:gap-x-4 lg:max-w-5xl lg:gap-x-8 lg:gap-y-12 xl:grid-cols-6"
        >
          <li v-for="item in Users.speakers.value" :key="item.id">
            <div class="space-y-2">
              <div class="group">
                <div>
                  <picture class="inline-block relative">
                    <source type="image/webp" :srcset="'/avatars/' + item.id + '.webp'" />
                    <img
                      class="mx-auto h-20 w-20 rounded-full lg:w-24 lg:h-24"
                      :src="'/avatars/' + item.id + '.png'"
                      alt="Avatar Image"
                    />
                    <span v-if="!item.audio" class="absolute bottom-0 right-0 bg-indigo-600 rounded-lg text-gray-200">
                      <svg
                        aria-hidden="true"
                        focusable="false"
                        data-prefix="fas"
                        data-icon="microphone-slash"
                        class="h-5 w-5"
                        role="img"
                        xmlns="http://www.w3.org/2000/svg"
                        viewBox="0 0 640 512"
                      >
                        <path
                          fill="currentColor"
                          d="M383.1 464l-39.1-.0001v-33.77c20.6-2.824 39.98-9.402 57.69-18.72l-43.26-33.91c-14.66 4.65-30.28 7.179-46.68 6.144C245.7 379.6 191.1 317.1 191.1 250.9V247.2L143.1 209.5l.0001 38.61c0 89.65 63.97 169.6 151.1 181.7v34.15l-40 .0001c-17.67 0-31.1 14.33-31.1 31.1C223.1 504.8 231.2 512 239.1 512h159.1c8.838 0 15.1-7.164 15.1-15.1C415.1 478.3 401.7 464 383.1 464zM630.8 469.1l-159.3-124.9c15.37-25.94 24.53-55.91 24.53-88.21V216c0-13.25-10.75-24-23.1-24c-13.25 0-24 10.75-24 24l-.0001 39.1c0 21.12-5.559 40.77-14.77 58.24l-25.72-20.16c5.234-11.68 8.493-24.42 8.493-38.08l-.001-155.1c0-52.57-40.52-98.41-93.07-99.97c-54.37-1.617-98.93 41.95-98.93 95.95l0 54.25L38.81 5.111C34.41 1.673 29.19 0 24.03 0C16.91 0 9.839 3.158 5.12 9.189c-8.187 10.44-6.37 25.53 4.068 33.7l591.1 463.1c10.5 8.203 25.57 6.328 33.69-4.078C643.1 492.4 641.2 477.3 630.8 469.1z"
                        ></path>
                      </svg>
                    </span>
                    <span v-if="item.audio" class="absolute bottom-0 right-0 bg-gray-600 rounded-lg text-gray-200">
                      <svg
                        aria-hidden="true"
                        focusable="false"
                        data-prefix="fas"
                        data-icon="microphone"
                        class="h-5 w-5"
                        role="img"
                        xmlns="http://www.w3.org/2000/svg"
                        viewBox="0 0 384 512"
                      >
                        <path
                          fill="currentColor"
                          d="M192 352c53.03 0 96-42.97 96-96v-160c0-53.03-42.97-96-96-96s-96 42.97-96 96v160C96 309 138.1 352 192 352zM344 192C330.7 192 320 202.7 320 215.1V256c0 73.33-61.97 132.4-136.3 127.7c-66.08-4.169-119.7-66.59-119.7-132.8L64 215.1C64 202.7 53.25 192 40 192S16 202.7 16 215.1v32.15c0 89.66 63.97 169.6 152 181.7V464H128c-18.19 0-32.84 15.18-31.96 33.57C96.43 505.8 103.8 512 112 512h160c8.222 0 15.57-6.216 15.96-14.43C288.8 479.2 274.2 464 256 464h-40v-33.77C301.7 418.5 368 344.9 368 256V215.1C368 202.7 357.3 192 344 192z"
                        ></path>
                      </svg>
                    </span>
                    <span
                      v-if="!item.video"
                      class="absolute -bottom-0 left-0 bg-indigo-600 rounded-lg text-gray-200 p-0.5"
                    >
                      <svg class="h-4 w-4" aria-hidden="true" viewBox="0 0 640 512" xmlns="http://www.w3.org/2000/svg">
                        <path
                          fill="currentColor"
                          d="M630.8 469.1L38.81 5.113C34.41 1.676 29.19 .0042 24.03 .0042c-7.125 0-14.19 3.156-18.91 9.187c-8.187 10.44-6.37 25.53 4.068 33.7l591.1 463.1c10.5 8.203 25.57 6.328 33.69-4.078C643.1 492.4 641.3 477.3 630.8 469.1z"
                        />
                        <path
                          fill="currentColor"
                          d="M319.9 63.1c88.37 0 159.1 71.62 159.1 159.1c0 37.68-13.4 71.92-35.27 99.16l50.48 39.57c30.42-38.12 48.92-86.1 48.92-138.7c0-123.7-100.4-223.1-223.1-223.1c-72.02 0-135.7 34.24-176.6 87.04l50.51 39.59C223 88.71 268.3 63.1 319.9 63.1zM319.9 175.1c-15.75 0-29.04 8.086-37.72 19.84l137.3 107.6c17.62-21.78 28.5-49.17 28.5-79.41c0-70.62-57.29-127.1-127.9-127.1c-41.28 0-77.63 19.93-100.9 50.38l37.43 29.34c14.55-19.23 37.47-31.72 63.42-31.72c8.875 0 15.96 7.124 15.96 15.1C335.9 168.9 328.7 175.1 319.9 175.1zM319.1 383.1c-88.37 0-160.1-71.63-160.1-159.1c0-.6406 .1816-1.234 .1895-1.871L101.3 176c-3.373 15.46-5.267 31.48-5.267 47.96c0 76.25 38.12 143.4 96.12 183.9l-49.12 30.75c-9.375 5.875-14.1 16.12-14.1 27.12v14.25c0 17.62 14.48 32 32.11 32l319.9-.0053c12.21 0 22.52-7.137 27.91-17.23l-148.4-116.3C346.8 381.7 333.7 383.1 319.1 383.1z"
                        />
                      </svg>
                    </span>
                    <span v-if="item.video" class="absolute bottom-0 left-0 bg-gray-600 rounded-lg text-gray-200 p-0.5">
                      <svg class="h-4 w-4" aria-hidden="true" viewBox="0 0 448 512" xmlns="http://www.w3.org/2000/svg">
                        <path
                          fill="currentColor"
                          d="M400.9 438.6l-49.12-30.75C409.9 367.4 448 300.3 448 224c0-123.8-100.4-224-224-224c-123.8 0-224 100.3-224 224c0 76.25 38.13 143.4 96.13 183.9L47 438.6C37.63 444.5 32 454.8 32 465.8v14.25c0 17.62 14.48 31.1 32.11 31.1L384 512c17.62 0 32-14.38 32-32V465.8C416 454.8 410.3 444.5 400.9 438.6zM224 384c-88.37 0-160.1-71.62-160.1-159.1s71.63-160 159.1-160s160 71.62 160 160S312.3 384 224 384z"
                        />
                        <path
                          fill="currentColor"
                          d="m224 96c-70.6 0-128.08 57.3-128.08 128s57.38 128 128 128c70.75 0 128-57.25 128-128-0.02-70.6-57.32-128-127.92-128zm-0.1 80c-26.5 0-47.88 21.5-48 48 0 8.875-7.125 16-16 16-8.75 0-16-7.125-16-16 0.125-44.13 35.92-80 80.04-80 8.875 0 15.96 7.125 15.96 16s-7.1 16-16 16z"
                        />
                      </svg>
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
                  @click="api.listener(item.id)"
                  type="button"
                  class="hidden group-hover:inline-flex items-center rounded-md border border-transparent bg-indigo-600 px-2.5 py-1.5 text-xs font-bold text-white shadow-sm hover:bg-indigo-700 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
                >
                  <SpeakerWaveIcon class="-ml-0.5 mr-1 h-4 w-4" aria-hidden="true" />
                  To Audience
                </button>
                <button
                  v-if="!Users.host_status.value && item.id === api.session().user_id && room?.show"
                  @click="api.listener(item.id)"
                  type="button"
                  class="inline-flex items-center rounded-md border border-transparent bg-indigo-600 px-2.5 py-1.5 text-xs font-bold text-white shadow-sm hover:bg-indigo-700 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
                >
                  <SpeakerWaveIcon class="-ml-0.5 mr-1 h-4 w-4" aria-hidden="true" />
                  Leave Stage
                </button>
                <div class="space-y-2">
                  <div class="text-base font-medium">
                    <h3>{{ item.name }}</h3>
                  </div>
                </div>
              </div>
            </div>
          </li>
        </ul>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { Users } from '../ws/users'
import api from '../api'
import { SpeakerWaveIcon } from '@heroicons/vue/24/outline'

const room = Users.room
</script>
