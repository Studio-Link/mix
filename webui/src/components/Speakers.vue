<template>
  <div ref="surface" class="h-[70vh] px-2">
    <ul role="list" class="text-center grid gap-2" :class="colsc">
      <li v-for="speaker in Users.speakers.value" :key="speaker.id" class="text-center xl:text-left">
        <canvas :id="'canvas-' + speaker.id" class="mx-auto rounded-lg" alt="" :width="widthc" :height="heightc" />
      </li>
    </ul>
  </div>
</template>

<script setup lang="ts">
import { largestSquare } from 'rect-scaler'
import { ref, onMounted, onUpdated } from 'vue'
import { Users } from '../ws/users'

const surface = ref<HTMLDivElement | null>(null)
const heightc = ref(0)
const widthc = ref(0)
const colsc = ref('grid-cols-1')
let resizeEnd = <NodeJS.Timeout | undefined>undefined

function resize() {
  if (!surface.value) return
  const containerWidth = surface.value.clientWidth
  const containerHeight = surface.value.clientHeight
  const numSquares = Users.speakers.value?.length
  if (!numSquares) return

  // console.log(numSquares, containerWidth, containerHeight)
  const { rows, cols, width, height, area } = largestSquare(containerWidth, containerHeight, numSquares)
  heightc.value = Math.round(height) - 4 /* height - gap-2 */
  widthc.value = Math.round(width) - 4 /* width - gap-2 */
  colsc.value = 'grid-cols-' + cols
  /* console.log(rows, cols, width, height, area, colsc.value) */
}

function canvasLoop() {
  setTimeout(() => {
    requestAnimationFrame(canvasFrame)
  }, 1000 / 25)
}

function canvasFrame() {
  const video = <HTMLVideoElement | null>document.querySelector('video#live')
  const rows = Users.speakers.value?.length
  const width = 1920
  const height = 1080

  if (!rows) {
    canvasLoop()
    return
  }

  Users.speakers.value?.forEach((speaker, idx) => {
    const canvas = <HTMLCanvasElement | null>document.getElementById('canvas-' + speaker.id)
    const ctx = canvas?.getContext('2d')

    if (!video || !canvas || !ctx) {
      canvasLoop()
      return
    }

    ctx.imageSmoothingEnabled = false

    if (rows == 1) {
      ctx.drawImage(video, (width - height) / 2, 0, height, height, 0, 0, canvas.width, canvas.height)
      canvasLoop()
      return
    }

    const w = width / rows
    const h = height / rows
    const x = w * (idx % rows)
    const y = h * (idx / rows)

    ctx.drawImage(video, x, y, w, h, 0, 0, canvas.width, canvas.height)
  })

  canvasLoop()
}

onMounted(() => {
  addEventListener('resize', () => {
    clearTimeout(resizeEnd)
    resizeEnd = setTimeout(resize, 100)
  })
  resize()
  canvasLoop()
})

onUpdated(() => {
  /*@FIXME: avoid duplicate resize events, maybe only if speakers.length change*/
  clearTimeout(resizeEnd)
  resizeEnd = setTimeout(resize, 100)
})
</script>
