<template>
  <div class="flex min-h-full flex-col justify-center sm:px-6 lg:px-8">
    <div class="sm:mx-auto sm:w-full sm:max-w-md">
      <h2 class="mt-6 text-center text-3xl font-bold tracking-tight text-gray-900">Login</h2>
    </div>

    <div class="mt-8 sm:mx-auto sm:w-full sm:max-w-md">
      <div class="bg-white py-8 px-4 shadow sm:rounded-lg sm:px-10">
        <div class="space-y-6">
          <Avatar v-if="random" ref="avatar" v-bind="props" class="h-48 mx-auto" />
          <WebcamPhoto v-if="!random" />
          <div v-if="random || webcam.picture.value">
            <label for="name" class="block text-sm font-medium text-gray-700">Name</label>
            <div class="mt-1">
              <input
                id="name"
                v-model="name"
                name="name"
                type="text"
                required
                class="block w-full appearance-none rounded-md border border-gray-300 px-3 py-2 placeholder-gray-400 shadow-sm focus:border-indigo-500 focus:outline-none focus:ring-indigo-500 sm:text-sm"
                :class="{ 'border-red-300': error }"
              />
              <p v-if="error" id="email-error" class="mt-2 text-sm text-red-600">Please enter name</p>
            </div>
          </div>
          <div class="space-y-2">
            <button
              v-if="!random && !webcam.picture.value"
              class="flex w-full justify-center rounded-md border border-transparent bg-red-700 py-2 px-4 text-sm font-medium text-white shadow-sm hover:bg-red-800 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
              @click="randomize()"
            >
              Cancel
            </button>
            <button
              v-if="random"
              class="flex w-full justify-center rounded-md border border-transparent bg-gray-600 py-2 px-4 text-sm font-medium text-white shadow-sm hover:bg-gray-700 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
              @click="randomize()"
            >
              Random Avatar
            </button>
            <button
              v-if="random || webcam.picture.value"
              class="flex w-full justify-center rounded-md border border-transparent bg-green-700 py-2 px-4 text-sm font-medium text-white shadow-sm hover:bg-green-800 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
              @click="activateCam()"
            >
              Webcam Photo
            </button>
          </div>
          <button
            v-if="random || webcam.picture.value"
            type="submit"
            class="w-full justify-center rounded-md border border-transparent bg-indigo-600 py-2 px-4 text-base font-bold text-white shadow-sm hover:bg-indigo-700 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
            @click="login()"
          >
            Join Room
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { Avatar, Factory } from 'vue3-avataaars'
import { ref, onMounted } from 'vue'
import WebcamPhoto from '../components/WebcamPhoto.vue'
import webcam from '../webcam'
import api from '../api'

const vprops = defineProps({ token: String })
api.connect(vprops?.token)

const error = ref(false)
const name = ref('')
const random = ref(true)
const avatar = ref<SVGElement | null>(null)
const props = ref(Factory({ isCircle: true }))

onMounted(() => {
  randomize()
})

function randomize() {
  webcam.stop()
  random.value = true
  const items = ['Smile', 'Twinkle', 'Tongue', 'Default']
  const mouth: string = items[Math.floor(Math.random() * items.length)]
  props.value = Factory({ isCircle: true, mouth: mouth })
}

function activateCam() {
  webcam.picture.value = undefined
  random.value = false
  webcam.start()
}

function login() {
  if (!name.value) {
    error.value = true
    return
  }
  error.value = false

  if (webcam.picture.value) {
    api.login(name.value, webcam.picture.value)
  } else {
    const svg = avatar.value
    svg?.setAttribute('width', String(256))
    svg?.setAttribute('height', String(256))
    const xml = new XMLSerializer().serializeToString(svg!)
    const blob = new Blob([xml], { type: 'image/svg+xml' })
    const canvas = document.createElement('canvas')
    const URL = window.URL || window.webkitURL || window
    canvas.width = 256
    canvas.height = 256

    const image = new Image()
    image.onload = () => {
      canvas.getContext('2d')?.drawImage(image, 0, 0)
      api.login(name.value, canvas.toDataURL('image/png'))
    }
    image.src = URL.createObjectURL(blob)
  }
}
</script>
