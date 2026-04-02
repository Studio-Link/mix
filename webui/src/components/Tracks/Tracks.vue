<template>
    <ul class="grid grid-cols-1 gap-x-5 gap-y-2 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4">
        <RemoteTrack v-for="(track, index) in remoteTracks" :key="track.id" :pkey="track.id" :idx="index" :error="track.error" />
        <li class="col-span-1" @mouseenter="newTrackVisible = true" @mouseleave="newTrackVisible = false">
            <div class="flex items-center justify-center h-24 md:h-44 mt-4">
                <button accesskey="t" aria-label="Add Remote Track" :class="{
                    'text-sl-disabled': newTrackVisible,
                    'text-sl-01dp': !newTrackVisible,
                }" class="inline-flex items-center rounded-lg px-20 py-12 font-bold text-2xl leading-none uppercase tracking-wide focus:outline-hidden focus:text-sl-disabled"
                    @focus="newTrackVisible = true" @focusout="newTrackVisible = false"
                    @click="api.track_add('remote')">
                    <svg aria-hidden="true" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24"
                        stroke="currentColor" class="h-20 w-20">
                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2"
                            d="M12 6v6m0 0v6m0-6h6m-6 0H6" />
                    </svg>
                </button>
            </div>
        </li>
    </ul>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import RemoteTrack from './RemoteTrack.vue'
import { Tracks } from '../../ws/tracks'
import api from '../../api'

const newTrackVisible = ref(false)
const remoteTracks = Tracks.remote_tracks
</script>
