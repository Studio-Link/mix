<template>
  <div class="relative">
    <button
      @click="open = !open"
      :class="{ 'bg-gray-700': open }"
      class="text-gray-300 hover:bg-gray-700 hover:text-white group block px-2 py-2 text-base font-medium rounded-md"
    >
      <HeartIcon class="h-9 w-9 mx-auto" />
    </button>

    <div
      v-if="open"
      class="absolute bottom-full left-1/2 transform -translate-x-1/2 mb-2 bg-white rounded-sm shadow-lg p-4"
    >
      <div class="flex space-x-4 text-lime-600 text-3xl">
        <button v-for="(reaction, index) in reactions" @click="push(index)">
          {{ reaction.emoji }}
        </button>
      </div>
    </div>
  </div>

  <Teleport to="body">
    <div
      aria-hidden="true"
      v-for="emoji in emojis"
      :key="emoji.id"
      class="emoji text-blue-600"
      :style="{ left: `${emoji.x}%`, fontSize: `${emoji.size}px` }"
    >
      {{ reactions[emoji.reaction_id].emoji }}
    </div>
  </Teleport>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { Users } from '../ws/users'
import api from '../api'
import { HeartIcon } from '@heroicons/vue/24/outline'

const emojis = Users.emojis
const open = ref(false)
const reactions = [{ emoji: '❤️' }, { emoji: '🤣' }]

async function push(reaction_id: number) {
  api.emoji(reaction_id)
}
</script>

<style scoped>
.emoji {
  position: absolute;
  user-select: none;
  bottom: 0px;
  animation: fly 3s ease-out;
  will-change: transform, opacity;
  z-index: 99;
}

/* Animation keyframes */
@keyframes fly {
  0% {
    transform: translateY(0);
    opacity: 1;
  }

  100% {
    transform: translateY(-50vh);
    opacity: 0;
  }
}
</style>
