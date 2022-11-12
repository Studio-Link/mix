<template>
  <div class="fixed inset-x-0 bottom-0">
    <div class="bg-gray-600">
      <div class="mx-auto py-1 px-3 sm:px-6 lg:px-8">
        <div class="flex items-center flex-wrap justify-center space-x-16">
          <div>
            <a
              href="#"
              class="text-gray-300 hover:bg-gray-700 hover:text-white group block px-2 py-2 text-base font-medium rounded-md"
            >
              <svg
                class="h-9 w-9 mx-auto"
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
              <!-- <div class="text-gray-300 text-sm text-center">Hand</div> -->
            </a>
          </div>
          <div>
            <a
              @click="listen()"
              href="#"
              class="text-gray-300 hover:bg-gray-700 hover:text-white group items-center px-2 py-2 text-base font-medium rounded-md block"
              title="Join as listener"
            >
              <svg class="h-14 w-14 mx-auto" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 20 20" fill="currentColor">
                <path
                  fill-rule="evenodd"
                  d="M10 18a8 8 0 100-16 8 8 0 000 16zM9.555 7.168A1 1 0 008 8v4a1 1 0 001.555.832l3-2a1 1 0 000-1.664l-3-2z"
                  clip-rule="evenodd"
                />
              </svg>
            </a>
          </div>
          <div>
            <a
              href="#"
              title="Chat"
              class="text-gray-300 hover:bg-gray-700 hover:text-white group block px-2 py-2 text-base font-medium rounded-md"
            >
              <svg
                class="h-9 w-9 mx-auto"
                xmlns="http://www.w3.org/2000/svg"
                fill="none"
                viewBox="0 0 24 24"
                stroke="currentColor"
              >
                <path
                  stroke-linecap="round"
                  stroke-linejoin="round"
                  stroke-width="2"
                  d="M8 12h.01M12 12h.01M16 12h.01M21 12c0 4.418-4.03 8-9 8a9.863 9.863 0 01-4.255-.949L3 20l1.395-3.72C3.512 15.042 3 13.574 3 12c0-4.418 4.03-8 9-8s9 3.582 9 8z"
                />
              </svg>
              <!-- <div class="text-gray-300 text-sm text-center">Chat</div> -->
            </a>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import webrtc from '../webrtc'

function listen() {
  webrtc.listen()
}
</script>
