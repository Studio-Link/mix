<template>
  <div class="z-10" v-if="Users.chat_active.value">
    <div class="hidden md:flex w-screen max-w-xs xl:max-w-sm"></div>
    <div class="overflow-hidden" aria-labelledby="slide-over-title" role="dialog" aria-modal="true">
      <div class="fixed inset-y-0 right-0 pl-4 max-w-full flex sm:pl-16">
        <div
          class="w-screen max-w-xs xl:max-w-sm"
          x-transition:enter="transform transition ease-in-out duration-500 sm:duration-700"
          x-transition:enter-start="translate-x-full"
          x-transition:enter-end="translate-x-0"
          x-transition:leave="transform transition ease-in-out duration-500 sm:duration-700"
          x-transition:leave-start="translate-x-0"
          x-transition:leave-end="translate-x-full"
        >
          <div class="h-full flex flex-col bg-white shadow-xl overflow-y-scroll">
            <div class="p-4">
              <div class="flex items-start justify-between">
                <h2 class="text-lg font-medium text-gray-900" id="slide-over-title">Chat</h2>
              </div>
            </div>

            <ul class="chat flex-1 divide-y divide-gray-200 overflow-y-auto">
              <li v-for="(item, index) in messages" :key="index">
                <div class="relative group py-4 px-5 flex items-center">
                  <div class="absolute inset-0 group-hover:bg-gray-50" aria-hidden="true"></div>
                  <div class="flex-1 flex items-top min-w-0 relative">
                    <span class="flex-shrink-0 inline-block relative">
                      <picture>
                        <source type="image/webp" :srcset="'/avatars/' + item.user_id + '.webp'" />
                        <img
                          class="h-10 w-10 rounded-full"
                          :src="'/avatars/' + item.user_id + '.png'"
                          alt="Avatar Image"
                        />
                      </picture>
                    </span>
                    <div class="ml-4">
                      <div class="text-sm space-x-2">
                        <span class="text-gray-900">{{ item.user_name }}</span>
                        <span class="text-gray-500">
                          {{ formatTimeAgo(new Date(parseInt(item.time) * 1000)) }}
                        </span>
                      </div>
                      <p>{{ item.msg }}</p>
                    </div>
                  </div>
                </div>
              </li>
              <li v-for="(item, index) in Users.chat_messages?.value" :key="index">
                <div class="relative group py-4 px-5 flex items-center">
                  <div class="absolute inset-0 group-hover:bg-gray-50" aria-hidden="true"></div>
                  <div class="flex-1 flex items-top min-w-0 relative">
                    <span class="flex-shrink-0 inline-block relative">
                      <picture>
                        <source type="image/webp" :srcset="'/avatars/' + item.user_id + '.webp'" />
                        <img
                          class="h-10 w-10 rounded-full"
                          :src="'/avatars/' + item.user_id + '.png'"
                          alt="Avatar Image"
                        />
                      </picture>
                    </span>
                    <div class="ml-4">
                      <div class="text-sm space-x-2">
                        <span class="text-gray-900">{{ item.user_name }}</span>
                        <span class="text-gray-500">
                          {{ formatTimeAgo(new Date(parseInt(item.time) * 1000)) }}
                        </span>
                      </div>
                      <p>{{ item.msg }}</p>
                    </div>
                  </div>
                </div>
              </li>
            </ul>
            <div class="bg-gray-50 px-4 py-6 sm:px-6">
              <div class="flex space-x-3">
                <div class="flex-shrink-0">
                  <picture>
                    <source type="image/webp" :srcset="'/avatars/' + user_id + '.webp'" />
                    <img class="h-10 w-10 rounded-full" :src="'/avatars/' + user_id + '.png'" alt="Avatar Image" />
                  </picture>
                </div>
                <div class="min-w-0 flex-1">
                  <div>
                    <label for="comment" class="sr-only">Chat message</label>
                    <input
                      v-model="msg"
                      v-on:keyup.enter="chat()"
                      class="shadow-sm block w-full h-8 p-5 focus:ring-lime-600 focus:border-lime-600 sm:text-sm border border-gray-300 rounded-md"
                      placeholder="Add a comment"
                    />
                  </div>
                  <div class="mt-3 flex items-center justify-between">
                    <span class="group inline-flex items-start text-sm space-x-2 text-gray-500 hover:text-gray-900">
                    </span>
                    <button
                      @click="chat()"
                      type="submit"
                      class="inline-flex items-center justify-center px-4 py-2 border border-transparent text-sm font-medium rounded-md shadow-sm text-white bg-blue-600 hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500"
                    >
                      Comment
                    </button>
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import api from '../api'
import { onMounted, onUpdated, ref } from 'vue'
import { Users } from '../ws/users'
import { formatTimeAgo } from '@vueuse/core'

const messages = ref<any>([])
const msg = ref('')
const user_id = api.session().user_id

function chat() {
  api.chat(msg.value)
  msg.value = ''
}

onUpdated(() => {
  const chat = document.querySelector('.chat')
  if (chat) chat.scrollTop = chat.scrollHeight
  window.onresize = function () {
    if (chat) chat.scrollTop = chat.scrollHeight
  }
})

onMounted(async () => {
  const chat: Response | void = await api.get_chat()
  const json = await chat?.json()
  messages.value = json?.chats
})
</script>
