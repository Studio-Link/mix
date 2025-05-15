<template>
    <div class="flex min-h-full flex-col justify-center sm:px-6 lg:px-8">
        <div class="sm:mx-auto sm:w-full sm:max-w-md">
            <h2 class="mt-6 text-center text-3xl font-bold tracking-tight text-gray-900">Login</h2>
        </div>

        <div class="mt-8 sm:mx-auto sm:w-full sm:max-w-md">
            <div class="bg-white py-8 px-4 shadow-sm sm:rounded-lg sm:px-10">
                <form class="space-y-6" @submit.prevent="login()">
                    <div id="avatar">
                        <Avatar v-if="random" v-bind="props" class="h-48 mx-auto" />
                    </div>
                    <WebcamPhoto v-if="!random" />
                    <div v-if="random || webcam.picture.value">
                        <label for="name" class="block text-sm font-medium text-gray-700">Name</label>
                        <div class="mt-1">
                            <input id="name" v-model="name" title="Use letters and numbers" name="name" type="text"
                                required pattern="[a-zA-Z0-9 ]+" minlength="3" maxlength="20"
                                class="block w-full appearance-none rounded-md border border-gray-300 px-3 py-2 placeholder-gray-400 shadow-xs focus:border-indigo-500 focus:outline-hidden focus:ring-indigo-500 sm:text-sm"
                                :class="{ 'border-red-300': error }" />
                            <p v-if="error" id="name-error" class="mt-2 text-sm text-red-600">Please enter name</p>
                        </div>
                    </div>
                    <div class="space-y-2">
                        <button type="button" v-if="!random && !webcam.picture.value"
                            class="flex w-full justify-center rounded-md border border-transparent bg-red-700 py-2 px-4 text-sm font-medium text-white shadow-xs hover:bg-red-800 focus:outline-hidden focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
                            @click="randomize()">
                            Cancel
                        </button>
                        <button type="button" v-if="random"
                            class="flex w-full justify-center rounded-md border border-transparent bg-gray-600 py-2 px-4 text-sm font-medium text-white shadow-xs hover:bg-gray-700 focus:outline-hidden focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
                            @click="randomize()">
                            Random Avatar
                        </button>
                        <button type="button" v-if="random || webcam.picture.value"
                            class="flex w-full justify-center rounded-md border border-transparent bg-green-700 py-2 px-4 text-sm font-medium text-white shadow-xs hover:bg-green-800 focus:outline-hidden focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2"
                            @click="activateCam()">
                            Webcam Photo
                        </button>
                    </div>
                    <button v-if="random || webcam.picture.value" type="submit"
                        class="w-full justify-center rounded-md border border-transparent bg-indigo-600 py-2 px-4 text-base font-bold text-white shadow-xs hover:bg-indigo-700 focus:outline-hidden focus:ring-2 focus:ring-indigo-500 focus:ring-offset-2">
                        Join Room
                    </button>
                </form>
                <div class="">
                    <div class="relative mt-10">
                        <div class="absolute inset-0 flex items-center" aria-hidden="true">
                            <div class="w-full border-t border-gray-200"></div>
                        </div>
                        <div class="relative flex justify-center text-sm/6 font-medium">
                            <span class="bg-white px-6 text-gray-900">Or continue with</span>
                        </div>
                    </div>

                    <div class="mt-6 grid grid-cols-1 gap-4">
                        <button @click="router.push({ name: 'Social' })"
                            class="w-full items-center justify-center gap-2 rounded-md bg-white px-32 py-2 text-sm font-semibold text-gray-900 ring-1 shadow-xs ring-gray-300 ring-inset hover:bg-gray-50 focus-visible:ring-transparent">
                            <span class="mb-2 flex justify-center text-xm font-semibold border-b py-1">Social
                                Login</span>
                            <div class="flex gap-2 w-full items-center justify-center" aria-hidden="true">
                                <svg class="size-5" aria-hidden="true" viewBox="0 0 196.52 196.52"
                                    xmlns="http://www.w3.org/2000/svg">
                                    <path
                                        d="m47.924 72.797a18.228 18.228 0 0 1-7.7959 7.7597l42.798 42.965 10.318-5.2291zm56.452 56.67-10.318 5.2291 21.686 21.771a18.228 18.228 0 0 1 7.7975-7.7608z"
                                        fill="#a730b8" />
                                    <path
                                        d="m129.66 102.08 1.7865 11.427 27.415-13.894a18.228 18.228 0 0 1-4.9719-9.8124zm-14.066 7.1282-57.289 29.034a18.228 18.228 0 0 1 4.9728 9.8133l54.103-27.419z"
                                        fill="#5496be" />
                                    <path
                                        d="m69.531 91.654 8.1618 8.1933 29.269-57.139a18.228 18.228 0 0 1-9.787-5.0219zm-7.1897 14.036-14.002 27.335a18.228 18.228 0 0 1 9.786 5.0214l12.378-24.164z"
                                        fill="#ce3d1a" />
                                    <path
                                        d="m39.891 80.676a18.228 18.228 0 0 1-10.866 1.7198l8.1762 52.298a18.228 18.228 0 0 1 10.864-1.7198z"
                                        fill="#d0188f" />
                                    <path
                                        d="m63.326 148.31a18.228 18.228 0 0 1-1.7322 10.863l52.289 8.3907a18.228 18.228 0 0 1 1.7322-10.863z"
                                        fill="#5b36e9" />
                                    <path
                                        d="m134.91 146.92a18.228 18.228 0 0 1 9.788 5.0224l24.134-47.117a18.228 18.228 0 0 1-9.7875-5.0229z"
                                        fill="#30b873" />
                                    <path
                                        d="m126.13 33.16a18.228 18.228 0 0 1-7.7975 7.7608l37.376 37.521a18.228 18.228 0 0 1 7.7969-7.7608z"
                                        fill="#ebe305" />
                                    <path
                                        d="m44.77 51.628a18.228 18.228 0 0 1 4.9723 9.8123l47.248-23.945a18.228 18.228 0 0 1-4.9718-9.8113z"
                                        fill="#f47601" />
                                    <path
                                        d="m118.25 40.964a18.228 18.228 0 0 1-10.851 1.8123l4.1853 26.8 11.42 1.8324zm-4.2333 44.193 9.8955 63.363a18.228 18.228 0 0 1 10.88-1.6278l-9.355-59.904z"
                                        fill="#57c115" />
                                    <path
                                        d="m49.776 61.641a18.228 18.228 0 0 1-1.694 10.869l26.821 4.3077 5.2715-10.294zm45.968 7.382-5.272 10.296 63.371 10.178a18.228 18.228 0 0 1 1.7606-10.859z"
                                        fill="#dbb210" />
                                    <path d="m93.438 23.842a1 1 0 1 0 33.092 1.8025 1 1 0 1 0-33.092-1.8025"
                                        fill="#ffca00" />
                                    <path d="m155.31 85.957a1 1 0 1 0 33.092 1.8025 1 1 0 1 0-33.092-1.8025"
                                        fill="#64ff00" />
                                    <path d="m115.35 163.98a1 1 0 1 0 33.092 1.8025 1 1 0 1 0-33.092-1.8025"
                                        fill="#00a3ff" />
                                    <path d="m28.77 150.09a1 1 0 1 0 33.092 1.8025 1 1 0 1 0-33.092-1.8025"
                                        fill="#9500ff" />
                                    <path d="m15.23 63.478a1 1 0 1 0 33.092 1.8025 1 1 0 1 0-33.092-1.8025"
                                        fill="#f00" />
                                </svg>
                                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 216.4144 232.00976" class="size-5"
                                    aria-hidden="true">
                                    <path fill="#2b90d9"
                                        d="M211.80734 139.0875c-3.18125 16.36625-28.4925 34.2775-57.5625 37.74875-15.15875 1.80875-30.08375 3.47125-45.99875 2.74125-26.0275-1.1925-46.565-6.2125-46.565-6.2125 0 2.53375.15625 4.94625.46875 7.2025 3.38375 25.68625 25.47 27.225 46.39125 27.9425 21.11625.7225 39.91875-5.20625 39.91875-5.20625l.8675 19.09s-14.77 7.93125-41.08125 9.39c-14.50875.7975-32.52375-.365-53.50625-5.91875C9.23234 213.82 1.40609 165.31125.20859 116.09125c-.365-14.61375-.14-28.39375-.14-39.91875 0-50.33 32.97625-65.0825 32.97625-65.0825C49.67234 3.45375 78.20359.2425 107.86484 0h.72875c29.66125.2425 58.21125 3.45375 74.8375 11.09 0 0 32.975 14.7525 32.975 65.0825 0 0 .41375 37.13375-4.59875 62.915" />
                                    <path fill="#fff"
                                        d="M177.50984 80.077v60.94125h-24.14375v-59.15c0-12.46875-5.24625-18.7975-15.74-18.7975-11.6025 0-17.4175 7.5075-17.4175 22.3525v32.37625H96.20734V85.42325c0-14.845-5.81625-22.3525-17.41875-22.3525-10.49375 0-15.74 6.32875-15.74 18.7975v59.15H38.90484V80.077c0-12.455 3.17125-22.3525 9.54125-29.675 6.56875-7.3225 15.17125-11.07625 25.85-11.07625 12.355 0 21.71125 4.74875 27.8975 14.2475l6.01375 10.08125 6.015-10.08125c6.185-9.49875 15.54125-14.2475 27.8975-14.2475 10.6775 0 19.28 3.75375 25.85 11.07625 6.36875 7.3225 9.54 17.22 9.54 29.675" />
                                </svg>
                                <!--
                                <svg version="1.1" viewBox="0 0 50 50" xmlns="http://www.w3.org/2000/svg"
                                    xmlns:xlink="http://www.w3.org/1999/xlink" class="size-5" aria-hidden="true">
                                    <defs>
                                        <rect id="j" y=".11211" width="50" height="49.776" />
                                        <linearGradient id="i" x1="100%" x2="0%" y1="55.807%" y2="60.118%">
                                            <stop stop-color="#FF5C34" offset="0" />
                                            <stop stop-color="#EB0256" offset="1" />
                                        </linearGradient>
                                        <linearGradient id="n" x1="33.089%" x2="68.99%" y1="100%" y2="15.31%">
                                            <stop stop-color="#A63FDB" offset="0" />
                                            <stop stop-color="#FF257E" offset="1" />
                                        </linearGradient>
                                        <linearGradient id="m" x1="14.722%" x2="94.315%" y1="50%" y2="67.526%">
                                            <stop stop-color="#00FFF0" offset="0" />
                                            <stop stop-color="#0087FF" offset="1" />
                                        </linearGradient>
                                        <linearGradient id="l" x1="81.226%" x2="20.815%" y1="10.013%" y2="74.492%">
                                            <stop stop-color="#17C934" offset="0" />
                                            <stop stop-color="#03FF6E" offset="1" />
                                        </linearGradient>
                                        <linearGradient id="h" x1="50%" x2="30.56%" y1="111.91%">
                                            <stop stop-color="#FFB000" offset="0" />
                                            <stop stop-color="#FF7725" offset="1" />
                                        </linearGradient>
                                        <path id="b"
                                            d="m25.88 24.664c-0.36087-6.8014-6.2354-12.026-13.121-11.67-6.8858 0.35644-12.175 6.159-11.814 12.96l0.010371 0.19546c-0.029587-0.49168-0.044591-0.98724-0.044591-1.4863 0-10.156 6.2146-18.878 15.093-22.659l0.52338-0.20887c6.3931-2.5513 13.67 0.49954 16.253 6.8143 2.583 6.3148-0.50574 13.502-6.8989 16.053z" />
                                        <path id="a"
                                            d="m16.339 1.8705 0.18698-0.07462c6.3931-2.5513 13.67 0.49954 16.253 6.8143 2.583 6.3148-0.50574 13.502-6.8989 16.053-0.05343-1.007-0.22773-1.9795-0.50829-2.9043 3.43-1.8567 5.7555-5.4561 5.7555-9.5919 0-6.0367-4.9545-10.93-11.066-10.93-1.3058 0-2.5588 0.2234-3.7219 0.63361z" />
                                        <linearGradient id="q" x1="-81.365%" x2="121.42%" y1="59.623%" y2="72.058%">
                                            <stop stop-color="#9EE85D" offset="0" />
                                            <stop stop-color="#0ED061" offset="1" />
                                        </linearGradient>
                                        <path id="g"
                                            d="m28.379 9.2701 0.18698-0.07462c6.3931-2.5513 13.67 0.49954 16.253 6.8143 2.583 6.3148-0.50574 13.502-6.8989 16.053-0.05343-1.007-0.22773-1.9795-0.50829-2.9043 3.43-1.8567 5.7555-5.4561 5.7555-9.5919 0-6.0367-4.9545-10.93-11.066-10.93-1.3058 0-2.5588 0.2234-3.7219 0.63361z" />
                                        <linearGradient id="p" x1="45.51%" x2="0%" y1="116.82%" y2="-4.0376%">
                                            <stop stop-color="#21EFE3" offset="0" />
                                            <stop stop-color="#2598FF" offset="1" />
                                        </linearGradient>
                                        <path id="f"
                                            d="m25.135 22.732 0.18698-0.07462c6.3931-2.5513 13.67 0.49954 16.253 6.8143 2.583 6.3148-0.50574 13.502-6.8989 16.053-0.05343-1.007-0.22773-1.9795-0.50829-2.9043 3.43-1.8567 5.7555-5.4561 5.7555-9.5919 0-6.0367-4.9545-10.93-11.066-10.93-1.3058 0-2.5588 0.2234-3.7219 0.63361z" />
                                        <linearGradient id="o" x1="100%" x2="-89.649%" y1="58.207%" y2="74.317%">
                                            <stop stop-color="#A63FDB" offset="0" />
                                            <stop stop-color="#FF257E" offset="1" />
                                        </linearGradient>
                                        <path id="e"
                                            d="m10.654 23.765 0.18698-0.07462c6.3931-2.5513 13.67 0.49954 16.253 6.8143 2.583 6.3148-0.50574 13.502-6.8989 16.053-0.05343-1.007-0.22773-1.9795-0.50829-2.9043 3.43-1.8567 5.7555-5.4561 5.7555-9.5919 0-6.0367-4.9545-10.93-11.066-10.93-1.3058 0-2.5588 0.2234-3.7219 0.63361z" />
                                        <path id="d"
                                            d="m5.5459 10.697 0.18698-0.07462c6.3931-2.5513 13.67 0.49954 16.253 6.8143 2.583 6.3148-0.50574 13.502-6.8989 16.053-0.05343-1.007-0.22773-1.9795-0.50829-2.9043 3.43-1.8567 5.7555-5.4561 5.7555-9.5919 0-6.0367-4.9545-10.93-11.066-10.93-1.3058 0-2.5588 0.2234-3.7219 0.63361z" />
                                        <path id="c"
                                            d="m35.632 42.373h4.5448c4.2814 0 7.7522-3.3636 7.7522-7.5129s-3.4708-7.5129-7.7522-7.5129h-6.5595c-2.47 0-4.4724 1.9406-4.4724 4.3344v16.87l6.4871-6.1785z" />
                                        <filter id="r" x="-26.6%" y="-18.9%" width="153.2%" height="147.2%">
                                            <feOffset dx="0" dy="1" in="SourceAlpha" result="shadowOffsetOuter1" />
                                            <feGaussianBlur in="shadowOffsetOuter1" result="shadowBlurOuter1"
                                                stdDeviation="1.5" />
                                            <feColorMatrix in="shadowBlurOuter1"
                                                values="0 0 0 0 0   0 0 0 0 0   0 0 0 0 0  0 0 0 0.298686594 0" />
                                        </filter>
                                    </defs>
                                    <g fill="none" fill-rule="evenodd">
                                        <mask id="k" fill="white">
                                            <use xlink:href="#j" />
                                        </mask>
                                        <g mask="url(#k)">
                                            <g transform="translate(-12.599 -12.101)">
                                                <g transform="translate(37.299 37.253) rotate(40) translate(-25.781 -25)"
                                                    fill-rule="evenodd">
                                                    <path id="s"
                                                        d="m25.88 24.664c-6.6904-1.6477-13.466 2.3738-15.134 8.9822-1.6681 6.6084 2.4033 13.301 9.0937 14.949l0.50617 0.12466c-10.654-2.3814-18.724-11.491-19.391-22.569l-0.010371-0.19546c-0.36087-6.8014 4.9286-12.604 11.814-12.96 6.8858-0.35644 12.76 4.8682 13.121 11.67z"
                                                        fill="url(#i)" />
                                                    <path
                                                        d="m25.88 24.664c-3.5513 5.8379-1.6389 13.414 4.2715 16.922s13.581 1.6188 17.132-4.2191l0.23527-0.38675c-4.3151 7.3807-12.39 12.348-21.639 12.348-1.902 0-3.7544-0.21006-5.5346-0.60796l-0.50617-0.12466c-6.6904-1.6477-10.762-8.3405-9.0937-14.949 1.6681-6.6084 8.444-10.63 15.134-8.9822z"
                                                        fill="url(#n)" />
                                                    <path
                                                        d="m25.88 24.664c4.4322 5.2173 12.307 5.8978 17.589 1.52s5.971-12.156 1.5389-17.373l-0.10149-0.11946c3.7066 4.3034 5.9433 9.8802 5.9433 15.973 0 4.4858-1.2124 8.6919-3.3312 12.316l-0.23527 0.38675c-3.5513 5.8379-11.222 7.7268-17.132 4.2191s-7.8228-11.084-4.2715-16.922z"
                                                        fill="url(#m)" />
                                                    <path
                                                        d="m25.88 24.664c6.3931-2.5513 9.4819-9.7387 6.8989-16.053-2.583-6.3148-9.8596-9.3656-16.253-6.8143l-0.52338 0.20887c3.0294-1.2901 6.3688-2.0048 9.8773-2.0048 7.622 0 14.446 3.3732 19.027 8.6907l0.10149 0.11946c4.4322 5.2173 3.7432 12.996-1.5389 17.373s-13.157 3.6973-17.589-1.52z"
                                                        fill="url(#l)" />
                                                    <g fill="url(#h)">
                                                        <use xlink:href="#b" />
                                                        <use fill-opacity=".1" style="mix-blend-mode:multiply"
                                                            xlink:href="#b" />
                                                    </g>
                                                    <g opacity=".50491">
                                                        <use fill="url(#h)" xlink:href="#a" />
                                                        <use fill="#000000" fill-opacity=".49618"
                                                            style="mix-blend-mode:overlay" xlink:href="#a" />
                                                    </g>
                                                    <g transform="translate(37.056 20.179) rotate(72) translate(-37.056 -20.179)"
                                                        opacity=".54425">
                                                        <use fill="url(#q)" xlink:href="#g" />
                                                        <use fill="#000000" fill-opacity=".49989"
                                                            style="mix-blend-mode:overlay" xlink:href="#g" />
                                                    </g>
                                                    <g transform="translate(33.811 33.641) rotate(143) translate(-33.811 -33.641)"
                                                        opacity=".56222">
                                                        <use fill="url(#p)" xlink:href="#f" />
                                                        <use fill="#000000" style="mix-blend-mode:overlay"
                                                            xlink:href="#f" />
                                                    </g>
                                                    <g transform="translate(19.33 34.673) rotate(217) translate(-19.33 -34.673)"
                                                        opacity=".58415">
                                                        <use fill="url(#o)" xlink:href="#e" />
                                                        <use fill="#000000" fill-opacity=".50309"
                                                            style="mix-blend-mode:overlay" xlink:href="#e" />
                                                    </g>
                                                    <g transform="translate(14.222 21.606) rotate(-71) translate(-14.222 -21.606)"
                                                        opacity=".18013">
                                                        <use fill="url(#i)" xlink:href="#d" />
                                                        <use fill="#000000" fill-opacity=".77284"
                                                            style="mix-blend-mode:multiply" xlink:href="#d" />
                                                    </g>
                                                </g>
                                                <g fill-rule="nonzero">
                                                    <use fill="black" filter="url(#r)" xlink:href="#c" />
                                                    <use fill="#FFFFFF" xlink:href="#c" />
                                                </g>
                                            </g>
                                        </g>
                                    </g>
                                </svg>

                                <svg class="size-5" aria-hidden="true" version="1.1" viewBox="0 0 600 530"
                                    xmlns="http://www.w3.org/2000/svg">
                                    <path
                                        d="m135.72 44.03c66.496 49.921 138.02 151.14 164.28 205.46 26.262-54.316 97.782-155.54 164.28-205.46 47.98-36.021 125.72-63.892 125.72 24.795 0 17.712-10.155 148.79-16.111 170.07-20.703 73.984-96.144 92.854-163.25 81.433 117.3 19.964 147.14 86.092 82.697 152.22-122.39 125.59-175.91-31.511-189.63-71.766-2.514-7.3797-3.6904-10.832-3.7077-7.8964-0.0174-2.9357-1.1937 0.51669-3.7077 7.8964-13.714 40.255-67.233 197.36-189.63 71.766-64.444-66.128-34.605-132.26 82.697-152.22-67.108 11.421-142.55-7.4491-163.25-81.433-5.9562-21.282-16.111-152.36-16.111-170.07 0-88.687 77.742-60.816 125.72-24.795z"
                                        fill="#1185fe" />
                                </svg>

                                <img src="/sendegate.png" class="size-5" aria-hidden="true" />
                                -->
                            </div>
                        </button>
                    </div>
                </div>
            </div>
        </div>
        <FooterLinks />
    </div>
</template>

<script setup lang="ts">
import { Avatar, Factory } from 'vue3-avataaars'
import { ref, onMounted } from 'vue'
import WebcamPhoto from '../components/WebcamPhoto.vue'
import FooterLinks from '../components/FooterLinks.vue'
import webcam from '../webcam'
import api from '../api'
import router from '../router'

/*
const vprops = defineProps({ token: String })
api.connect(vprops?.token)
*/

const error = ref(false)
const name = ref('')
const random = ref(true)
const props = ref(Factory({ isCircle: true }))

onMounted(() => {
    randomize()
})

function randomize() {
    webcam.stop()
    random.value = true
    const mouths = ['Smile', 'Twinkle', 'Tongue', 'Default']
    const mouth: string = mouths[Math.floor(Math.random() * mouths.length)]
    const eyes = ['EyeRoll', 'Happy', 'Hearts', 'Side', 'Squint', 'Wink', 'Default']
    const eye: string = eyes[Math.floor(Math.random() * eyes.length)]
    props.value = Factory({ isCircle: true, mouth: mouth, facialHair: 'Blank', eyes: eye })
}

function activateCam() {
    webcam.picture.value = undefined
    random.value = false
    webcam.start()
}

function login() {
    if (!name.value) {
        error.value = true
        return
    }
    error.value = false

    if (webcam.picture.value) {
        api.login(name.value, webcam.picture.value)
    } else {
        const svg = document.querySelector('#avatar svg')
        svg?.setAttribute('width', String(256))
        svg?.setAttribute('height', String(256))
        const xml = new XMLSerializer().serializeToString(svg!)
        const blob = new Blob([xml], { type: 'image/svg+xml' })
        const canvas = document.createElement('canvas')
        const URL = window.URL || window.webkitURL || window
        canvas.width = 256
        canvas.height = 256

        const image = new Image()
        image.onload = () => {
            canvas.getContext('2d')?.drawImage(image, 0, 0)
            api.login(name.value, canvas.toDataURL('image/png'))
        }
        image.src = URL.createObjectURL(blob)
    }
}
</script>
