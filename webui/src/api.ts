import config from './config'
import { Webrtc } from './webrtc'

let sessid: string

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
        Webrtc.error("API Network error: " + error.toString())
    })

    if (response && response.status >= 400 && response.status < 600) {
        Webrtc.error("${response.statusText} (${met} ${url})")
    }

    return response
}

export default {
    async connect() {
        let req = await api_fetch('POST', '/client/connect', null)
        let session_id = req?.headers.get('Session-ID')
        if (session_id)
            sessid = session_id
    },

    async sdp(desc: RTCSessionDescription | null) {
        return await api_fetch('PUT', '/client/sdp', desc)
    },
}
