<template>
  <main>
    <video id="live" playsinline autoplay></video>
    <audio id="live" autoplay></audio>
    <Speakers />
    <Listeners />
  </main>
  <BottomActions v-if="!Webrtc.errorText.value" />
</template>

<script setup lang="ts">
import Speakers from '../components/Speakers.vue'
import Listeners from '../components/Listeners.vue'
import BottomActions from '../components/BottomActions.vue'
import {Webrtc} from '../webrtc'
</script>
