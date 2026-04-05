<template>
    <li aria-label="Remote track" class="aspect-video col-span-1" @mouseenter="setActive()" @touchstart.passive="setActive()">
        <div class="flex justify-between h-5">
            <h2 class="ml-1 font-semibold text-sl-disabled text-sm truncate pr-2">{{ getTrackName() }}</h2>
            <div class="flex">
                <div class="font-semibold text-sm text-sl-disabled uppercase text-right"></div>
            </div>
            <!-- <div class="font-semibold text-sm text-yellow-500 uppercase">Calling</div> -->
            <!-- <div class="font-semibold text-sm text-red-500 uppercase">Error</div> -->
            <!-- <div class="font-semibold text-sm text-sl-green uppercase">Connected</div> -->
            <!-- <div class="font-semibold text-sm text-sl-yellow uppercase">Calling</div> -->
            <!-- <div class="font-semibold text-sm text-sl-red uppercase">Error</div> -->
        </div>

        <div class="flex mt-1 border-red-500" :class="[isFocused() ? 'border-red-700 border-2' : '']">
            <div class="relative bg-sl-02dpa rounded-lg min-h-[12em] w-full shadow-sm pb-2">
                <div class="flex justify-between items-center">
                    <div :id="`track${pkey}`" ref="tid" tabindex="0" @focus="setActive()"
                        :class="[isActive() ? 'bg-sl-disabled' : 'bg-sl-24dpa']"
                        class="inline-flex items-center justify-center ml-2 text-sm leading-none text-black font-bold rounded-full px-2 py-1 focus:outline-hidden">
                        <span class="sr-only">Remote Track</span>
                        {{ pkey }}
                        <span class="sr-only">selected</span>
                    </div>
                    <div class="shrink-0 pr-2 text-right mt-2 w-8 h-8">
                        <RemoteTrackSettings v-if="isActive()" :pkey="props.pkey" />
                    </div>
                </div>
                <RemoteTrackCall :pkey="props.pkey" :idx="props.idx" :error="props.error" />
                <div class="absolute bottom-0 right-0 mr-0.5 text-xs">
                    <button class="opacity-30 hover:opacity-60 focus:opacity-60" title="Remove track"
                        @click="deleteRemoteTrack()" @focus="setActive()">
                        <svg v-if="isActive()" class="w-5 h-5" viewBox="0 0 20 20" fill="currentColor">
                            <path fill-rule="evenodd"
                                d="M4.293 4.293a1 1 0 011.414 0L10 8.586l4.293-4.293a1 1 0 111.414 1.414L11.414 10l4.293 4.293a1 1 0 01-1.414 1.414L10 11.414l-4.293 4.293a1 1 0 01-1.414-1.414L8.586 10 4.293 5.707a1 1 0 010-1.414z"
                                clip-rule="evenodd" />
                        </svg>
                    </button>
                </div>
            </div>

            <div class="flex w-5 items-end ml-0.5 opacity-60" aria-hidden="true">
                <div id="levels" class="levels">
                    <div :id="'level'+(pkey+pkey-1)" class="level"></div>
                    <div :id="'level'+(pkey+pkey)" class="level"></div>
                </div>
            </div>
        </div>
    </li>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import RemoteTrackSettings from './RemoteTrackSettings.vue'
import RemoteTrackCall from './RemoteTrackCall.vue'
import { Tracks } from '../../ws/tracks'
import api from '../../api'

const props = defineProps({ 
    'pkey': { type: Number, required: true },
    'idx': { type: Number, required: true },
    'error': { type: String, required: true },
})

const tid = ref<HTMLDivElement>()

onMounted(() => {
    tid.value?.focus()
})

function isActive() {
    return Tracks.isSelected(props.pkey)
}

function isFocused() {
    return Tracks.isFocused(props.pkey)
}

function setActive() {
    Tracks.select(props.pkey)
}

function getTrackName() {
    return Tracks.getTrackName(props.pkey)
}

function deleteRemoteTrack() {
    api.track_del(props.pkey)
}
</script>
