<template>
    <div class="container mx-auto pt-6 mb-32 sm:mb-24">
        <h1 class="text-3xl font-extrabold tracking-tight sm:text-4xl text-center">
            Room 42
        </h1>
        <div :class="{'blur pointer-events-none': Webrtc.state.value < WebrtcState.Listening}">
            <video id="live" playsinline autoplay></video>
            <audio id="live" autoplay></audio>
            <Speakers />
            <Listeners />
        </div>
        <BottomActions v-if="!Webrtc.errorText.value"/>
        <Error />
    </div>
</template>

<script setup lang="ts">
import Speakers from './components/Speakers.vue'
import Listeners from './components/Listeners.vue'
import BottomActions from './components/BottomActions.vue'
import Error from './components/Error.vue'
import {Webrtc, WebrtcState} from './webrtc'
</script>
