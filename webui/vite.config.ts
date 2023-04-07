import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import { execSync } from 'child_process'

const commitHash = execSync('git rev-parse --short HEAD')
    .toString();

// https://vitejs.dev/config/
export default defineConfig({
    plugins: [vue()],
    define: {
        APP_VERSION: JSON.stringify("v0.5.1-" + commitHash)
    },
})
