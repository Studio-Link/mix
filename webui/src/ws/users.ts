import { Ref, ref } from 'vue'
import api from '../api'

interface User {
    id: string
    name: string
    hand?: boolean
    host: boolean
    video: boolean
    audio: boolean
}

interface Chat {
    user_id: string
    user_name: string
    time: string
    msg: string
}

interface Users {
    socket?: WebSocket
    websocket(host: string, sessid: string): void
    speakers: Ref<User[] | undefined>
    listeners: Ref<User[] | undefined>
    chat_messages: Ref<Chat[] | undefined>
    chat_active: Ref<boolean>
    settings_active: Ref<boolean>
}

export const Users: Users = {
    speakers: ref([]),
    listeners: ref([]),
    chat_messages: ref([]),
    chat_active: ref(false),
    settings_active: ref(false),
    websocket(host, sessid) {
        this.socket = new WebSocket(host + '/ws/v1/users')

        this.socket.onerror = () => {
            console.log('Websocket users error')
        }

        this.socket.onclose = (e) => {
            console.log('Websocket users closed', e.reason)
            if (e.code === 1011) {
                api.logout()
            }
        }

        this.socket.onopen = () => {
            this.socket?.send(sessid)
        }

        this.socket.onmessage = (message: any) => {
            const data = JSON.parse(message.data)

            if (data.type === 'users') {
                this.speakers.value = []
                this.listeners.value = []
                for (const key in data.users) {
                    if (data.users[key].speaker) {
                        this.speakers.value?.push(data.users[key])
                    } else {
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
                        host: data.host,
                        video: data.video,
                        audio: data.audio
                    }

                    if (data.speaker) {
                        this.speakers.value?.push(user)
                    } else {
                        this.listeners.value?.unshift(user)
                    }
                }
            }

            if (data.type === 'chat') {
                const chat = {
                    user_id: data.user_id,
                    user_name: data.user_name,
                    time: data.time,
                    msg: data.msg
                }
                this.chat_messages.value?.push(chat)
            }
        }
    },
}
