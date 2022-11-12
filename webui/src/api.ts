import config from './config'

function api_request(met: string, url: string, data: string | null) {
  const req = new XMLHttpRequest()
  req.open(met, 'http://' + config.ws_host() + '/api/v1' + url)
  req.onerror = (e) => {
    console.log(e)
  }
  req.send(data)
}

export default {
  sdp(desc: RTCSessionDescription | null) {
    api_request('POST', '/sdp', desc ? desc.sdp : null)
  },
}
