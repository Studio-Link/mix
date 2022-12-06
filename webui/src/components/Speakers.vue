<template>
  <div class="mt-6">
    <div class="max-w-7xl mx-auto px-4 text-center sm:px-6 lg:px-8">
      <div class="space-y-8 sm:space-y-10">
        <ul
          class="mx-auto grid grid-cols-3 gap-x-4 gap-y-6 sm:grid-cols-3 md:grid-cols-3 md:gap-x-4 lg:max-w-5xl lg:gap-x-8 lg:gap-y-12 xl:grid-cols-6"
        >
          <li v-for="item in Users.speakers.value" :key="item.id">
            <div class="space-y-2">
              <picture>
                <source type="image/webp" :srcset="'/avatars/' + item.id + '.webp'" />
                <img
                  class="mx-auto h-20 w-20 rounded-full lg:w-24 lg:h-24 hover:ring-2 ring-gray-500"
                  :src="'/avatars/' + item.id + '.png'"
                  alt="Avatar Image"
                />
              </picture>
              <button v-if="api.is_host()" @click="api.listener(item.id)">To Listener</button>
              <div class="space-y-2">
                <div class="text-xs font-medium">
                  <h3>{{ item.name }}</h3>
                  audio: {{item.audio}}
                  video: {{item.video}}
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
