<template>
  <div class="flex">
    <div v-show="Webrtc.state.value >= WebrtcState.Listening" class="w-1/3 mr-2 hidden">
      <div v-for="item in sources" class="my-1">
        <video :id="'source' + item.rtc?.id" playsinline autoplay muted preload="none"></video>
        <div>
          {{ item.dev }}
          <button
            @click="solo_source(item.dev)"
            :class="{ 'bg-red-600 hover:bg-red-500': item.solo }"
            class="rounded-sm bg-indigo-600 px-2 py-1 text-xs font-semibold text-white shadow-xs hover:bg-indigo-500 focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-indigo-600"
          >
            Solo
          </button>
        </div>
      </div>
    </div>

    <div
      ref="overlay_div"
      v-show="!PiP && Webrtc.state.value >= WebrtcState.Listening"
      @mouseover="nav = true"
      @mouseleave="nav = false"
      @touchstart.passive="nav = true"
      class="relative flex bg-black mx-auto"
      :class="[isFullscreen ? 'w-full' : 'max-w-(--breakpoint-xl) mt-8 lg:mt-0']"
    >
      <div v-show="nav" class="absolute z-30 text-gray-200 bottom-0 right-0 px-2">
        <button class="mr-4" @click="enable_stats()" type="button">
          <WifiIcon class="h-8 w-8" aria-hidden="true" />
        </button>
        <button v-if="hasFullscreen" @click="requestFullscreen(overlay_div)" class="text-lg" title="Fullscreen">
          <svg
            aria-hidden="true"
            focusable="false"
            data-prefix="fas"
            data-icon="expand"
            class="h-8 w-8"
            role="img"
            xmlns="http://www.w3.org/2000/svg"
            viewBox="0 0 448 512"
          >
            <path
              fill="currentColor"
              d="M128 32H32C14.31 32 0 46.31 0 64v96c0 17.69 14.31 32 32 32s32-14.31 32-32V96h64c17.69 0 32-14.31 32-32S145.7 32 128 32zM416 32h-96c-17.69 0-32 14.31-32 32s14.31 32 32 32h64v64c0 17.69 14.31 32 32 32s32-14.31 32-32V64C448 46.31 433.7 32 416 32zM128 416H64v-64c0-17.69-14.31-32-32-32s-32 14.31-32 32v96c0 17.69 14.31 32 32 32h96c17.69 0 32-14.31 32-32S145.7 416 128 416zM416 320c-17.69 0-32 14.31-32 32v64h-64c-17.69 0-32 14.31-32 32s14.31 32 32 32h96c17.69 0 32-14.31 32-32v-96C448 334.3 433.7 320 416 320z"
            ></path>
          </svg>
        </button>
      </div>

      <div
        v-show="!Webrtc.video_muted.value && selfview"
        v-for="(item, index) in vspeakers"
        class="absolute z-10 group"
        :key="item.pidx"
        :style="{ width: calc_width(), height: calc_height(), left: calc_left(index), top: calc_top(index) }"
      >
        <video
          v-if="item.id == api.user_id()"
          :class="{'scale-x-[-1]': Webrtc.video_select.value !== 'Screen'}"
          class="object-cover h-full w-full"
          id="selfview"
          playsinline
          autoplay
          muted
          preload="none"
        ></video>
      </div>

      <div
        v-if="overlay"
        v-for="(item, index) in vspeakers"
        :class="{ 'border-2': item.talk }"
        class="absolute z-20 border-green-500 border-0 group"
        :key="item.pidx"
        :style="{ width: calc_width(), height: calc_height(), left: calc_left(index), top: calc_top(index) }"
      >
        <div v-if="stats" class="bg-black opacity-60 text-gray-50 w-min text-xs">
          <pre class="pt-2 px-2">
Video RTT: {{ item.stats.vrtt }} ms
Audio RTT: {{ item.stats.artt }} ms
        </pre
          >
        </div>
        <div
          class="flex items-center absolute bottom-0 md:bg-gray-600 text-gray-100 rounded-md ml-1 mb-1 py-0.5 px-2 text-sm"
        >
          {{ item.name }}
          <span v-if="!item.audio" class="bg-red-700 rounded-lg text-gray-200 ml-2">
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
          <span v-if="item.audio" class="bg-gray-600 rounded-lg text-gray-200 ml-2">
            <svg
              aria-hidden="true"
              focusable="false"
              data-prefix="fas"
              data-icon="microphone"
              class="h-4 w-4"
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
          <span v-show="item.hand" class="ml-2 bg-indigo-600 rounded-lg text-gray-200">
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

          <button
            v-if="State.user.value.host"
            @click="api.listener(item.id)"
            type="button"
            class="hidden group-hover:inline-flex ml-2 items-center rounded-md border border-transparent bg-indigo-600 px-2.5 py-1.5 text-xs font-bold text-white shadow-xs hover:bg-indigo-700 focus:outline-hidden focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
          >
            <SpeakerWaveIcon class="-ml-0.5 mr-1 h-4 w-4" aria-hidden="true" />
            To Audience
          </button>
          <button
            v-if="!State.user.value.host && item.id === api.user_id() && room?.show"
            @click="api.listener(item.id)"
            type="button"
            class="hidden group-hover:inline-flex ml-2 items-center rounded-md border border-transparent bg-indigo-600 px-2.5 py-1.5 text-xs font-bold text-white shadow-xs hover:bg-indigo-700 focus:outline-hidden focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
          >
            <SpeakerWaveIcon class="-ml-0.5 mr-1 h-4 w-4" aria-hidden="true" />
            Leave Stage
          </button>
          <button
            v-if="State.user.value.host && !item.solo"
            @click="api.video_solo(item.id, true)"
            type="button"
            class="hidden group-hover:inline-flex ml-2 items-center rounded-md border border-transparent bg-indigo-600 px-2.5 py-1.5 text-xs font-bold text-white shadow-xs hover:bg-indigo-700 focus:outline-hidden focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
          >
            <PresentationChartBarIcon class="-ml-0.5 mr-1 h-4 w-4" aria-hidden="true" />
            Solo
          </button>
          <button
            v-if="State.user.value.host && item.solo"
            @click="api.video_solo(item.id, false)"
            type="button"
            class="hidden group-hover:inline-flex ml-2 items-center rounded-md border border-transparent bg-indigo-600 px-2.5 py-1.5 text-xs font-bold text-white shadow-xs hover:bg-indigo-700 focus:outline-hidden focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
          >
            <PresentationChartBarIcon class="-ml-0.5 mr-1 h-4 w-4" aria-hidden="true" />
            Disable Solo
          </button>
          <button
            v-if="State.user.value.host && !item.audio"
            @click="api.amix(item.id, true)"
            type="button"
            class="hidden group-hover:inline-flex ml-2 items-center rounded-md border border-transparent bg-indigo-600 px-2.5 py-1.5 text-xs font-bold text-white shadow-xs hover:bg-indigo-700 focus:outline-hidden focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
          >
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
            Enable Audio
          </button>
          <button
            v-if="State.user.value.host && item.audio"
            @click="api.amix(item.id, false)"
            type="button"
            class="hidden group-hover:inline-flex ml-2 items-center rounded-md border border-transparent bg-indigo-600 px-2.5 py-1.5 text-xs font-bold text-white shadow-xs hover:bg-indigo-700 focus:outline-hidden focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
          >
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
            Disable Audio
          </button>
          <button
            v-if="item.video && selfview && item.id === api.user_id()"
            @click="selfview = false"
            type="button"
            class="hidden group-hover:inline-flex ml-2 items-center rounded-md border border-transparent bg-indigo-600 px-2.5 py-1.5 text-xs font-bold text-white shadow-xs hover:bg-indigo-700 focus:outline-hidden focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
          >
           <EyeIcon class="size-5 mr-1" aria-hidden="true" />
            Disable Selfview
          </button>
          <button
            v-if="item.video && !selfview && item.id === api.user_id()"
            @click="selfview = true"
            type="button"
            class="hidden group-hover:inline-flex ml-2 items-center rounded-md border border-transparent bg-indigo-600 px-2.5 py-1.5 text-xs font-bold text-white shadow-xs hover:bg-indigo-700 focus:outline-hidden focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
          >
           <EyeSlashIcon class="size-5 mr-1" aria-hidden="true" />
            Enable Selfview
          </button>
        </div>
      </div>

      <div class="relative mx-auto" :class="[isFullscreen ? 'h-full' : 'w-full max-w-(--breakpoint-xl)']">
        <video
          ref="video"
          :class="[isFullscreen ? 'h-full' : '']"
          height=1080
          width=1920
          id="live"
          playsinline
          autoplay
          muted
          preload="none"
        ></video>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { watch, onMounted, ref } from 'vue'
import { Webrtc, WebrtcState } from '../webrtc'
import { State } from '../ws/state'
import api from '../api'
import { SpeakerWaveIcon, WifiIcon, PresentationChartBarIcon, EyeIcon, EyeSlashIcon } from '@heroicons/vue/24/outline'
import { useResizeObserver } from '@vueuse/core'

const video = ref<HTMLVideoElement | null>(null)
const overlay_div = ref<HTMLDivElement | null>(null)
const nav = ref(false)
const stats = ref(false)
const hasFullscreen = ref(false)
const isFullscreen = ref(false)
const hasPiP = ref(false)
const PiP = ref(false)
const overlay = ref(true)
const selfview = ref(true)
const vspeakers = State.vspeakers
const sources = State.sources
const room = State.room
let resizeTimer = <NodeJS.Timeout | undefined>undefined

watch(Webrtc.videostream, () => {
  const self: HTMLVideoElement | null = document.querySelector('video#selfview')
  if (self) self.srcObject = Webrtc.videostream.value
})

async function requestPiP() {
  try {
    await video.value?.requestPictureInPicture()
    if (navigator.userAgent.indexOf('Safari') > -1) {
      //Avoid safari setting PiP, since hiding the video stops it, only reduce size.
      video.value && (video.value.width = 256)
    } else {
      PiP.value = true
    }
    nav.value = false
  } catch {
    console.log('PictureInPicture error')
  }
}

function requestFullscreen(element: any) {
  if (isFullscreen.value) {
    document.exitFullscreen()
    return
  }
  if (element.requestFullscreen) {
    element.requestFullscreen()
    nav.value = false
  } else if (element.webkitRequestFullscreen) {
    element.webkitRequestFullscreen()
    nav.value = false
  }
}

function hasFullscreenSupport(element: any) {
  if (element.requestFullscreen) {
    return true
  } else if (element.webkitRequestFullscreen) {
    return true
  }

  return false
}

function resize() {
  overlay.value = true
}

function fullscreenchanged() {
  if (document.fullscreenElement) {
    isFullscreen.value = true
  } else {
    isFullscreen.value = false
  }
}

onMounted(() => {
  Webrtc.init()
  if (video.value?.requestPictureInPicture) hasPiP.value = true
  hasFullscreen.value = hasFullscreenSupport(video.value)

  video.value?.addEventListener('leavepictureinpicture', () => {
    // The video has left PiP mode.
    PiP.value = false
    video.value?.play()
  })

  useResizeObserver(video.value, () => {
    overlay.value = false
    clearTimeout(resizeTimer)
    resizeTimer = setTimeout(resize, 200)
  })
  document.addEventListener('fullscreenchange', fullscreenchanged)
})

function calc_rows() {
  const n = vspeakers.value.length
  if (n <= 3) return 1 

  let rows = 0
  for (rows = 2; ; rows++) {
    if (n <= rows * rows) break
  }

  return rows
}

function calc_cols() {
  const n = vspeakers.value.length
  if (n <= 3) return n 

  let cols = 0
  for (cols = 2; ; cols++) {
    if (n <= cols * cols) break
  }

  return cols
}

function calc_width() {
  if (!video.value) return '0 px'
  return Math.floor(video.value.clientWidth / calc_cols()) + 'px'
}

function calc_height() {
  if (!video.value) return '0 px'
  return Math.floor(video.value.clientHeight / calc_rows()) + 'px'
}

function calc_top(idx: number) {
  if (!video.value) return '0 px'
  const h = Math.floor(video.value.clientHeight / calc_rows())

  return h * Math.floor(idx / calc_cols()) + 'px'
}

function calc_left(idx: number) {
  if (!video.value) return '0 px'
  const cols = calc_cols()
  const w = Math.floor(video.value.clientWidth / cols)
  let offset

  if (isFullscreen.value) {
    offset = (screen.width - video.value!.clientWidth) / 2
  } else {
    offset = 0
  }

  return w * (idx % cols) + offset + 'px'
}

function solo_source(dev: string) {
  api.video_solo(dev, true)
  sources.value.forEach((item) => {
    if (dev === item.dev) item.solo = true
    else item.solo = false
  })
}

function enable_stats() {
  stats.value = !stats.value
  if (stats.value) {
  } else {
  }
}
</script>
