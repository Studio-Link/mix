<template>
    <div class="container mx-auto pt-6 mb-32 sm:mb-24">
        <h1 class="text-3xl font-extrabold tracking-tight sm:text-4xl text-center">
            Studio Link - Release Day 2022
        </h1>
        <Error />
    </div>
    <RouterView></RouterView>
</template>

<script setup lang="ts">
import { RouterView } from 'vue-router'
import Error from './components/Error.vue'
</script>
