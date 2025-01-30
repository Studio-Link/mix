import config from './config'
import { Webrtc } from './webrtc'
import router from './router'
import { Users, RecordType } from './ws/users'
import { Error } from './error'


let user_id: string | undefined = undefined

async function api_fetch(met: string, url: string, data: any, json = true, gzip = false) {
    const body = json ? JSON.stringify(data) : data
    const headers = {
        'Content-Type': 'application/json',
    }

    // Default options are marked with *
    const resp = await fetch(config.host() + config.base() + 'api/v1' + url, {
        method: met,
        cache: 'no-cache',
        credentials: 'same-origin',
        headers: gzip ? { ...headers, 'Content-Encoding': 'gzip' } : headers,
        redirect: 'follow',
        referrerPolicy: 'no-referrer',
        body: data ? body : null,
    }).catch((error) => {
        Error.error('API Network error: ' + error.toString())
    })

    if (resp?.status! === 401) {
        router.push({ name: 'Login' })
    }

    if (resp?.status! > 401) {
        Error.error('API error: ' + resp?.status + ' ' + resp?.headers.get('Status-Reason'))
    }

    return resp
}

export default {

    user_id() {
        return user_id
    },

    async isAuthenticated() {
        if (user_id) return true
        return false
    },

    async connect(token?: string | string[] | null) {
        const resp = await api_fetch('POST', '/client/connect', token)
        if (!resp?.ok) return

        user_id = await resp?.text()
    },

    async login(name: string, image: string) {
        Error.reset()
        let resp = await api_fetch('POST', '/client/name', name)
        if (!resp?.ok) return

        resp = await api_fetch('POST', '/client/avatar', image)
        if (!resp?.ok) return

        await this.connect(null)

        router.push({ name: 'Home' })
    },

    async chat(msg: string) {
        await api_fetch('POST', '/chat', msg)
    },

    async get_chat() {
        return await api_fetch('GET', '/chat', null)
    },

    async speaker(user_id: string | undefined) {
        if (user_id)
            await api_fetch('POST', '/client/speaker', user_id)
    },

    async listener(user_id: string | undefined) {
        if (user_id)
            await api_fetch('POST', '/client/listener', user_id)
    },

    async websocket() {
        Users.websocket()
    },

    async hangup() {
        Webrtc.hangup()
        await api_fetch('POST', '/client/hangup', null)
    },

    async logout() {
        Webrtc.hangup()
        await api_fetch('DELETE', '/client', null)
        router.push({ name: 'Login' })
        Users.ws_close()
    },

    async sdp_offer(desc: RTCSessionDescription | null) {
        return await api_fetch('PUT', '/webrtc/sdp/offer', desc)
    },

    async sdp_answer(desc: RTCSessionDescriptionInit | null, id: number) {
        return await api_fetch('PUT', '/webrtc/sdp/answer?id=' + id, desc)
    },

    async sdp_candidate(cand: RTCIceCandidate, id: number) {
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

    async amix(user_id: string, enable: boolean) {
        if (enable) await api_fetch('PUT', '/webrtc/amix/enable', user_id)
        else await api_fetch('PUT', '/webrtc/amix/disable', user_id)
    },

    async source_focus(dev: string) {
        await api_fetch('PUT', '/webrtc/focus', dev)
    },

    async video_solo(dev: string, enable: boolean) {
        if (enable)
            await api_fetch('PUT', '/webrtc/solo/enable', dev)
        else
            await api_fetch('PUT', '/webrtc/solo/disable', dev)
    },

    async rtc_stats(data: string) {
        const stream = new Blob([data]).stream();
        const compressedReadableStream = stream.pipeThrough(
            new CompressionStream("gzip")
        );

        const compressedResponse =
            new Response(compressedReadableStream);

        const blob = await compressedResponse.blob();

        await api_fetch('POST', '/webrtc/stats', blob, false, true)
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

    async emoji(reaction_id: number) {
        api_fetch('PUT', '/emoji', reaction_id.toString(), false, false)
    }
}
