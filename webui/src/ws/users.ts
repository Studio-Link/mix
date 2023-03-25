import { Ref, ref } from 'vue'
import api from '../api'
import { Error } from '../error'
import { Webrtc } from '../webrtc'
import config from '../config'

interface Room {
    name: string
    url: string
    image: string
}

interface User {
    id: string
    name: string
    hand: boolean
    host: boolean
    video: boolean
    audio: boolean
    webrtc: boolean
}

interface Chat {
    user_id: string
    user_name: string
    time: string
    msg: string
}

export enum RecordType {
    AudioVideo,
    AudioOnly
}

interface Users {
    socket?: WebSocket
    ws_close(): void
    websocket(sessid: string): void
    speakers: Ref<User[] | undefined>
    listeners: Ref<User[] | undefined>
    chat_messages: Ref<Chat[] | undefined>
    chat_active: Ref<boolean>
    settings_active: Ref<boolean>
    record_timer: Ref<string>
    record: Ref<boolean>
    record_type: Ref<RecordType>
    hand_status: Ref<boolean>
    speaker_status: Ref<boolean>
    rooms: Ref<Room[] | undefined>
}

function pad(num: number, size: number) {
    const s = '0000' + num
    return s.substring(s.length - size)
}

export const Users: Users = {
    rooms: ref([]),
    speakers: ref([]),
    listeners: ref([]),
    chat_messages: ref([]),
    chat_active: ref(false),
    settings_active: ref(false),
    record_timer: ref('0:00:00'),
    record: ref(false),
    record_type: ref(RecordType.AudioVideo),
    hand_status: ref(false),
    speaker_status: ref(false),

    ws_close() {
        this.socket?.close()
    },
    websocket(sessid) {
        this.socket = new WebSocket(config.ws_host() + config.base() + 'ws/v1/users')

        this.socket.onerror = () => {
            console.log('Websocket users error')
        }

        this.socket.onclose = (e) => {
            console.log('Websocket users closed', e.reason)
            if (e.code === 1007) {
                api.logout(false)
            }
            if (e.code === 1011) {
                Error.fatal('Already connected! Please close remaining windows.')
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
                    if (data.users[key].id === api.session().user_id) {
                        this.hand_status.value = data.users[key].hand
                        this.speaker_status.value = data.users[key].speaker
                    }
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
                        audio: data.audio,
                        hand: data.hand,
                        webrtc: data.webrtc
                    }

                    if (user.id === api.session().user_id) {
                        this.hand_status.value = user.hand
                        this.speaker_status.value = data.speaker

                        /* Only allow remote disable */
                        if (!data.speaker) {
                            if (!Webrtc.audio_muted.value)
                                Webrtc.audio_mute(true)

                            if (!Webrtc.video_muted.value)
                                Webrtc.video_mute(true)
                        }
                    }

                    if (data.speaker) {
                        this.speakers.value?.unshift(user)
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
                    msg: data.msg,
                }
                this.chat_messages.value?.push(chat)
            }

            if (data.type === 'rooms') {
                this.rooms.value = []
                for (const key in data.rooms) {
                    const room: Room = { name: key, image: '', url: data.rooms[key].url }
                    this.rooms.value.push(room)
                }
            }

            if (data.type === 'rec') {
                let time = parseInt(data.t)
                if (time) this.record.value = true
                else this.record.value = false

                const h = Math.floor(time / (60 * 60))
                time = time % (60 * 60)
                const m = Math.floor(time / 60)
                time = time % 60
                const s = Math.floor(time)

                this.record_timer.value = pad(h, 1) + ':' + pad(m, 2) + ':' + pad(s, 2)
            }
        }
    },
}
