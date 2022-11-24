<template>
  <div class="container mx-auto pt-6 mb-32 sm:mb-24">
    <h1 class="text-xl font-bold tracking-tight sm:text-3xl text-center">Studio Link - Release Day 2022</h1>
    <Error />
    <RouterView></RouterView>
  </div>
</template>

<script setup lang="ts">
import { RouterView } from 'vue-router'
import Error from './components/Error.vue'
</script>
