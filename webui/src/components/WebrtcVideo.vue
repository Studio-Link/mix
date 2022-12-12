<template>
  <div v-show="!PiP && Webrtc.state.value >= WebrtcState.Listening" @mouseover="nav = true" @mouseleave="nav = false" @touchstart.passive="nav = true">
    <div v-show="nav" class="text-center mt-2 text-gray-500">
      <button @click="video!.width = video!.width + 128" class="text-lg" title="Zoom in">
        <svg
          aria-hidden="true"
          focusable="false"
          data-prefix="fas"
          data-icon="magnifying-glass-plus"
          class="h-8 w-8"
          role="img"
          xmlns="http://www.w3.org/2000/svg"
          viewBox="0 0 512 512"
        >
          <path
            fill="currentColor"
            d="M500.3 443.7l-119.7-119.7c27.22-40.41 40.65-90.9 33.46-144.7c-12.23-91.55-87.28-166-178.9-177.6c-136.2-17.24-250.7 97.28-233.4 233.4c11.6 91.64 86.07 166.7 177.6 178.9c53.81 7.191 104.3-6.235 144.7-33.46l119.7 119.7c15.62 15.62 40.95 15.62 56.57 .0003C515.9 484.7 515.9 459.3 500.3 443.7zM288 232H231.1V288c0 13.26-10.74 24-23.1 24C194.7 312 184 301.3 184 288V232H127.1C114.7 232 104 221.3 104 208s10.74-24 23.1-24H184V128c0-13.26 10.74-24 23.1-24S231.1 114.7 231.1 128v56h56C301.3 184 312 194.7 312 208S301.3 232 288 232z"
          ></path>
        </svg>
      </button>
      <button @click="video!.width = video!.width - 128" class="text-lg ml-8" title="Zoom out">
        <svg
          aria-hidden="true"
          focusable="false"
          data-prefix="fas"
          data-icon="magnifying-glass-minus"
          class="h-8 w-8"
          role="img"
          xmlns="http://www.w3.org/2000/svg"
          viewBox="0 0 512 512"
        >
          <path
            fill="currentColor"
            d="M500.3 443.7l-119.7-119.7c27.22-40.41 40.65-90.9 33.46-144.7c-12.23-91.55-87.28-166-178.9-177.6c-136.2-17.24-250.7 97.28-233.4 233.4c11.6 91.64 86.07 166.7 177.6 178.9c53.81 7.191 104.3-6.235 144.7-33.46l119.7 119.7c15.62 15.62 40.95 15.62 56.57 .0003C515.9 484.7 515.9 459.3 500.3 443.7zM288 232H127.1C114.7 232 104 221.3 104 208s10.74-24 23.1-24h160C301.3 184 312 194.7 312 208S301.3 232 288 232z"
          ></path>
        </svg>
      </button>
      <button v-if="hasFullscreen" @click="video!.requestFullscreen()" class="text-lg ml-8" title="Fullscreen">
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
      <button v-if="hasPiP" @click="requestPiP()" class="text-lg ml-8" title="PictureInPicture">
        <svg
          aria-hidden="true"
          focusable="false"
          class="h-8 w-8"
          role="img"
          viewBox="0 0 512 512"
          xmlns="http://www.w3.org/2000/svg"
        >
          <path
            fill="currentColor"
            d="M320 0c-17.7 0-32 14.3-32 32s14.3 32 32 32h82.7L201.4 265.4c-12.5 12.5-12.5 32.8 0 45.3s32.8 12.5 45.3 0L448 109.3V192c0 17.7 14.3 32 32 32s32-14.3 32-32V32c0-17.7-14.3-32-32-32H320zM80 32C35.8 32 0 67.8 0 112V432c0 44.2 35.8 80 80 80H400c44.2 0 80-35.8 80-80V320c0-17.7-14.3-32-32-32s-32 14.3-32 32V432c0 8.8-7.2 16-16 16H80c-8.8 0-16-7.2-16-16V112c0-8.8 7.2-16 16-16H192c17.7 0 32-14.3 32-32s-14.3-32-32-32H80z"
          />
        </svg>
      </button>
    </div>

    <video ref="video" id="live" class="mx-auto px-4 mt-2" width="426" height="240" playsinline autoplay muted></video>
  </div>
</template>

<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { Webrtc, WebrtcState } from '../webrtc'

const video = ref<HTMLVideoElement | null>(null)
const nav = ref(false)
const hasFullscreen = ref(false)
const hasPiP = ref(false)
const PiP = ref(false)

async function requestPiP() {
  try {
    await video.value?.requestPictureInPicture()
    PiP.value = true
  } catch {
    console.log('PictureInPicture error')
  }
}

onMounted(() => {
  if (video.value?.requestPictureInPicture) hasPiP.value = true

  if (video.value?.requestFullscreen) hasFullscreen.value = true

  video.value?.addEventListener('leavepictureinpicture', () => {
    // The video has left PiP mode.
    PiP.value = false
  })
})
</script>
