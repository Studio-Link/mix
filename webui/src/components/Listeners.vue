<template>
  <div class="mt-10 sm:mt-12">
    <div class="max-w-7xl mx-auto px-4 text-center sm:px-6 lg:px-8">
      <div class="space-y-8 sm:space-y-10">
        <div class="space-y-5 sm:mx-auto sm:max-w-xl sm:space-y-4 lg:max-w-5xl">
          <h2 class="text-xl font-medium tracking-tight">Listeners</h2>
        </div>
        <ul
          class="mx-auto grid grid-cols-4 gap-x-4 gap-y-6 sm:grid-cols-5 md:grid-cols-6 md:gap-x-6 lg:max-w-5xl lg:gap-x-8 lg:gap-y-12 xl:grid-cols-6"
        >
          <li v-for="item in Users.listeners" :key="item.id">
            <div class="space-y-4">
              <picture>
                <source type="image/webp" :srcset="'/avatars/' + item.id + '.webp'" />
                <img
                  class="mx-auto h-16 w-16 rounded-full lg:w-20 lg:h-20 hover:ring-2 ring-gray-500"
                  :src="'/avatars/' + item.id + '.png'"
                  alt="Avatar Image"
                />
              </picture>
              <div class="space-y-2">
                <div class="text-xs font-medium">
                  <h3>{{ item.name }}</h3>
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
</script>
