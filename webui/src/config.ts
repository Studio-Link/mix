export default {
  host(): string {
    if (process.env.NODE_ENV == 'production') {
      return location.origin
    }
    /* Development */
    return 'http://' + location.hostname + ':9999'
  },
  ws_host(): string {
    if (process.env.NODE_ENV == 'production') {
      return 'wss://' + location.hostname
    }
    /* Development */
    return 'ws://' + location.hostname + ':9999'
  },
}
