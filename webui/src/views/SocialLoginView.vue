<template>
    <div class="relative z-10" role="dialog" aria-modal="true">
        <!--
    Background backdrop, show/hide based on modal state.

    Entering: "ease-out duration-300"
      From: "opacity-0"
      To: "opacity-100"
    Leaving: "ease-in duration-200"
      From: "opacity-100"
      To: "opacity-0"
  -->
        <div class="fixed inset-0 bg-gray-500/25 transition-opacity" aria-hidden="true"></div>

        <div class="fixed inset-0 z-10 w-screen overflow-y-auto p-4 sm:p-6 md:p-20">
            <!--
      Command palette, show/hide based on modal state.

      Entering: "ease-out duration-300"
        From: "opacity-0 scale-95"
        To: "opacity-100 scale-100"
      Leaving: "ease-in duration-200"
        From: "opacity-100 scale-100"
        To: "opacity-0 scale-95"
    -->
            <div
                class="mx-auto max-w-3xl transform divide-y divide-gray-100 overflow-hidden rounded-xl bg-white ring-1 shadow-2xl ring-black/5 transition-all">
                <div class="grid grid-cols-1">
                    <input v-model="profile" autofocus type="text"
                        class="col-start-1 row-start-1 h-12 w-full pr-4 pl-11 text-base text-gray-900 outline-hidden placeholder:text-gray-400 sm:text-sm"
                        placeholder="@me@mastodon.social" role="combobox" aria-expanded="false"
                        aria-controls="options" />
                    <LinkIcon class="pointer-events-none col-start-1 row-start-1 ml-4 size-5 self-center text-gray-400"
                        aria-hidden="true" />
                </div>

                <div v-if="state === State.Ready" class="flex transform-gpu content-center divide-x divide-gray-100">
                    <!-- Active item side-panel, show/hide based on active state -->
                    <div
                        class="mx-auto h-96 w-1/2 flex-none flex-col divide-y divide-gray-100 overflow-y-auto sm:flex">
                        <div class="flex-none p-6 text-center">
                            <img :src="avatar" alt="" class="mx-auto size-32 rounded-full" />
                            <h2 class="mt-3 font-semibold text-gray-900">{{name}}</h2>
                            <p class="text-sm/6 text-gray-500">{{summary}}</p>
                        </div>
                        <div class="flex flex-auto flex-col justify-between p-6">
                            <button type="button"
                                class="w-full rounded-md bg-indigo-600 px-3 py-2 text-sm font-semibold text-white shadow-xs hover:bg-indigo-500 focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-indigo-600">Join
                                Room</button>
                        </div>
                    </div>
                </div>
                <div v-if="state === State.NotFound" class="px-6 py-14 text-center text-sm sm:px-14">
                    Not Found...
                </div>

                <!-- Empty state, show/hide based on command palette state -->
                <div v-if="state === State.Empty" class="px-6 py-14 text-center text-sm sm:px-14">
                    <svg class="mx-auto size-6 text-gray-400" fill="none" viewBox="0 0 24 24" stroke-width="1.5"
                        stroke="currentColor" aria-hidden="true" data-slot="icon">
                        <path stroke-linecap="round" stroke-linejoin="round"
                            d="M15 19.128a9.38 9.38 0 0 0 2.625.372 9.337 9.337 0 0 0 4.121-.952 4.125 4.125 0 0 0-7.533-2.493M15 19.128v-.003c0-1.113-.285-2.16-.786-3.07M15 19.128v.106A12.318 12.318 0 0 1 8.624 21c-2.331 0-4.512-.645-6.374-1.766l-.001-.109a6.375 6.375 0 0 1 11.964-3.07M12 6.375a3.375 3.375 0 1 1-6.75 0 3.375 3.375 0 0 1 6.75 0Zm8.25 2.25a2.625 2.625 0 1 1-5.25 0 2.625 2.625 0 0 1 5.25 0Z" />
                    </svg>
                    <p class="mt-4 font-semibold text-gray-900">No social profile found</p>
                    <p class="mt-2 text-gray-500">Please enter a valid Social URL or Account</p>

                    <div class="mt-4 mb-2"><b>Examples</b></div>
                    <fieldset aria-label="Server size">
                        <div class="space-y-4">
                            <!-- Active: "border-indigo-600 ring-2 ring-indigo-600", Not Active: "border-gray-300" -->
                            <label aria-label="Mastodon" @click=""
                                class="relative  cursor-pointer rounded-lg border bg-white px-2 py-4 shadow-xs focus:outline-hidden flex">
                                <input type="radio" name="server-size" value="Mastodon" class="sr-only">
                                <span class="mr-2 flex text-sm sm:mt-0 sm:ml-4 sm:flex-col sm:text-right">

                                    <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 216.4144 232.00976"
                                        class="size-5" aria-hidden="true">
                                        <path fill="#2b90d9"
                                            d="M211.80734 139.0875c-3.18125 16.36625-28.4925 34.2775-57.5625 37.74875-15.15875 1.80875-30.08375 3.47125-45.99875 2.74125-26.0275-1.1925-46.565-6.2125-46.565-6.2125 0 2.53375.15625 4.94625.46875 7.2025 3.38375 25.68625 25.47 27.225 46.39125 27.9425 21.11625.7225 39.91875-5.20625 39.91875-5.20625l.8675 19.09s-14.77 7.93125-41.08125 9.39c-14.50875.7975-32.52375-.365-53.50625-5.91875C9.23234 213.82 1.40609 165.31125.20859 116.09125c-.365-14.61375-.14-28.39375-.14-39.91875 0-50.33 32.97625-65.0825 32.97625-65.0825C49.67234 3.45375 78.20359.2425 107.86484 0h.72875c29.66125.2425 58.21125 3.45375 74.8375 11.09 0 0 32.975 14.7525 32.975 65.0825 0 0 .41375 37.13375-4.59875 62.915" />
                                        <path fill="#fff"
                                            d="M177.50984 80.077v60.94125h-24.14375v-59.15c0-12.46875-5.24625-18.7975-15.74-18.7975-11.6025 0-17.4175 7.5075-17.4175 22.3525v32.37625H96.20734V85.42325c0-14.845-5.81625-22.3525-17.41875-22.3525-10.49375 0-15.74 6.32875-15.74 18.7975v59.15H38.90484V80.077c0-12.455 3.17125-22.3525 9.54125-29.675 6.56875-7.3225 15.17125-11.07625 25.85-11.07625 12.355 0 21.71125 4.74875 27.8975 14.2475l6.01375 10.08125 6.015-10.08125c6.185-9.49875 15.54125-14.2475 27.8975-14.2475 10.6775 0 19.28 3.75375 25.85 11.07625 6.36875 7.3225 9.54 17.22 9.54 29.675" />
                                    </svg>
                                </span>
                                <span class="flex items-center">
                                    <span class="flex flex-col text-sm">
                                        <span class="font-bold text-gray-900">Mastodon - @me@mastdon.social</span>
                                    </span>
                                </span>
                                <!--
        Active: "border", Not Active: "border-2"
        Checked: "border-indigo-600", Not Checked: "border-transparent"
      -->
                                <span class="pointer-events-none absolute -inset-px rounded-lg border-2"
                                    aria-hidden="true"></span>
                            </label>
                        </div>
                    </fieldset>


                </div>
                <div v-if="state === State.Loading" class="px-6 py-14 text-center text-sm sm:px-14">
                    <div class="flex animate-pulse space-x-4">
                        <div class="size-10 rounded-full bg-gray-200"></div>
                        <div class="flex-1 space-y-6 py-1">
                            <div class="h-2 rounded bg-gray-200"></div>
                            <div class="space-y-3">
                                <div class="grid grid-cols-3 gap-4">
                                    <div class="col-span-2 h-2 rounded bg-gray-200"></div>
                                    <div class="col-span-1 h-2 rounded bg-gray-200"></div>
                                </div>
                                <div class="h-2 rounded bg-gray-200"></div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { LinkIcon } from '@heroicons/vue/24/outline'
import { watchDebounced } from '@vueuse/core'
import api from '../api'

enum State {
    NotFound = -1,
    Empty,
    Loading,
    Ready
}
let state = ref(State.Empty)
let profile = ref("")
let avatar = ref("/avatars/default.png")
let name = ref("...")
let summary = ref("...")

watchDebounced(profile, async () => {
    if (!profile.value) {
        state.value = State.Empty
        return
    }
    state.value = State.Loading
    let json = await api.social(profile.value)
    console.log(json)
    if (json.status === 404) {
        state.value = State.NotFound
    }
    if (json.status === 200) {
        avatar.value = "/avatars/" + json.id + ".webp?" + new Date().getTime()
        name.value = json.name
        summary.value = json.summary.replace(/<\/?[^>]+(>|$)/g, "");
        state.value = State.Ready
    }
}, { debounce: 600, maxWait: 5000 })
</script>
