<template>
    <div v-if="isActive() && isNoCall()" class="flex-1 px-4 grid grid-cols-1">
        <label :for="pkey.toString()" class="block text-sm font-medium leading-5 text-sl-on_surface_2">Enter
            Partner ID</label>
        <div class="mt-1 relative rounded-md shadow-xs">
            <input @keyup.enter="api.track_dial(pkey, peer)" :id="pkey.toString()" v-model="peer" ref="slid" type="text"
                class="form-input block w-full sm:text-sm sm:leading-5 text-sl-on_surface_1 bg-sl-surface mb-2 border-none focus:ring-sl-primary rounded-lg"
                placeholder="xyz@studio.link" />
                <p class="text-sm text-red-500">{{ error }}</p>
        </div>
        <div class="mt-2 flex justify-between items-center">
            <ButtonPrimary @click="api.track_dial(pkey, peer)">
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 20 20" fill="currentColor"
                    class="h-4 mr-1">
                    <path
                        d="M17.924 2.617a.997.997 0 00-.215-.322l-.004-.004A.997.997 0 0017 2h-4a1 1 0 100 2h1.586l-3.293 3.293a1 1 0 001.414 1.414L16 5.414V7a1 1 0 102 0V3a.997.997 0 00-.076-.383z" />
                    <path
                        d="M2 3a1 1 0 011-1h2.153a1 1 0 01.986.836l.74 4.435a1 1 0 01-.54 1.06l-1.548.773a11.037 11.037 0 006.105 6.105l.774-1.548a1 1 0 011.059-.54l4.435.74a1 1 0 01.836.986V17a1 1 0 01-1 1h-2C7.82 18 2 12.18 2 5V3z" />
                </svg>
                Call
            </ButtonPrimary>
            <!--
            <span class="text-sl-on_surface_2">OR</span>
            <ButtonSecondary>
                <svg xmlns="http://www.w3.org/2000/svg" class="h-4 mr-1" viewBox="0 0 20 20"
                    fill="currentColor">
                    <path fill-rule="evenodd"
                        d="M12.586 4.586a2 2 0 112.828 2.828l-3 3a2 2 0 01-2.828 0 1 1 0 00-1.414 1.414 4 4 0 005.656 0l3-3a4 4 0 00-5.656-5.656l-1.5 1.5a1 1 0 101.414 1.414l1.5-1.5zm-5 5a2 2 0 012.828 0 1 1 0 101.414-1.414 4 4 0 00-5.656 0l-3 3a4 4 0 105.656 5.656l1.5-1.5a1 1 0 10-1.414-1.414l-1.5 1.5a2 2 0 11-2.828-2.828l3-3z"
                        clip-rule="evenodd" />
                </svg>Invite
            </ButtonSecondary>
            -->
        </div>
    </div>
    <div v-if="!isActive() && isNoCall()" class="text-center mt-10 text-sl-disabled">No call</div>
    <div v-if="isIncoming()" class="text-center mt-2 text-sl-disabled">
        <div class="mb-4 truncate animate-pulse">Incoming call from <br />{{ peerName() }}</div>
        <ButtonPrimary @click="api.track_accept(pkey)" class="mr-2">
            Accept
        </ButtonPrimary>
        <ButtonSecondary @click="api.track_hangup(pkey)">
            Cancel 
        </ButtonSecondary>
    </div>
    <div v-if="isCalling()" class="text-center mt-2 text-sl-disabled">
        <div class="mb-4 truncate animate-pulse">Calling <br />{{ peerName() }}</div>
        <ButtonSecondary @click="api.track_hangup(pkey)">
            Hangup
        </ButtonSecondary>
    </div>
    <div v-if="isOnCall()" class="text-center">
        <video></video>
        <svg class="w-24 h-24 fill-yellow-800 mx-auto"
        xmlns="http://www.w3.org/2000/svg" viewBox="0 0 448 512">
        <path
        d="M224 32C135.6 32 64 103.6 64 192v16c0 8.8-7.2 16-16 16s-16-7.2-16-16V192C32 86 118 0 224 0S416 86 416 192v16c0 61.9-50.1 112-112 112H240 224 208c-17.7 0-32-14.3-32-32s14.3-32 32-32h32c17.7 0 32 14.3 32 32h32c44.2 0 80-35.8 80-80V192c0-88.4-71.6-160-160-160zM96 192c0-70.7 57.3-128 128-128s128 57.3 128 128c0 13.9-2.2 27.3-6.3 39.8C337.4 246.3 321.8 256 304 256h-8.6c-11.1-19.1-31.7-32-55.4-32H208c-35.3 0-64 28.7-64 64c0 1.4 0 2.7 .1 4C114.8 268.6 96 232.5 96 192zM224 352h16 64 9.6C387.8 352 448 412.2 448 486.4c0 14.1-11.5 25.6-25.6 25.6H25.6C11.5 512 0 500.5 0 486.4C0 412.2 60.2 352 134.4 352H208h16z" />
        </svg>
        <ButtonSecondary v-if="isActive()" class="mt-2" @click="api.track_hangup(pkey)">
        Hangup
        </ButtonSecondary>
    </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { Tracks, TrackStatus } from '../../ws/tracks'
import api from '../../api'

const props = defineProps({
'pkey': { type: Number, required: true },
'idx': { type: Number, required: true },
'error': { type: String, required: true }
})

const peer = ref("");

function isActive() {
    return Tracks.isSelected(props.pkey)
}

function isOnCall() {
    return Tracks.remote_tracks[props.idx].status === TrackStatus.REMOTE_CONNECTED
}

function isNoCall() {
    return Tracks.remote_tracks[props.idx].status === TrackStatus.IDLE
}

function isCalling() {
    return Tracks.remote_tracks[props.idx].status === TrackStatus.REMOTE_CALLING
}

function isIncoming() {
    return Tracks.remote_tracks[props.idx].status === TrackStatus.REMOTE_INCOMING
}

function peerName() {
    return Tracks.getTrackName(props.pkey);
}

</script>
