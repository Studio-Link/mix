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
      message: 'Released under the MIT License.',
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
      link: '/self-hosting/',
      activeMatch: '/self-hosting/'
    },
    {
      text: '0.6.0-beta',
      items: [
        {
          text: 'Changelog',
          link: 'https://github.com/studio-link/mix/blob/main/CHANGELOG.md'
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
  ]
}

function sidebarSelfHosted(): DefaultTheme.SidebarItem[] {
  return [
    {
      text: 'Reference',
      items: [
        { text: 'Site Config', link: 'site-config' },
        { text: 'Frontmatter Config', link: 'frontmatter-config' },
        { text: 'Runtime API', link: 'runtime-api' },
        { text: 'CLI', link: 'cli' },
        {
          text: 'Default Theme',
          base: '/reference/default-theme-',
          items: [
            { text: 'Overview', link: 'config' },
            { text: 'Nav', link: 'nav' },
            { text: 'Sidebar', link: 'sidebar' },
            { text: 'Home Page', link: 'home-page' },
            { text: 'Footer', link: 'footer' },
            { text: 'Layout', link: 'layout' },
            { text: 'Badge', link: 'badge' },
            { text: 'Team Page', link: 'team-page' },
            { text: 'Prev / Next Links', link: 'prev-next-links' },
            { text: 'Edit Link', link: 'edit-link' },
            { text: 'Last Updated Timestamp', link: 'last-updated' },
            { text: 'Search', link: 'search' },
            { text: 'Carbon Ads', link: 'carbon-ads' }
          ]
        }
      ]
    }
  ]
}
