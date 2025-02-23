<template>
  <div class="z-30" v-if="Users.chat_active.value">
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

            <ul v-scroll="onScroll" ref="chat" class="chat flex-1 divide-y divide-gray-200 overflow-y-auto">
              <li v-for="(item, index) in messages" :key="index">
                <div class="relative group py-4 px-2 flex items-center">
                  <div class="absolute inset-0 group-hover:bg-gray-50" aria-hidden="true"></div>
                  <div class="flex-1 flex items-top min-w-0 relative">
                    <span class="shrink-0 inline-block relative">
                      <picture>
                        <source type="image/webp" :srcset="'/avatars/' + item.user_id + '.webp'" />
                        <img
                          class="h-10 w-10 rounded-full"
                          src="/avatars/default.png"
                          alt="Avatar"
                          @error="imgError"
                        />
                      </picture>
                    </span>
                    <div class="ml-2">
                      <div class="text-sm space-x-2">
                        <span class="text-gray-900">{{ item.user_name }}</span>
                        <span class="text-gray-500">
                          {{ formatTimeAgo(new Date(parseInt(item.time) * 1000)) }}
                        </span>
                      </div>
                      <article class="prose" target="_blank" v-html="message(item.msg)"></article>
                    </div>
                  </div>
                </div>
              </li>
              <li v-for="(item, index) in Users.chat_messages?.value" :key="index">
                <div class="relative group py-4 px-2 flex items-center">
                  <div class="absolute inset-0 group-hover:bg-gray-50" aria-hidden="true"></div>
                  <div class="flex-1 flex items-top min-w-0 relative">
                    <span class="shrink-0 inline-block relative">
                      <picture>
                        <source type="image/webp" :srcset="'/avatars/' + item.user_id + '.webp'" />
                        <img
                          class="h-10 w-10 rounded-full"
                          :src="'/avatars/' + item.user_id + '.png'"
                          alt="Avatar Image"
                        />
                      </picture>
                    </span>
                    <div class="ml-2">
                      <div class="text-sm space-x-2">
                        <span class="text-gray-900">{{ item.user_name }}</span>
                        <span class="text-gray-500">
                          {{ formatTimeAgo(new Date(parseInt(item.time) * 1000)) }}
                        </span>
                      </div>
                      <article class="prose" v-html="message(item.msg)"></article>
                    </div>
                  </div>
                </div>
              </li>
            </ul>
            <div v-if="scroll_stop" class="flex justify-end">
              <button
                @click="scroll_stop = false"
                type="button"
                class="rounded-full bg-indigo-600 p-2 text-white shadow-xs hover:bg-indigo-500 focus-visible:outline focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-indigo-600"
              >
                <ArrowDownIcon class="h-5 w-5" aria-hidden="true" />
                <span class="sr-only">Scroll down</span>
              </button>
            </div>
            <div class="bg-gray-50 py-6 px-2">
              <div class="flex space-x-3">
                <div class="shrink-0">
                  <picture>
                    <source type="image/webp" :srcset="'/avatars/' + user_id + '.webp'" />
                    <img class="h-10 w-10 rounded-full" :src="'/avatars/' + user_id + '.png'" alt="Avatar Image" />
                  </picture>
                </div>
                <div class="min-w-0 flex-1">
                  <div>
                    <label for="comment" class="sr-only">Chat message</label>
                    <textarea
                      v-model="msg"
                      v-on:keydown.enter.exact.prevent="chat_button()"
                      class="shadow-xs block w-full h-12 p-2 focus:ring-lime-600 focus:border-lime-600 sm:text-sm border border-gray-300 rounded-md"
                      placeholder="Add a comment"
                    ></textarea>
                  </div>
                  <div class="mt-3 flex items-center justify-between">
                    <span class="group inline-flex items-start text-sm space-x-2 text-gray-500 hover:text-gray-900">
                    </span>
                    <button
                      @click="chat_button()"
                      type="submit"
                      class="inline-flex items-center justify-center px-4 py-2 border border-transparent text-sm font-medium rounded-md shadow-xs text-white bg-blue-600 hover:bg-blue-700 focus:outline-hidden focus:ring-2 focus:ring-offset-2 focus:ring-blue-500"
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
import type { UseScrollReturn } from '@vueuse/core'
import { vScroll } from '@vueuse/components'
import markdownit from 'markdown-it'
import { ArrowDownIcon } from '@heroicons/vue/24/outline'

const messages = ref<any>([])
const msg = ref('')
const scroll_stop = ref(false)
const chat = ref<HTMLElement | null>(null)

const user_id = api.user_id()
const md = markdownit({
  html: false,
  linkify: true,
  typographer: true,
  breaks: true,
})

function imgError(e: any) {
    e.target.parentNode.children[0].srcset = e.target.src
}

function chat_button() {
  api.chat(msg.value)
  msg.value = ''
  scroll_stop.value = false
}

function onScroll(state: UseScrollReturn) {
  if (state.directions.top) scroll_stop.value = true
  if (state.arrivedState.bottom) scroll_stop.value = false
}

onUpdated(() => {
  if (chat.value && !scroll_stop.value) chat.value.scrollTop = chat.value.scrollHeight

  window.onresize = function () {
    if (chat.value) chat.value.scrollTop = chat.value.scrollHeight
  }

  if (Users.chat_active.value && Users.chat_unread.value) {
    Users.chat_unread.value = 0
  }
})

onMounted(async () => {
  const chat_json: Response | void = await api.get_chat()
  const json = await chat_json?.json()
  messages.value = json?.chats
})

function message(msg: string) {
  let txt = ''
  msg.split('\\n').forEach((item) => {
    txt += item + '\n'
  })
  return md.render(txt)
}
</script>
