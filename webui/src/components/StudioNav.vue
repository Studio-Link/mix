<template>
  <!-- Static sidebar for desktop -->
  <div class="flex flex-shrink-0 mr-2 bg-gray-600 min-h-screen">
    <div class="flex w-16 flex-col h-screen">
      <div class="flex min-h-0 flex-1 flex-col overflow-y-auto bg-sl-01dpa">
        <div class="flex-1">
          <nav aria-label="Sidebar" class="flex flex-col items-center space-y-3 py-6">
            <a href="#" class="relative block group -mt-2">
              <div class="absolute flex items-center h-full -left-2">
                <div class="h-10 w-1 transition-all duration-200 origin-left bg-sl-primary rounded-r"></div>
              </div>
              <div class="group-active:translate-y-px">
                <div
                  class="flex items-center justify-center w-12 h-12 transition-all duration-200 overflow-hidden rounded-2xl text-white hover:bg-sl-08dpa"
                >
                  <img class="h-10" src="https://www.podstock.de/assets/default_logo.jpg" alt="Logo" />
                  <span class="sr-only">Studio Link Default</span>
                </div>
              </div>
            </a>
            <hr class="w-2/3 border-t-white/[.06] border-t-2 rounded mx-2" />
            <a v-for="room in Users.rooms.value" :key="room.name" :href="room.url" class="relative block group">
              <div class="absolute flex items-center h-full -left-2 -top-2">
                <div
                  class="h-5 scale-0 opacity-0 group-hover:opacity-100 group-hover:scale-100 w-1 transition-all duration-200 origin-left bg-gray-100 rounded-r"
                ></div>
              </div>
              <div class="group-active:translate-y-px">
                <div
                  class="text-gray-100 group-hover:rounded-2xl group-hover:bg-gray-100 group-hover:text-white bg-sl-02dpa rounded-3xl flex items-center justify-center w-12 h-12 transition-all duration-200 overflow-hidden"
                >
                  <img alt="Studio" class="rounded-lg" src="https://www.podstock.de/assets/default_logo.jpg" />
                  <span class="sr-only">{{ room.name }}</span>

                  <span
                    class="absolute top-0 right-0 block translate-y-1/2 translate-x-1/2 transform rounded-full -mt-5 mr-2"
                  >
                    <span class="bg-gray-200 text-gray-600 ml-auto inline-block rounded-full py-0.5 px-2 text-xs">{{
                      room.listeners
                    }}</span>
                  </span>
                </div>
              </div>
              <div class="w-12 text-gray-200 text-xs mt-1 overflow-hidden whitespace-nowrap text-clip">
                {{room.name}}
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
</script>
