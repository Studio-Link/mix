<template>
  <div>
    <video ref="video" :class="{ hidden: previewTaken }" />
    <canvas ref="picture" :class="{ hidden: !previewTaken || webcam.picture.value }" class="w-full" />
    <img v-if="webcam.picture.value" :src="webcam.picture.value" class="mx-auto rounded-full h-48" />
  </div>
  <button
    v-if="!previewTaken"
    @click="preview()"
    class="flex w-full justify-center rounded-md border border-transparent bg-gray-600 py-2 px-4 text-sm font-medium text-white shadow-sm hover:bg-gray-700 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
  >
    Save Preview
  </button>
  <button
    v-if="previewTaken && !webcam.picture.value"
    @click="webcam.savePicture()"
    class="flex w-full justify-center rounded-md border border-transparent bg-gray-600 py-2 px-4 text-sm font-medium text-white shadow-sm hover:bg-gray-700 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
  >
    Save Avatar
  </button>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import webcam from '../webcam'

const video = ref(null)
const picture = ref(null)

const previewTaken = ref(false)

function preview() {
  webcam.takePicture(picture.value, video.value)
  previewTaken.value = true
  webcam.stop()
}

onMounted(() => {
  webcam.start(video.value)
})
</script>
