import { defineConfig, type DefaultTheme } from 'vitepress'

export const en = defineConfig({
    lang: 'en-US',
    description: 'Mix Rooms',

    themeConfig: {
        nav: nav(),

        sidebar: {
            '/hosted/': { base: '/hosted/', items: sidebarHosted() },
            '/self-hosting/': { base: '/self-hosting/', items: sidebarSelfHosted() }
        },

        footer: {
            message: 'Released under the MIT License. <br> <a href="https://studio-link.de/impressum.html">About</a> | <a href="https://studio-link.de/datenschutz_en.html">Privacy Policy</a>',
            copyright: 'Copyright © 2013-present IT-Service Sebastian Reimers'
        }
    }
})

function nav(): DefaultTheme.NavItem[] {
    return [
        {
            text: 'Get started',
            link: '/hosted/started',
            activeMatch: '/hosted/'
        },
        {
            text: 'Open Source',
            link: '/self-hosting/install-intro',
            activeMatch: '/self-hosting/'
        },
        {
            text: '0.6.0-beta',
            items: [
                {
                    text: 'Changelog',
                    link: '/changelog'
                }
            ]
        }
    ]
}

function sidebarHosted(): DefaultTheme.SidebarItem[] {
    return [
        {
            text: 'Introduction',
            collapsed: false,
            items: [
                { text: 'What is Mix Rooms?', link: 'what-is-mix-rooms' },
                { text: 'Getting Started', link: 'started' },
            ]
        },
        {
            text: 'Usage - Howto',
            collapsed: false,
            base: '/hosted/howto/',
            items: [
                { text: 'Overview', link: 'overview' },
                { text: 'Login', link: 'login' },
                { text: 'Recording', link: 'recording' },
                { text: 'Chat', link: 'chat' },
            ]
        },
    ]
}

function sidebarSelfHosted(): DefaultTheme.SidebarItem[] {
    return [
        {
            text: 'Self-Hosting',
            items: [
                {
                    text: 'Installation',
                    base: '/self-hosting/install-',
                    collapsed: false,
                    items: [
                        { text: 'Introduction', link: 'intro' },
                        { text: 'Docker', link: 'docker' },
                        {
                            text: 'From source',
                            base: '/self-hosting/install-source-',
                            items: [
                                { text: 'Ubuntu 22.04', link: 'ubuntu' },
                                { text: 'Ubuntu 24.04', link: 'ubuntu24_04' },
                                { text: 'Arch Linux', link: 'archlinux' },
                                { text: 'Build and configure', link: 'build' },
                                { text: 'Update', link: 'update' },
                            ]
                        },
                    ]

                }
            ]
        }
    ]
}
