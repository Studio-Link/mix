import config from './config'
import { Webrtc } from './webrtc'
import router from './router'
import { Users } from './ws/users'

interface Session {
    id: string
    auth: boolean
}

let sess: Session = JSON.parse(window.localStorage.getItem('sess')!)

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
        body: JSON.stringify(data),
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
    }

    return resp
}

export default {
    async isAuthenticated() {
        sess = JSON.parse(window.localStorage.getItem('sess')!)
        if (sess?.auth) return true
        return false
    },

    async connect(token?: string) {
        const resp = await api_fetch('POST', '/client/connect', token)

        const session_id = resp?.headers.get('Session-ID')
        if (!session_id) {
            window.localStorage.removeItem('sess')
            // router.push({ name: 'LoginError' })
            return
        }

        sess = { id: session_id, auth: false }
        console.log(sess)

        window.localStorage.setItem('sess', JSON.stringify(sess))
    },

    async login(name: string, image: string) {
        Webrtc.error('')
        let resp = await api_fetch('POST', '/client/name', name)
        if (!resp?.ok) return

        resp = await api_fetch('POST', '/client/avatar', image)
        if (!resp?.ok) return

        sess.auth = true
        window.localStorage.setItem('sess', JSON.stringify(sess))

        router.push({ name: 'Home' })
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

    async logout() {
        await api_fetch('DELETE', '/client/logout', null)
        window.localStorage.removeItem('sess')
        router.push({ name: 'Login' })
    },

    async sdp(desc: RTCSessionDescription | null) {
        return await api_fetch('PUT', '/webrtc/sdp', desc)
    },
}
