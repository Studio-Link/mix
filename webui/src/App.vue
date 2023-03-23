<template>
    <RouterView></RouterView>
    <ErrorText />
</template>

<script setup lang="ts">
import { RouterView } from 'vue-router'
import ErrorText from './components/ErrorText.vue'
</script>
