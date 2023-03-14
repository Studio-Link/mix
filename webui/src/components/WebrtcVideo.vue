<template>
  <div
    v-show="!PiP && Webrtc.state.value >= WebrtcState.Listening"
    @mouseover="nav = true"
    @mouseleave="nav = false"
    @touchstart.passive="nav = true"
    class="relative flex bg-blue-100 max-w-screen-xl mx-auto"
  >
    <div v-show="nav" class="absolute z-10 text-gray-200 bottom-0 right-0 px-2">
      <button v-if="hasFullscreen" @click="requestFullscreen(video)" class="text-lg" title="Fullscreen">
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
      <button v-if="hasPiP" @click="requestPiP()" class="text-lg ml-5" title="PictureInPicture">
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

    <video ref="video" id="live" class="relative mx-auto w-full max-w-screen-xl" playsinline autoplay muted preload="none">

    </video>
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

onMounted(() => {
  if (video.value?.requestPictureInPicture) hasPiP.value = true
  hasFullscreen.value = hasFullscreenSupport(video.value)

  video.value?.addEventListener('leavepictureinpicture', () => {
    // The video has left PiP mode.
    PiP.value = false
    video.value?.play()
  })
})
</script>
