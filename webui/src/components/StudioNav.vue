<template>
  <div class="z-10 md:hidden flex items-center fixed left-0">
    <button
      :class="{ 'ml-16': menuOpen }"
      type="button"
      class="inline-flex items-center justify-center rounded-md p-2.5 text-gray-400"
      @click="menuOpen = !menuOpen"
    >
      <span class="sr-only">Open/Close nav menu</span>
      <Bars3Icon v-if="!menuOpen" class="h-6 w-6" aria-hidden="true" />
      <XMarkIcon v-if="menuOpen" class="h-6 w-6" aria-hidden="true" />
    </button>
  </div>
  <!-- Static sidebar for desktop -->
  <div :class="[menuOpen ? 'flex' : 'hidden']" class="md:flex flex-shrink-0 bg-gray-600 min-h-screen">
    <div class="flex w-16 flex-col h-screen">
      <div class="flex min-h-0 flex-1 flex-col overflow-y-auto bg-sl-01dpa">
        <div class="flex-1">
          <nav aria-label="Sidebar" class="flex flex-col items-center space-y-3 py-4">
            <a target="_self" v-for="room in Users.rooms.value" :key="room.name" :href="room.url" class="relative block group">
              <div class="absolute flex items-center h-full -left-2 -top-2">
                <div
                  :class="[room.url != path ? 'scale-0 opacity-0' : '']"
                  class="h-5 group-hover:opacity-100 group-hover:scale-100 w-1 transition-all duration-200 origin-left bg-gray-100 rounded-r"
                ></div>
              </div>
              <div class="group-active:translate-y-px">
                <div
                  class="text-gray-100 group-hover:rounded-2xl group-hover:bg-gray-100 group-hover:text-white bg-sl-02dpa rounded-3xl flex items-center justify-center w-12 h-12 transition-all duration-200 overflow-hidden"
                >
                  <img alt="Studio" class="rounded-lg" src="https://www.podstock.de/assets/default_logo.jpg" />
                  <span class="sr-only">{{ room.name }}</span>

                  <span
                    class="absolute top-0 right-0 block translate-y-1/2 translate-x-1/2 transform rounded-full -mt-5 mr-1"
                  >
                    <span class="bg-gray-500 text-gray-50 ml-auto inline-block rounded-full px-1.5 text-xs">{{
                      room.listeners
                    }}</span>
                  </span>
                </div>
              </div>
              <div class="w-12 text-center text-gray-200 text-xs mt-1 overflow-hidden whitespace-nowrap text-clip">
                {{ room.name }}
              </div>
            </a>
          </nav>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { Users } from '../ws/users'
import { ref } from 'vue'
import { Bars3Icon, XMarkIcon } from '@heroicons/vue/24/outline'

const path = ref(location.pathname)
const menuOpen = ref(false)
</script>
