<template>
  <div
    class="fadeout inline-flex rounded-md shadow-sm px-3 h-9"
    :class="{
      hidden: !Users.record.value && !Users.host_status.value,
    }"
  >
    <button
      @click="api.record_switch(RecordType.AudioVideo)"
      type="button"
      :class="{
        'bg-red-700 hover:bg-red-600 rounded-l-md': !Users.record.value,
        'bg-gray-600 hover:bg-gray-500 rounded-md': Users.record.value,
      }"
      class="relative inline-flex items-center px-3 py-2 uppercase text-xs font-semibold text-white hover:bg-red-600 focus:z-10"
      :disabled="!Users.host_status.value"
    >
      <span v-if="Users.record.value && Users.record_type.value === RecordType.AudioVideo" class="font-mono">REC {{ Users.record_timer.value }}</span>
      <span v-if="Users.record.value && Users.record_type.value === RecordType.AudioOnly" class="font-mono">REC Audio {{ Users.record_timer.value }}</span>
      <span v-if="!Users.record.value" class="font-mono">Record</span>
    </button>
    <Menu v-if="!Users.record.value" as="div" class="relative -ml-px block">
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
          class="absolute right-0 z-10 mt-2 -mr-1 w-56 origin-top-right rounded-md bg-white shadow-lg ring-1 ring-black ring-opacity-5 focus:outline-none"
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
import { Users, RecordType } from '../ws/users'
</script>
