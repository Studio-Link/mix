<template>
  <!-- Static sidebar for desktop -->
  <div class="flex flex-shrink-0 mr-2 bg-gray-600 min-h-screen">
    <div class="flex w-16 flex-col h-screen">
      <div class="flex min-h-0 flex-1 flex-col overflow-y-auto bg-sl-01dpa">
        <div class="flex-1">
          <nav aria-label="Sidebar" class="flex flex-col items-center space-y-3 py-6">
            <a href="#" class="relative block group -mt-2">
              <div class="absolute flex items-center h-full -left-2">
                <div class="h-10 w-1 transition-all duration-200 origin-left bg-sl-primary rounded-r"></div>
              </div>
              <div class="group-active:translate-y-px">
                <div
                  class="flex items-center justify-center w-12 h-12 transition-all duration-200 overflow-hidden rounded-2xl text-white hover:bg-sl-08dpa"
                >
                  <img class="h-10" src="https://source.unsplash.com/TCpfPxKPOvk/100x100" alt="Studio Link - Default" />
                  <span class="sr-only">Studio Link Default</span>
                </div>
              </div>
            </a>
            <hr class="w-2/3 border-t-white/[.06] border-t-2 rounded mx-2" />
            <a href="#" class="relative block group">
              <div class="absolute flex items-center h-full -left-2">
                <div
                  class="h-5 scale-0 opacity-0 group-hover:opacity-100 group-hover:scale-100 w-1 transition-all duration-200 origin-left bg-sl-primary rounded-r"
                ></div>
              </div>
              <div class="group-active:translate-y-px">
                <div
                  class="text-gray-100 group-hover:rounded-2xl group-hover:bg-brand group-hover:text-white bg-sl-02dpa rounded-3xl flex items-center justify-center w-12 h-12 transition-all duration-200 overflow-hidden"
                >
                  <img alt="Studio" class="rounded-lg" src="https://source.unsplash.com/random/100x100" />
                  <span class="sr-only">Podstock</span>
                </div>
              </div>
            </a>
            <a href="#" class="relative block group">
              <div class="absolute flex items-center h-full -left-2">
                <div
                  class="h-5 scale-0 opacity-0 group-hover:opacity-100 group-hover:scale-100 w-1 transition-all duration-200 origin-left bg-sl-primary rounded-r"
                ></div>
              </div>
              <div class="group-active:translate-y-px">
                <div
                  class="text-gray-100 group-hover:rounded-2xl group-hover:bg-brand group-hover:text-white bg-sl-02dpa rounded-3xl flex items-center justify-center w-12 h-12 transition-all duration-200 overflow-hidden"
                >
                  <img alt="Studio" class="rounded-lg" src="https://source.unsplash.com/WLUHO9A_xik/100x100" />
                  <span class="sr-only">Podstock</span>
                </div>
              </div>
            </a>
          </nav>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { BookmarkSquareIcon, FireIcon, HomeIcon, InboxIcon, UserIcon } from '@heroicons/vue/24/outline'

const navigation = [
  { name: 'Home', href: '#', icon: HomeIcon },
  { name: 'Trending', href: '#', icon: FireIcon },
  { name: 'Bookmarks', href: '#', icon: BookmarkSquareIcon },
  { name: 'Messages', href: '#', icon: InboxIcon },
  { name: 'Profile', href: '#', icon: UserIcon },
]
</script>
