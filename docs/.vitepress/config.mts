import { defineConfig } from 'vitepress'
import { en } from './en'
import { de } from './de'

// https://vitepress.dev/reference/site-config
export default defineConfig({
    title: "Studio Link - Mix Rooms",
    description: "Mix Rooms",
    cleanUrls: true,
    mpa: true,
    themeConfig: {
        socialLinks: [
            { icon: 'github', link: 'https://github.com/studio-link/mix' },
            { icon: 'mastodon', link: 'https://social.studio.link/@social' }
        ],
        search: {
            provider: 'local'
        }
    },
    locales: {
        root: {
            label: 'English',
            ...en
        },
        de: {
            label: 'Deutsch',
            ...de 
        }
    }

})
