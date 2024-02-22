import config from './config'
import { Webrtc } from './webrtc'
import router from './router'
import { Users, RecordType } from './ws/users'
import { Error } from './error'

interface Session {
    id: string
    auth: boolean
    user_id: string | null
    user_name: string
}

let sess: Session = JSON.parse(window.localStorage.getItem('sess')!)

async function api_fetch(met: string, url: string, data: any) {
    // Default options are marked with *
    const resp = await fetch(config.host() + config.base() + 'api/v1' + url, {
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
        Error.error('API Network error: ' + error.toString())
    })

    if (resp?.status! >= 400) {
        Error.error('API error: ' + resp?.status + ' ' + resp?.headers.get('Status-Reason'))
    }

    const session_id = resp?.headers.get('Session-ID')
    if (!session_id && resp?.status! >= 400) {
        window.localStorage.removeItem('sessid')
        router.push({ name: 'Login' })
        // if (url !== '/client/connect')
        //         location.reload()
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

    async reauth(token: string | string[]) {
        if (!token)
            return

        await api_fetch('POST', '/client/reauth', token)
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
        sess = { id: session_id, auth: false, user_id: null, user_name: '' }

        window.localStorage.setItem('sess', JSON.stringify(sess))
    },

    async login(name: string, image: string) {
        Error.reset()
        let resp = await api_fetch('POST', '/client/name', name)
        if (!resp?.ok) return

        resp = await api_fetch('POST', '/client/avatar', image)
        if (!resp?.ok) return

        sess.auth = true
        const user = JSON.parse(await resp?.text())

        sess.user_id = user.id
        sess.user_name = name
        window.localStorage.setItem('sess', JSON.stringify(sess))

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

    async listener(user_id: string | null) {
        if (user_id)
            await api_fetch('POST', '/client/listener', user_id)
    },

    async websocket() {
        Users.websocket(sess.id)
    },

    async hangup() {
       Webrtc.hangup()
       await api_fetch('POST', '/client/hangup', null) 
    },

    async logout(force: boolean) {
        Webrtc.hangup()
        await api_fetch('DELETE', '/client', sess)
        window.localStorage.removeItem('sess')
        if (force)
            window.localStorage.removeItem('token')
        router.push({ name: 'Login' })
        Users.ws_close()
    },

    async sdp_offer(desc: RTCSessionDescription | null) {
        return await api_fetch('PUT', '/webrtc/sdp/offer', desc)
    },

    async sdp_answer(desc: RTCSessionDescriptionInit | null, id: number) {
        return await api_fetch('PUT', '/webrtc/sdp/answer?id=' + id, desc)
    },

    async sdp_candidate(cand: RTCIceCandidate | null, id: number)
    {
        return await api_fetch('PUT', '/webrtc/sdp/candidate?id=' + id, cand)
    },

    async video(enable: boolean) {
        if (enable) await api_fetch('PUT', '/webrtc/video/enable', null)
        else await api_fetch('PUT', '/webrtc/video/disable', null)
    },

    async audio(enable: boolean) {
        if (enable) await api_fetch('PUT', '/webrtc/audio/enable', null)
        else await api_fetch('PUT', '/webrtc/audio/disable', null)
    },

    async source_focus(dev: string) {
        await api_fetch('PUT', '/webrtc/focus', dev)
    },

    async source_solo(dev: string) {
        await api_fetch('PUT', '/webrtc/solo', dev)
    },

    async video_solo(dev: string, enable: boolean) {
        if (enable)
            await api_fetch('PUT', '/webrtc/solo/enable', dev)
        else
            await api_fetch('PUT', '/webrtc/solo/disable', dev)
    },

    async record_switch(type: RecordType) {
        if (!Users.host_status.value) return

        if (Users.record.value) {
            Users.record.value = false
            Users.record_timer.value = '0:00:00'
            await api_fetch('PUT', '/record/disable', null)
        } else {
            Users.record.value = true
            if (type === RecordType.AudioOnly) {
                Users.record_type.value = RecordType.AudioOnly
                await api_fetch('PUT', '/record/audio/enable', null)
            }
            else {
                Users.record_type.value = RecordType.AudioVideo
                await api_fetch('PUT', '/record/enable', null)
            }
        }
    },

    hand(enable: boolean) {
        if (enable) api_fetch('PUT', '/hand/enable', null)
        else api_fetch('PUT', '/hand/disable', null)
    },
}
