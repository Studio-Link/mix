<template>
    <div v-if="Users.user.value.calling">
        <Dialog class="relative z-20" :open=true>
            <div class="fixed inset-0 bg-gray-500/75 transition-opacity" />

            <div class="fixed inset-0 z-10 w-screen overflow-y-auto">
                <div class="flex min-h-full items-end justify-center p-4 text-center sm:items-center sm:p-0">
                    <DialogPanel
                        class="relative transform overflow-hidden rounded-lg bg-white px-4 pt-5 pb-4 text-left shadow-xl transition-all sm:my-8 sm:w-full sm:max-w-lg sm:p-6">
                        <div class="absolute top-0 right-0 hidden pt-4 pr-4 sm:block">
                            <button type="button" @click="api.call_delete(Users.user.value.id)"
                                class="rounded-md bg-white text-gray-400 hover:text-gray-500 focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2 focus:outline-hidden">
                                <span class="sr-only">Close</span>
                                <XMarkIcon class="size-6" aria-hidden="true" />
                            </button>
                        </div>
                        <div class="sm:flex sm:items-start">
                            <div
                                class="mx-auto flex size-12 shrink-0 items-center justify-center rounded-full bg-green-100 sm:mx-0 sm:size-10">
                                <PhoneIcon class="size-6 text-green-600" aria-hidden="true" />
                            </div>
                            <div class="mt-3 text-center sm:mt-0 sm:ml-4 sm:text-left">
                                <DialogTitle as="h3" class="text-base font-semibold text-gray-900">Your call is
                                    waiting...
                                </DialogTitle>
                            </div>
                        </div>
                        <div class="mt-5 sm:mt-4 sm:flex sm:flex-row-reverse">
                            <button type="button"
                                class="mt-3 inline-flex w-full justify-center rounded-md bg-white px-3 py-2 text-sm font-semibold text-gray-900 shadow-xs ring-1 ring-gray-300 ring-inset hover:bg-gray-50 sm:mt-0 sm:w-auto"
                                @click="api.call_delete(Users.user.value.id)">Cancel</button>
                        </div>
                    </DialogPanel>
                </div>
            </div>
        </Dialog>
    </div>
    <div v-if="Users.host_status.value" v-for="user in Users.calls.value" as="template">
        <Dialog class="relative z-20" :open=true>
            <div class="fixed inset-0 bg-gray-500/75 transition-opacity" />

            <div class="fixed inset-0 z-10 w-screen overflow-y-auto">
                <div class="flex min-h-full items-end justify-center p-4 text-center sm:items-center sm:p-0">
                    <DialogPanel
                        class="relative transform overflow-hidden rounded-lg bg-white px-4 pt-5 pb-4 text-left shadow-xl transition-all sm:my-8 sm:w-full sm:max-w-lg sm:p-6">
                        <div class="absolute top-0 right-0 hidden pt-4 pr-4 sm:block">
                            <button type="button" @click="api.call_delete(user.id)"
                                class="rounded-md bg-white text-gray-400 hover:text-gray-500 focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2 focus:outline-hidden">
                                <span class="sr-only">Close</span>
                                <XMarkIcon class="size-6" aria-hidden="true" />
                            </button>
                        </div>
                        <div class="sm:flex sm:items-start">
                            <div
                                class="mx-auto flex size-12 shrink-0 items-center justify-center rounded-full bg-green-100 sm:mx-0 sm:size-10">
                                <PhoneIcon class="size-6 text-green-600" aria-hidden="true" />
                            </div>
                            <div class="mt-3 text-center sm:mt-0 sm:ml-4 sm:text-left">
                                <DialogTitle as="h3" class="text-base font-semibold text-gray-900">Call from {{
                                    user.name }}
                                </DialogTitle>
                            </div>
                        </div>
                        <div class="mt-5 sm:mt-4 sm:flex sm:flex-row-reverse">
                            <button type="button"
                                class="inline-flex w-full justify-center rounded-md bg-green-600 px-3 py-2 text-sm font-semibold text-white shadow-xs hover:bg-red-500 sm:ml-3 sm:w-auto"
                                @click="api.speaker(user.id)">Accept</button>
                            <button type="button"
                                class="mt-3 inline-flex w-full justify-center rounded-md bg-white px-3 py-2 text-sm font-semibold text-gray-900 shadow-xs ring-1 ring-gray-300 ring-inset hover:bg-gray-50 sm:mt-0 sm:w-auto"
                                @click="api.call_delete(user.id)">Cancel</button>
                        </div>
                    </DialogPanel>
                </div>
            </div>
        </Dialog>
    </div>
</template>

<script setup lang="ts">
import { Dialog, DialogPanel, DialogTitle } from '@headlessui/vue'
import { PhoneIcon, XMarkIcon } from '@heroicons/vue/24/outline'
import { Users } from '../ws/users'
import api from '../api'
</script>
