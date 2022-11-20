<template>
  <div class="flex min-h-full flex-col justify-center py-12 sm:px-6 lg:px-8">
    <div class="sm:mx-auto sm:w-full sm:max-w-md">
      <h2 class="mt-6 text-center text-3xl font-bold tracking-tight text-gray-900">Login???</h2>
    </div>

    <div class="mt-8 sm:mx-auto sm:w-full sm:max-w-md">
      <div class="bg-white py-8 px-4 shadow sm:rounded-lg sm:px-10">
        <div class="space-y-6">
          <div>
            <label for="name" class="block text-sm font-medium text-gray-700">Name</label>
            <div class="mt-1">
              <input
                id="name"
                name="name"
                type="text"
                required
                class="block w-full appearance-none rounded-md border border-gray-300 px-3 py-2 placeholder-gray-400 shadow-sm focus:border-indigo-500 focus:outline-none focus:ring-indigo-500 sm:text-sm"
              />
            </div>
          </div>
          <Avatar v-if="random" ref="avatar" v-bind="props" class="h-48 mx-auto" />
          <WebcamPhoto v-if="!random" />
          <div v-if="!webcam.picture.value" class="space-y-2">
            <button
              v-if="!random"
              @click="randomize()"
              class="flex w-full justify-center rounded-md border border-transparent bg-red-600 py-2 px-4 text-sm font-medium text-white shadow-sm hover:bg-red-700 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
            >
              Cancel
            </button>
            <button
              v-if="random"
              @click="randomize()"
              class="flex w-full justify-center rounded-md border border-transparent bg-gray-600 py-2 px-4 text-sm font-medium text-white shadow-sm hover:bg-gray-700 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
            >
              Random
            </button>
            <button
              v-if="random"
              @click="activateCam()"
              class="flex w-full justify-center rounded-md border border-transparent bg-green-600 py-2 px-4 text-sm font-medium text-white shadow-sm hover:bg-green-700 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
            >
              Webcam Photo
            </button>
          </div>
          <div>
            <button
              @click="login()"
              type="submit"
              class="flex w-full justify-center rounded-md border border-transparent bg-indigo-600 py-2 px-4 text-sm font-medium text-white shadow-sm hover:bg-indigo-700 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
            >
              Raum betreten
            </button>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { Avatar, Factory } from 'vue3-avataaars'
import { ref } from 'vue'
import WebcamPhoto from '../components/WebcamPhoto.vue'
import webcam from '../webcam'
import api from '../api'

const random = ref(true)
const avatar = ref<Node | null>(null)
const props = ref(Factory({ isCircle: true }))

function randomize() {
  random.value = true
  props.value = Factory({ isCircle: true })
}

function activateCam() {
  random.value = false
}

function login() {
  if (webcam.picture.value) {
    api.avatar(webcam.picture.value)
  } else {
    let xml = new XMLSerializer().serializeToString(avatar.value!)
    let blob = new Blob([xml], { type: 'image/svg+xml' })
    let canvas = document.createElement('canvas')

    let image = new Image()
    image.onload = () => {
      canvas?.getContext('2d')?.drawImage(image, 0, 0)
      api.avatar(canvas.toDataURL('image/png'))
    }
    image.src = window.URL.createObjectURL(blob)
  }
}
</script>
