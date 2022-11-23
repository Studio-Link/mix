import { reactive } from 'vue'

interface Speaker {
    id: number
}

interface Listener {
    id: number
}

interface Users {
    socket?: WebSocket
    websocket(host: string, sessid: string): void
    speakers: Speaker[]
    listeners: Listener[]
}

export const Users: Users = {
    speakers: reactive([]),
    listeners: reactive([]),
    websocket(host, sessid) {
        this.socket = new WebSocket(host + '/ws/v1/users')
        this.socket.onerror = function() {
            console.log('Websocket users error')
        }
        this.socket.onopen = () => { this.socket?.send(sessid) }
        this.socket.onmessage = (message: any) => {
            console.log("users:", message)
        }
    }
}

