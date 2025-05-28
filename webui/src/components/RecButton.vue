<template>
  <div
    class="fadeout inline-flex rounded-md shadow-xs px-3 h-9"
    :class="{
      hidden: !State.record.value && !State.user.value.host,
    }"
  >
    <button
      @click="api.record_switch(RecordType.AudioVideo)"
      type="button"
      :class="{
        'bg-red-700 hover:bg-red-600 rounded-l-md': !State.record.value,
        'bg-gray-600 hover:bg-gray-500 rounded-md': State.record.value,
      }"
      class="relative inline-flex items-center px-3 py-2 uppercase text-xs font-semibold text-white hover:bg-red-600 focus:z-10"
      :disabled="!State.user.value.host"
    >
      <span v-if="State.record.value && State.record_type.value === RecordType.AudioVideo" class="font-mono">REC {{ State.record_timer.value }}</span>
      <span v-if="State.record.value && State.record_type.value === RecordType.AudioOnly" class="font-mono">REC Audio {{ State.record_timer.value }}</span>
      <span v-if="!State.record.value" class="font-mono">Record</span>
    </button>
    <Menu v-if="!State.record.value" as="div" class="relative -ml-px block">
      <MenuButton
        class="relative inline-flex items-center rounded-r-md bg-red-700 px-2 py-2 text-white hover:bg-red-600 focus:z-10"
      >
        <span class="sr-only">Open options</span>
        <ChevronDownIcon class="h-5 w-5" aria-hidden="true" />
      </MenuButton>
      <transition
        enter-active-class="transition ease-out duration-100"
        enter-from-class="transform opacity-0 scale-95"
        enter-to-class="transform opacity-100 scale-100"
        leave-active-class="transition ease-in duration-75"
        leave-from-class="transform opacity-100 scale-100"
        leave-to-class="transform opacity-0 scale-95"
      >
        <MenuItems
          class="absolute right-0 z-10 mt-2 -mr-1 w-56 origin-top-right rounded-md bg-white shadow-lg ring-1 ring-black ring-opacity-5 focus:outline-hidden"
        >
          <div class="py-1">
            <MenuItem v-slot="{ active }">
              <button
                @click="api.record_switch(RecordType.AudioOnly)"
                href="#"
                :class="[active ? 'bg-gray-100 text-gray-900' : 'text-gray-700', 'block px-4 py-2 text-sm w-full text-left']"
                >Record Audio only</button>
            </MenuItem>
          </div>
        </MenuItems>
      </transition>
    </Menu>
  </div>
</template>

<script setup lang="ts">
import { Menu, MenuButton, MenuItem, MenuItems } from '@headlessui/vue'
import { ChevronDownIcon } from '@heroicons/vue/20/solid'
import api from '../api'
import { State , RecordType } from '../ws/state'
</script>
