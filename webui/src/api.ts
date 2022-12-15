import config from './config'
import { Webrtc } from './webrtc'
import router from './router'
import { Users } from './ws/users'

interface Session {
    id: string
    auth: boolean
    host: boolean
    user_id: string | null
}

let sess: Session = JSON.parse(window.localStorage.getItem('sess')!)

function reloadPage() {
    const reload = window.localStorage.getItem('reload')
    if (reload)
        return

    window.localStorage.setItem('reload', 'true')
    location.reload();
}

async function api_fetch(met: string, url: string, data: any) {
    // Default options are marked with *
    const resp = await fetch(config.host() + '/api/v1' + url, {
        method: met,
        cache: 'no-cache',
        credentials: 'same-origin',
        headers: {
            'Content-Type': 'application/json',
            'Session-ID': sess?.id,
        },
        redirect: 'follow',
        referrerPolicy: 'no-referrer',
        body: data ? JSON.stringify(data) : null,
    }).catch((error) => {
        Webrtc.error('API Network error: ' + error.toString())
    })

    if (resp?.status! >= 400) {
        Webrtc.error('API error: ' + resp?.status + ' ' + resp?.headers.get('Status-Reason'))
    }

    const session_id = resp?.headers.get('Session-ID')
    if (!session_id && resp?.status! >= 400) {
        window.localStorage.removeItem('sessid')
        router.push({ name: 'Login' })
        if (url !== '/client/connect')
            reloadPage()
    }

    return resp
}

export default {
    session() {
        return sess
    },
    async isAuthenticated() {
        sess = JSON.parse(window.localStorage.getItem('sess')!)
        if (sess?.auth) return true
        return false
    },

    async connect(token?: string | null) {
        if (!token)
            token = window.localStorage.getItem('token')
        else
            window.localStorage.setItem('token', token)

        const resp = await api_fetch('POST', '/client/connect', token)

        const session_id = resp?.headers.get('Session-ID')
        if (!session_id) {
            window.localStorage.removeItem('sess')
            window.localStorage.removeItem('token')
            return
        }

        /* Readonly! Use ws/users for updated states */
        sess = { id: session_id, auth: false, host: false, user_id: null }

        window.localStorage.setItem('sess', JSON.stringify(sess))
    },

    async login(name: string, image: string) {
        Webrtc.error('')
        let resp = await api_fetch('POST', '/client/name', name)
        if (!resp?.ok) return

        resp = await api_fetch('POST', '/client/avatar', image)
        if (!resp?.ok) return

        sess.auth = true
        const user = JSON.parse(await resp?.text())

        sess.user_id = user.id
        sess.host = user.host
        window.localStorage.setItem('sess', JSON.stringify(sess))
        window.localStorage.removeItem('reload')

        router.push({ name: 'Home' })
    },

    async chat(msg: string) {
        await api_fetch('POST', '/chat', msg)
    },

    async get_chat() {
        return await api_fetch('GET', '/chat', null)
    },

    async speaker(user_id: string) {
        await api_fetch('POST', '/client/speaker', user_id)
    },

    async listener(user_id: string) {
        await api_fetch('POST', '/client/listener', user_id)
    },

    async websocket() {
        Users.websocket(config.ws_host(), sess.id)
    },

    async logout(force: boolean) {
        Webrtc.logout()
        await api_fetch('DELETE', '/client', sess)
        window.localStorage.removeItem('sess')
        if (force)
            window.localStorage.removeItem('token')
        router.push({ name: 'Login' })
        Users.ws_close()
    },

    async sdp(desc: RTCSessionDescription | null) {
        return await api_fetch('PUT', '/webrtc/sdp', desc)
    },

    async video(enable: boolean) {
        if (enable) await api_fetch('PUT', '/webrtc/video/enable', null)
        else await api_fetch('PUT', '/webrtc/video/disable', null)
    },

    async audio(enable: boolean) {
        if (enable) await api_fetch('PUT', '/webrtc/audio/enable', null)
        else await api_fetch('PUT', '/webrtc/audio/disable', null)
    },

    is_host(): boolean {
        return sess.host
    },

    async record_switch() {
        if (!sess.host) return

        if (Users.record.value) {
            Users.record.value = false
            Users.record_timer.value = '0:00:00'
            await api_fetch('PUT', '/record/disable', null)
        } else {
            Users.record.value = true
            await api_fetch('PUT', '/record/enable', null)
        }
    },

    hand(enable: boolean) {
        if (enable) api_fetch('PUT', '/hand/enable', null)
        else api_fetch('PUT', '/hand/disable', null)
    },
}
