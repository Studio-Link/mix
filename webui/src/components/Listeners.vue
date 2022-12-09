<template>
  <div class="mt-10 sm:mt-12">
    <div class="max-w-7xl mx-auto px-4 text-center sm:px-6 lg:px-8">
      <div class="space-y-8 sm:space-y-10">
        <div class="space-y-5 sm:mx-auto sm:max-w-xl sm:space-y-4 lg:max-w-5xl">
          <h2 v-if="Users.listeners.value?.length" class="text-xl font-medium tracking-tight">Listeners</h2>
        </div>
        <ul
          class="mx-auto grid grid-cols-4 gap-x-4 gap-y-6 sm:grid-cols-4 md:gap-x-5 lg:max-w-5xl lg:gap-x-8 lg:gap-y-12 xl:grid-cols-6"
        >
          <li v-for="item in Users.listeners.value" :key="item.id">
            <div class="space-y-4">
              <div class="inline-block relative">
                <picture>
                  <source type="image/webp" :srcset="'/avatars/' + item.id + '.webp'" />
                  <img
                    class="mx-auto h-16 w-16 rounded-full lg:w-20 lg:h-20 hover:ring-2 ring-gray-500"
                    :src="'/avatars/' + item.id + '.png'"
                    alt="Avatar Image"
                  />
                </picture>
                <span v-show="item.hand" class="absolute -top-1 -right-1 bg-indigo-600 rounded-lg text-gray-200">
                  <svg
                    class="h-6 w-6"
                    xmlns="http://www.w3.org/2000/svg"
                    fill="none"
                    viewBox="0 0 24 24"
                    stroke="currentColor"
                  >
                    <path
                      stroke-linecap="round"
                      stroke-linejoin="round"
                      stroke-width="2"
                      d="M7 11.5V14m0-2.5v-6a1.5 1.5 0 113 0m-3 6a1.5 1.5 0 00-3 0v2a7.5 7.5 0 0015 0v-5a1.5 1.5 0 00-3 0m-6-3V11m0-5.5v-1a1.5 1.5 0 013 0v1m0 0V11m0-5.5a1.5 1.5 0 013 0v3m0 0V11"
                    />
                  </svg>
                </span>
                <button v-if="api.is_host()" @click="api.speaker(item.id)">To Speaker</button>
                <div class="space-y-2">
                  <div class="text-xs font-medium">
                    <h3>{{ item.name }}</h3>
                  </div>
                </div>
              </div>
            </div>
          </li>
        </ul>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { Users } from '../ws/users'
import api from '../api'
</script>
