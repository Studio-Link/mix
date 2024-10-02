import { defineConfig, type DefaultTheme } from 'vitepress'

export const de = defineConfig({
    lang: 'de-DE',
    description: 'Mix Rooms',

    themeConfig: {
        nav: nav(),

        sidebar: {
            '/de/hosted/': { base: '/de/hosted/', items: sidebarHosted() },
            '/de/self-hosting/': { base: '/de/self-hosting/', items: sidebarSelfHosted() }
        },

        footer: {
            message: 'Veröffentlicht unter der MIT Lizenz. <br> <a href="https://studio-link.de/impressum.html">Impressum</a> | <a href="https://studio-link.de/datenschutz.html">Datenschutz</a>',
            copyright: 'Copyright © 2013-heute IT-Service Sebastian Reimers'
        }
    }
})

function nav(): DefaultTheme.NavItem[] {
    return [
        {
            text: 'Jetzt loslegen',
            link: '/de/hosted/started',
            activeMatch: '/de/hosted/'
        },
        {
            text: 'Open Source',
            link: '/de/self-hosting/install-intro',
            activeMatch: '/de/self-hosting/'
        },
        {
            text: '0.6.0-beta',
            items: [
                {
                    text: 'Changelog',
                    link: '/de/changelog'
                }
            ]
        }
    ]
}

function sidebarHosted(): DefaultTheme.SidebarItem[] {
    return [
        {
            text: 'Einführung',
            collapsed: false,
            items: [
                { text: 'Was ist Mix Rooms?', link: 'what-is-mix-rooms' },
                { text: 'Jetzt loslegen', link: 'started' },
            ]
        },
        {
            text: 'Bedienung - Howto',
            collapsed: false,
            base: '/de/hosted/howto/',
            items: [
                { text: 'Überblick', link: 'overview' },
                { text: 'Login', link: 'login' },
                { text: 'Aufnahmen', link: 'recording' },
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
                    base: '/de/self-hosting/install-',
                    collapsed: false,
                    items: [
                        { text: 'Einführung', link: 'intro' },
                        { text: 'Docker', link: 'docker' },
                        {
                            text: 'Quellcode',
                            base: '/de/self-hosting/install-source-',
                            items: [
                                { text: 'Ubuntu 22.04', link: 'ubuntu' },
                                { text: 'Ubuntu 24.04', link: 'ubuntu24_04' },
                                { text: 'Arch Linux', link: 'archlinux' },
                                { text: 'Bauen und konfigurieren', link: 'build' },
                                { text: 'Update', link: 'update' },
                            ]
                        },
                    ]

                }
            ]
        }
    ]
}
