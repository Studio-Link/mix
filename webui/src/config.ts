export default {
    base(): string {
        const pathSegments = location.pathname.split('/');
        const firstFolder = pathSegments[1]
        if (firstFolder === 'rooms')
            return '/rooms/' + pathSegments[2] + '/'

        return '/'
    },
    host(): string {
        return location.origin
    },
    ws_host(): string {
        if (location.protocol == 'https:')
            return 'wss://' + location.host
        else
            return 'ws://' + location.host
    },
}
