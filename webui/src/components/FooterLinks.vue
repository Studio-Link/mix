<template>
<div class="flex justify-center text-sm text-gray-500 mt-10"><span class="font-bold">{{version}}</span></div>
</template>

<script setup lang=ts>
import { ref } from 'vue'
const version = ref(APP_VERSION)
</script>
