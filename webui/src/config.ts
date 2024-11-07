export default {
    base(): string {
        const pathSegments = location.pathname.split('/');
        const firstFolder = pathSegments[1]
        if (firstFolder === 'rooms')
            return '/rooms/' + pathSegments[2] + '/'

        return '/'
    },
    host(): string {
        if (process.env.NODE_ENV == 'production') {
            return location.origin
        }
        /* Development */
        return 'http://' + location.hostname + ':9999'
    },
    ws_host(): string {
        if (process.env.NODE_ENV == 'production') {
            if (location.protocol == 'https:')
                return 'wss://' + location.host
            else
                return 'ws://' + location.host
        }
        /* Development */
        return 'ws://' + location.hostname + ':9999'
    },
}
