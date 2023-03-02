<template>
  <div class="container mx-auto pt-2 mb-32 sm:mb-24">
    <RouterView></RouterView>
    <ErrorText />
  </div>
</template>

<script setup lang="ts">
import { RouterView } from 'vue-router'
import ErrorText from './components/ErrorText.vue'
</script>
