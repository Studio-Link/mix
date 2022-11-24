import { reactive } from 'vue'

interface Speaker {
    id: number
    name: string
}

interface Listener {
    id: number
    name: string
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
        this.socket.onerror = () => {
            console.log('Websocket users error')
        }
        this.socket.onopen = () => { this.socket?.send(sessid) }
        this.socket.onmessage = (message: any) => {

            const users = JSON.parse(message.data)
            for (const key in users) {
                if (users[key].speaker)
                    this.speakers.push(users[key])
                else
                    this.listeners.push(users[key])
            }
        }
    }
}

