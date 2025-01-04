<template>
  <div>
    <video ref="video" :class="{ hidden: Webcam.preview.value }" playsinline autoplay muted />
    <canvas ref="picture" :class="{ hidden: !Webcam.preview.value || Webcam.picture.value }" class="w-full" />
    <img v-if="Webcam.picture.value" :src="Webcam.picture.value" class="mx-auto rounded-full h-48" />
  </div>
  <button
    v-if="!Webcam.preview.value"
    class="flex w-full justify-center rounded-md border border-transparent bg-green-700 py-2 px-4 text-sm font-bold text-white shadow-sm hover:bg-geen-800 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
    @click="preview()"
  >
    Snapshot
  </button>
  <div v-if="!Webcam.preview.value">
    <label for="camera" class="block text-sm font-medium text-gray-700">Cam</label>
    <select
      id="cam"
      v-model="video_input_id"
      name="cam"
      class="mt-1 block w-full rounded-md border-gray-300 py-2 pl-3 pr-10 text-base focus:border-indigo-500 focus:outline-none focus:ring-indigo-500 sm:text-sm"
    >
      <template v-for="item in Webcam.deviceInfos.value">
        <option v-if="item.kind === 'videoinput'" :key="item.deviceId" :value="item.deviceId">
          {{ item.label }}
        </option>
      </template>
    </select>
  </div>
  <button
    v-if="Webcam.preview.value && !Webcam.picture.value"
    class="flex w-full justify-center rounded-md border border-transparent bg-green-700 py-2 px-4 text-sm font-bold text-white shadow-sm hover:bg-green-800 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
    @click="Webcam.savePicture()"
  >
    Save Avatar
  </button>
</template>

<script setup lang="ts">
import { ref, onMounted, watch } from 'vue'
import Webcam from '../webcam'

const video = ref(null)
const picture = ref(null)
const video_input_id = Webcam.deviceId

function preview() {
  Webcam.takePicture(picture.value, video.value)
  Webcam.stop()
}

watch(video_input_id, async (newValue: any, oldValue: any) => {
  if (oldValue === undefined) return //prevent first auto change
  Webcam.stop()
  Webcam.start()
})

onMounted(() => {
  Webcam.video(video.value)
})
</script>
