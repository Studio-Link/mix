<template>
    <Menu as="div" class="relative inline-block text-left">
        <div>
            <MenuButton
                class="flex items-center rounded-full text-neutral-500 hover:text-neutral-400 focus:outline-hidden focus:ring-2 focus:ring-sl-primary">
                <span class="sr-only">Open options</span>
                <EllipsisVerticalIcon class="h-6 w-6" aria-hidden="true" />
            </MenuButton>
        </div>

        <transition enter-active-class="transition ease-out duration-100"
            enter-from-class="transform opacity-0 scale-95" enter-to-class="transform opacity-100 scale-100"
            leave-active-class="transition ease-in duration-75" leave-from-class="transform opacity-100 scale-100"
            leave-to-class="transform opacity-0 scale-95">
            <MenuItems
                class="absolute right-0 z-10 mt-2 w-56 origin-top-right divide-y divide-neutral-800 rounded-md shadow-lg ring-1 ring-black ring-opacity-5 bg-sl-06dp text-sl-on_surface_2">
                <div class="py-1">
                    <MenuItem v-slot="{ active }">
                    <a href="#" @click="deleteTrack()" :class="[active ? 'bg-sl-01dp' : '', 'group flex items-center px-4 py-2 text-sm']">

                        <TrashIcon class="mr-3 h-5 w-5 text-neutral-400 group-hover:text-neutral-500"
                            aria-hidden="true" />
                        Delete Track
                    </a>
                    </MenuItem>
                </div>
            </MenuItems>
        </transition>
    </Menu>
</template>

<script setup lang="ts">
import { Menu, MenuButton, MenuItem, MenuItems } from '@headlessui/vue'
import { EllipsisVerticalIcon, TrashIcon } from '@heroicons/vue/20/solid'
import api from '../../api'

const props = defineProps({ 'pkey': { type: Number, required: true } })

function deleteTrack() {
    api.track_del(props.pkey)
}

</script>
