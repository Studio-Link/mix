<template>
  <div class="container mx-auto pt-6 mb-32 sm:mb-24">
    <RouterView></RouterView>
    <Error />
  </div>
</template>

<script setup lang="ts">
import { RouterView } from 'vue-router'
import Error from './components/Error.vue'
</script>
