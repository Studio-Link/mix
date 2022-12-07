<template>
  <TransitionRoot as="template" :show="open">
    <Dialog as="div" class="relative z-10" @close="open = false">
      <TransitionChild
        as="template"
        enter="ease-out duration-300"
        enter-from="opacity-0"
        enter-to="opacity-100"
        leave="ease-in duration-200"
        leave-from="opacity-100"
        leave-to="opacity-0"
      >
        <div class="fixed inset-0 bg-gray-500 bg-opacity-75 transition-opacity" />
      </TransitionChild>

      <div class="fixed inset-0 z-10 overflow-y-auto">
        <div class="flex min-h-full items-end justify-center p-4 text-center sm:items-center sm:p-0">
          <TransitionChild
            as="template"
            enter="ease-out duration-300"
            enter-from="opacity-0 translate-y-4 sm:translate-y-0 sm:scale-95"
            enter-to="opacity-100 translate-y-0 sm:scale-100"
            leave="ease-in duration-200"
            leave-from="opacity-100 translate-y-0 sm:scale-100"
            leave-to="opacity-0 translate-y-4 sm:translate-y-0 sm:scale-95"
          >
            <DialogPanel 
              class="relative transform overflow-hidden rounded-lg bg-white text-left shadow-xl transition-all sm:my-8 sm:w-full sm:max-w-lg"
            >
              <div class="bg-white px-4 pt-5 pb-4 sm:p-6 sm:pb-4">
                <div class="sm:flex sm:items-start">
                  <div
                    class="mx-auto flex h-12 w-12 flex-shrink-0 items-center justify-center rounded-full bg-green-100 sm:mx-0 sm:h-10 sm:w-10"
                  >
                    <Cog6ToothIcon class="h-6 w-6 text-gray-800" aria-hidden="true" />
                  </div>
                  <div class="mt-3 text-center sm:mt-0 sm:ml-4 sm:text-left space-y-3">
                    <DialogTitle as="h3" class="text-lg font-medium leading-6 text-gray-900"
                      >Audio/Video Settings</DialogTitle
                    >
                    <video
                      ref="video_echo"
                      playsinline
                      autoplay
                      muted
                      class="mx-auto px-4 mt-2"
                      height="640"
                      width="360"
                    ></video>

                    <div>
                      <label for="camera" class="block text-sm font-medium text-gray-700">Cam</label>
                      <select
                        v-model="video_input_id"
                        id="cam"
                        name="cam"
                        class="mt-1 block w-full rounded-md border-gray-300 py-2 pl-3 pr-10 text-base focus:border-indigo-500 focus:outline-none focus:ring-indigo-500 sm:text-sm"
                      >
                        <option value="disabled">Disabled</option>
                        <template v-for="item in Webrtc.deviceInfos.value">
                          <option v-if="item.kind === 'videoinput'" :value="item.deviceId">{{ item.label }}</option>
                        </template>
                      </select>
                    </div>
                    <div>
                      <label for="microphone" class="block text-sm font-medium text-gray-700">Microphone</label>
                      <select
                        v-model="audio_input_id"
                        id="micro"
                        name="micro"
                        class="mt-1 block w-full rounded-md border-gray-300 py-2 pl-3 pr-10 text-base focus:border-indigo-500 focus:outline-none focus:ring-indigo-500 sm:text-sm"
                      >
                        <template v-for="item in Webrtc.deviceInfos.value">
                          <option v-if="item.kind === 'audioinput'" :value="item.deviceId">{{ item.label }}</option>
                        </template>
                      </select>
                    </div>
                    <div class="hidden">
                      <label for="headset" class="block text-sm font-medium text-gray-700">Headset</label>
                      <select
                        id="headset"
                        name="headset"
                        class="mt-1 block w-full rounded-md border-gray-300 py-2 pl-3 pr-10 text-base focus:border-indigo-500 focus:outline-none focus:ring-indigo-500 sm:text-sm"
                      >
                        <option>Default</option>
                      </select>
                    </div>
                    <div class="relative flex items-start">
                      <div class="flex items-center h-5">
                        <input
                          id="echo_headset"
                          name="echo_headset"
                          type="checkbox"
                          class="focus:ring-indigo-500 h-4 w-4 text-indigo-600 border-gray-300 rounded"
                        />
                      </div>
                      <div class="ml-3 text-base">
                        <label for="echo_headset" class="text-gray-700"
                          >I'm <b>not</b> wearing a headset
                          <span class="text-sm">(activate echo cancellation)</span></label
                        >
                      </div>
                    </div>
                  </div>
                </div>
              </div>
              <div class="bg-gray-50 px-4 py-3 sm:flex sm:flex-row-reverse sm:px-6">
                <button
                  type="button"
                  class="inline-flex w-full justify-center rounded-md border border-transparent bg-indigo-600 px-4 py-2 text-base font-medium text-white shadow-sm hover:bg-indigo-700 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2 sm:ml-3 sm:w-auto sm:text-sm"
                  @click="join()"
                >
                  Join the conversation
                </button>
              </div>
            </DialogPanel>
          </TransitionChild>
        </div>
      </div>
    </Dialog>
  </TransitionRoot>
</template>

<script setup lang="ts">
import { Dialog, DialogPanel, DialogTitle, TransitionChild, TransitionRoot } from '@headlessui/vue'
import { Cog6ToothIcon } from '@heroicons/vue/24/outline'
import { Users } from '../ws/users'
import { Webrtc } from '../webrtc'
import { ref, watch, onUpdated } from 'vue'

const open = Users.settings_active
const audio_input_id = Webrtc.audio_input_id
const video_input_id = Webrtc.video_input_id
const video_echo = ref<HTMLVideoElement>()

watch(audio_input_id, async (newValue, oldValue) => {
  if (oldValue === undefined) return //prevent first auto change
  console.log('new audio device: ', newValue)
  if (video_input_id.value !== 'disabled' && video_echo.value) {
    video_echo.value.srcObject = await Webrtc.change_audio()
  }
})

watch(video_input_id, async (newValue, oldValue) => {
  if (oldValue === undefined) return //prevent first auto change
  console.log('new video device: ', newValue)
  if (video_echo.value) video_echo.value.srcObject = await Webrtc.change_video()
})

onUpdated(async () => {
  if (open.value) {
    if (video_input_id.value !== 'disabled') {
        const avstream = await Webrtc.init_avdevices()
        if (video_echo.value) video_echo.value.srcObject = avstream
    }
  }
})

function join() {
  open.value = false
  Webrtc.join()
}
</script>
