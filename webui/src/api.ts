import config from './config'
import { Webrtc } from './webrtc'
import router from './router'

let sessid = window.localStorage.getItem('sessid')!

async function api_fetch(met: string, url: string, data: any) {
    // Default options are marked with *
    const response = await fetch(config.host() + '/api/v1' + url, {
        method: met,
        cache: 'no-cache',
        credentials: 'same-origin',
        headers: {
            'Content-Type': 'application/json',
            'Session-ID': sessid,
        },
        redirect: 'follow',
        referrerPolicy: 'no-referrer',
        body: JSON.stringify(data),
    }).catch((error) => {
        Webrtc.error('API Network error: ' + error.toString())
    })

    return response
}

export default {
    async isAuthenticated() {
        if (window.localStorage.getItem('sessid')) return true
        return false
    },

    async login(name: string, image: string) {
        let resp = await api_fetch('POST', '/client/connect', null)

        let session_id = resp?.headers.get('Session-ID')
        if (!session_id) return

        sessid = session_id

        resp = await api_fetch('POST', '/client/name', name)
        if (!resp?.ok) return

        resp = await api_fetch('POST', '/client/avatar', image)
        if (!resp?.ok) return

        window.localStorage.setItem('sessid', sessid)

        router.push({ name: 'Home' })
    },

    async logout() {
        await api_fetch('DELETE', '/client/logout', null)
        window.localStorage.removeItem('sessid')
    },

    async connect() {
        return await api_fetch('POST', '/webrtc/connect', null)
    },

    async sdp(desc: RTCSessionDescription | null) {
        return await api_fetch('PUT', '/webrtc/sdp', desc)
    },
}
