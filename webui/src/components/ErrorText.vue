<template>
  <div v-if="Error.text.value" class="fixed inset-x-0 top-0 z-50">
    <div class="bg-red-700">
      <div class="mx-auto max-w-7xl py-3 px-3 sm:px-6 lg:px-8">
        <div class="flex flex-wrap items-center justify-between">
          <div class="flex w-0 flex-1 items-center">
            <span class="flex rounded-lg bg-red-800 p-2">
              <MegaphoneIcon class="h-6 w-6 text-white" aria-hidden="true" />
            </span>
            <p class="ml-3 truncate font-medium text-white">
              <span class="">Error: {{ Error.text.value }}</span>
            </p>
          </div>
          <div class="order-2 shrink-0 sm:order-3 sm:ml-3">
            <button
              type="button"
              class="-mr-1 flex rounded-md p-2 hover:bg-red-800 focus:outline-hidden focus:ring-2 focus:ring-white sm:-mr-2"
              @click="Error.reset()"
            >
              <span class="sr-only">Dismiss</span>
              <XMarkIcon class="h-6 w-6 text-white" aria-hidden="true" />
            </button>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { Error } from '../error'
import { MegaphoneIcon, XMarkIcon } from '@heroicons/vue/24/outline'
</script>
