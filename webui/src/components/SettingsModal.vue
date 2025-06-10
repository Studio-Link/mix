<template>
  <TransitionRoot as="template" :show="open">
    <Dialog as="div" class="relative z-50" @close="open = false">
      <TransitionChild
        as="template"
        enter="ease-out duration-300"
        enter-from="opacity-0"
        enter-to="opacity-100"
        leave="ease-in duration-200"
        leave-from="opacity-100"
        leave-to="opacity-0"
      >
        <div class="fixed inset-0 bg-gray-500 opacity-75 transition-opacity" />
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
                    class="mx-auto hidden sm:flex h-12 w-12 shrink-0 items-center justify-center rounded-full bg-green-100 sm:mx-0 sm:h-10 sm:w-10"
                  >
                    <Cog6ToothIcon class="h-6 w-6 text-gray-800" aria-hidden="true" />
                  </div>
                  <div class="mt-3 text-center sm:mt-0 sm:ml-4 sm:text-left space-y-3">
                    <DialogTitle as="h3" class="text-lg font-medium leading-6 text-gray-900">
                      Audio/Video Settings</DialogTitle
                    >
                    <video
                      v-show="video_select !== 'Disabled'"
                      ref="video_echo"
                      playsinline
                      autoplay
                      muted
                      class="mx-auto px-4 mt-2"
                      :class="{'scale-x-[-1]': Webrtc.video_select.value !== 'Screen'}"
                      height="640"
                      width="360"
                    ></video>

                    <div class="text-red-800" v-show="video_select !== 'Disabled' && video_error">
                      {{ video_error }}
                    </div>

                    <div>
                      <div class="flex items-center justify-between">
                        <h2 class="text-sm font-medium text-gray-900">Choose a video option</h2>
                      </div>

                      <RadioGroup v-model="video_select" class="mt-2">
                        <RadioGroupLabel class="sr-only"> Choose a video option </RadioGroupLabel>
                        <div class="grid grid-cols-3 gap-3">
                          <RadioGroupOption
                            as="template"
                            v-for="option in videoOptions"
                            :key="option"
                            :value="option"
                            v-slot="{ active, checked }"
                          >
                            <div
                              :class="[
                                active ? 'ring-2 ring-offset-2 ring-indigo-500' : '',
                                checked
                                  ? 'bg-indigo-600 border-transparent text-white hover:bg-indigo-700'
                                  : 'bg-white border-gray-200 text-gray-900 hover:bg-gray-50',
                                'cursor-pointer border rounded-md py-2 px-3 flex items-center justify-center text-sm font-medium uppercase sm:flex-1',
                              ]"
                            >
                              <RadioGroupLabel as="span">{{ option }} </RadioGroupLabel>
                            </div>
                          </RadioGroupOption>
                        </div>
                      </RadioGroup>
                    </div>

                    <div v-if="video_select === 'Camera'">
                      <label for="cam" class="block text-sm font-medium text-gray-700">Cam</label>
                      <select
                        id="cam"
                        v-model="video_input_id"
                        name="cam"
                        class="mt-1 block w-full rounded-md border-gray-300 py-2 pl-3 pr-10 text-base focus:border-indigo-500 focus:outline-hidden focus:ring-indigo-500 sm:text-sm"
                      >
                        <template v-for="item in Webrtc.deviceInfos.value">
                          <option v-if="item.kind === 'videoinput'" :key="item.deviceId" :value="item.deviceId">
                            {{ item.label }}
                          </option>
                        </template>
                      </select>
                    </div>
                    <div v-if="video_select === 'Camera'">
                      <label for="resolution" class="block text-sm font-medium text-gray-700">Video resolution</label>
                      <select
                        id="resolution"
                        v-model="video_resolution"
                        name="resolution"
                        class="mt-1 block w-full rounded-md border-gray-300 py-2 pl-3 pr-10 text-base focus:border-indigo-500 focus:outline-hidden focus:ring-indigo-500 sm:text-sm"
                      >
                        <option value="720p">High Quality (720p HD)</option>
                        <option value="360p">Normal Quality (360p SD)</option>
                        <option value="360pl">Low Bandwidth (360p SD)</option>
                      </select>
                    </div>
                    <div>
                      <label for="micro" class="block text-sm font-medium text-gray-700">Microphone</label>
                      <select
                        id="micro"
                        v-model="audio_input_id"
                        name="micro"
                        class="mt-1 block w-full rounded-md border-gray-300 py-2 pl-3 pr-10 text-base focus:border-indigo-500 focus:outline-hidden focus:ring-indigo-500 sm:text-sm"
                      >
                        <template v-for="item in Webrtc.deviceInfos.value">
                          <option v-if="item.kind === 'audioinput'" :key="item.deviceId" :value="item.deviceId">
                            {{ item.label }}
                          </option>
                        </template>
                      </select>
                    </div>
                    <div v-if="audio_output_id">
                      <label for="headset" class="block text-sm font-medium text-gray-700">Speaker</label>
                      <select
                        v-model="audio_output_id"
                        id="headset"
                        name="headset"
                        class="mt-1 block w-full rounded-md border-gray-300 py-2 pl-3 pr-10 text-base focus:border-indigo-500 focus:outline-hidden focus:ring-indigo-500 sm:text-sm"
                      >
                        <template v-for="item in Webrtc.deviceInfos.value">
                          <option v-if="item.kind === 'audiooutput'" :key="item.deviceId" :value="item.deviceId">
                            {{ item.label }}
                          </option>
                        </template>
                      </select>
                    </div>
                    <div class="relative flex items-start">
                      <div class="flex items-center h-5">
                        <input
                          id="echo_headset"
                          v-model="echo"
                          name="echo_headset"
                          type="checkbox"
                          class="focus:ring-indigo-500 h-4 w-4 text-indigo-600 border-gray-300 rounded-sm"
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
              <div class="bg-gray-50 px-4 py-3 sm:flex sm:flex-row-reverse sm:px-6 justify-between">
                <button
                  type="button"
                  class="inline-flex w-full items-center rounded-md border border-transparent bg-indigo-600 px-4 py-2 text-base font-medium text-white shadow-xs hover:bg-indigo-700 focus:outline-hidden focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2 sm:ml-3 sm:w-auto sm:text-sm"
                  @click="join()"
                >
                  <MicrophoneIcon class="h-6 w-6 text-white mr-2" aria-hidden="true" /> Join the conversation
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
import { RadioGroup, RadioGroupLabel, RadioGroupOption } from '@headlessui/vue'
import { Cog6ToothIcon, MicrophoneIcon } from '@heroicons/vue/24/outline'
import { State } from '../ws/state'
import { Webrtc, WebrtcState } from '../webrtc'
import { ref, watch } from 'vue'
import api from '../api'
import { Error } from '../error'

const open = State.settings_active
const videoOptions = ['Disabled', 'Camera', 'Screen']
const video_select = Webrtc.video_select
const video_resolution = Webrtc.video_resolution
const audio_input_id = Webrtc.audio_input_id
const audio_output_id = Webrtc.audio_output_id
const video_input_id = Webrtc.video_input_id
const echo = Webrtc.echo
const video_echo = ref<HTMLVideoElement>()
const video_error = Error.video

watch(video_select, async (newValue, oldValue) => {
  const interval = setInterval(async () => {
    if (video_echo.value) {
      clearInterval(interval)
      video_echo.value.srcObject = await Webrtc.change_video()
    }
  }, 50)
})

watch(audio_input_id, async (newValue, oldValue) => {
  if (oldValue === undefined) return //prevent first auto change
  console.log('new audio device: ', newValue)
  await Webrtc.change_audio()
})

watch(video_input_id, async (newValue, oldValue) => {
  if (oldValue === undefined) return //prevent first auto change
  console.log('new video device: ', newValue)
  if (video_echo.value) video_echo.value.srcObject = await Webrtc.change_video()
})

watch(video_resolution, async (newValue, oldValue) => {
  if (oldValue === undefined) return //prevent first auto change
  console.log('new video resolution: ', newValue)
  if (video_echo.value) video_echo.value.srcObject = await Webrtc.change_video()
})

watch(echo, async () => {
  await Webrtc.change_echo()
})

watch(audio_output_id, async (newValue, oldValue) => {
  if (oldValue === undefined) return //prevent first auto change
  await Webrtc.change_audio_out()
})

watch(open, async () => {
  if (!open.value) return

  if (Webrtc.state.value < WebrtcState.ReadySpeaking) {
    await Webrtc.init_avdevices()
  }
})

watch(video_echo, async () => {
  if (video_echo.value) video_echo.value.srcObject = Webrtc.videostream.value
})

function join() {
  open.value = false
  Webrtc.join()
}

function join_listen() {
  open.value = false
  Webrtc.state.value = WebrtcState.Listening
  api.listener(api.user_id())
}
</script>
