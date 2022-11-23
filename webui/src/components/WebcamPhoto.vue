<template>
  <div>
    <video ref="video" :class="{ hidden: webcam.preview.value }" />
    <canvas ref="picture" :class="{ hidden: !webcam.preview.value || webcam.picture.value }" class="w-full" />
    <img v-if="webcam.picture.value" :src="webcam.picture.value" class="mx-auto rounded-full h-48" />
  </div>
  <button
    v-if="!webcam.preview.value"
    class="flex w-full justify-center rounded-md border border-transparent bg-green-700 py-2 px-4 text-sm font-bold text-white shadow-sm hover:bg-geen-800 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
    @click="preview()"
  >
    Snapshot
  </button>
  <button
    v-if="webcam.preview.value && !webcam.picture.value"
    class="flex w-full justify-center rounded-md border border-transparent bg-green-700 py-2 px-4 text-sm font-bold text-white shadow-sm hover:bg-green-800 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
    @click="webcam.savePicture()"
  >
    Save Avatar
  </button>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import webcam from '../webcam'

const video = ref(null)
const picture = ref(null)

function preview() {
  webcam.takePicture(picture.value, video.value)
  webcam.stop()
}

onMounted(() => {
  webcam.video(video.value)
})
</script>
