<script setup>
import Speakers from './components/Speakers.vue'
import Listeners from './components/Listeners.vue'
import BottomActions from './components/BottomActions.vue'
</script>

<template>
    <div class="container mx-auto pt-6 mb-32 sm:mb-24">
        <h1 class="text-3xl font-extrabold tracking-tight sm:text-4xl text-center">
            Room 42
        </h1>
        <Speakers />
        <Listeners />
        <BottomActions></BottomActions>
    </div>
</template>
