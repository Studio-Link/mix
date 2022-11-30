import { Ref, ref } from 'vue'
import router from '../router'

interface User {
    id: string
    name: string
    hand?: boolean
}

interface Users {
    socket?: WebSocket
    websocket(host: string, sessid: string): void
    speakers: Ref<User[] | undefined>
    listeners: Ref<User[] | undefined>
}

export const Users: Users = {
    speakers: ref([]),
    listeners: ref([]),
    websocket(host, sessid) {
        this.socket = new WebSocket(host + '/ws/v1/users')

        this.socket.onerror = () => {
            console.log('Websocket users error')
        }

        this.socket.onclose = (e) => {
            console.log('Websocket users closed', e.reason)
            if (e.code === 1011) {
                window.localStorage.removeItem('sessid')
                router.push({ name: 'Login' })
            }
        }

        this.socket.onopen = () => {
            this.socket?.send(sessid)
        }

        this.socket.onmessage = (message: any) => {
            const data = JSON.parse(message.data)

            if (data.type == 'users') {
                this.speakers.value = []
                this.listeners.value = []
                for (const key in data.users) {
                    if (data.users[key].speaker) {
                        this.speakers.value?.push(data.users[key])
                    }
                    else {
                        this.listeners.value?.push(data.users[key])
                    }
                }
                return
            }

            if (data.type === 'user') {
                // keep delete before add order!
                if (data.event === 'deleted' || data.event === 'updated') {
                    this.speakers.value = this.speakers.value?.filter((u) => u.id !== data.id)
                    this.listeners.value = this.listeners.value?.filter((u) => u.id !== data.id)
                }

                if (data.event === 'added' || data.event === 'updated') {
                    const user: User = {
                        id: data.id,
                        name: data.name,
                    }

                    if (data.speaker) {
                        this.speakers.value?.push(user)
                    }
                    else {
                        this.listeners.value?.unshift(user)
                    }
                }
            }
        }
    },
}
