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

  if (response && response.status >= 400 && response.status < 600) {
    Webrtc.error('${response.statusText} (${met} ${url})')
  }

  return response
}

export default {
  isAuthenticated() {
    if (sessid) return true
    return false
  },
  async avatar(image: string) {
    console.log(image)
  },
  async login() {
    let req = await api_fetch('POST', '/client/login', null)
    let session_id = req?.headers.get('Session-ID')
    if (session_id) {
      sessid = session_id
      //@FIXME: add destroy (404 answer from server)
      window.localStorage.setItem('sessid', sessid)
    }
    router.push({ name: 'Home' })
  },
  async logout() {
    await api_fetch('DELETE', '/client/logout', null)
  },
  async connect() {
    return await api_fetch('POST', '/webrtc/connect', null)
  },
  async sdp(desc: RTCSessionDescription | null) {
    return await api_fetch('PUT', '/webrtc/sdp', desc)
  },
}
