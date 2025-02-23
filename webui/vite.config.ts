import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import { execSync } from 'child_process'
import tailwindcss from "@tailwindcss/vite";

const commitHash = execSync('git rev-parse --short HEAD')
    .toString();

// https://vitejs.dev/config/
export default defineConfig({
    plugins: [vue(), tailwindcss()],
    define: {
        APP_VERSION: JSON.stringify("v1.0.0-beta-" + commitHash)
    },
    server: {
        proxy: {
            '/api': { target: 'http://127.0.0.1:9999' },
            '/ws': { target: 'ws://127.0.0.1:9999' },
        }
    }
})
