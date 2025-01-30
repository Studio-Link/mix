import { Ref, ref } from 'vue'
import api from '../api'
import { Error } from '../error'
import { Webrtc } from '../webrtc'
import config from '../config'
import { WebRTCSource } from '../webrtc_source'

interface Room {
    name: string
    url: string
    image: string
    listeners: number
    show: boolean
}

interface Stats {
    artt: number
    vrtt: number
}

interface Source {
    rtc: WebRTCSource | null
    dev: string
    solo: boolean
}

interface User {
    id: string
    speaker_id: number
    pidx: number
    name: string
    hand: boolean
    host: boolean
    video: boolean
    audio: boolean
    webrtc: boolean
    talk: boolean
    solo: boolean
    stats: Stats
}

interface Chat {
    user_id: string
    user_name: string
    time: string
    msg: string
}

interface Emoji {
    id: number
    reaction_id: number
    x: number
    size: number
}

export enum RecordType {
    AudioVideo,
    AudioOnly
}

interface Users {
    socket?: WebSocket
    ws_close(): void
    websocket(): void
    room: Ref<Room | undefined>
    rooms: Ref<Room[]>
    sources: Ref<Source[]>
    speakers: Ref<User[]>
    vspeakers: Ref<User[]>
    listeners: Ref<User[]>
    chat_messages: Ref<Chat[]>
    chat_active: Ref<boolean>
    chat_unread: Ref<number>
    settings_active: Ref<boolean>
    record_timer: Ref<string>
    record: Ref<boolean>
    record_type: Ref<RecordType>
    hand_status: Ref<boolean>
    speaker_status: Ref<boolean>
    host_status: Ref<boolean>
    user_name: Ref<string>
    emojis: Ref<Emoji[]>
}

function pad(num: number, size: number) {
    const s = '0000' + num
    return s.substring(s.length - size)
}

export const Users: Users = {
    room: ref(undefined),
    rooms: ref([]),
    sources: ref([]),
    speakers: ref([]),
    vspeakers: ref([]),
    listeners: ref([]),
    chat_messages: ref([]),
    chat_active: ref(false),
    chat_unread: ref(0),
    settings_active: ref(false),
    record_timer: ref('0:00:00'),
    record: ref(false),
    record_type: ref(RecordType.AudioVideo),
    hand_status: ref(false),
    speaker_status: ref(false),
    host_status: ref(false),
    user_name: ref(''),
    emojis: ref([]),

    ws_close() {
        this.socket?.close()
    },
    websocket() {
        this.socket = new WebSocket(config.ws_host() + config.base() + 'ws/v1/users')

        this.socket.onerror = () => {
            console.log('Websocket users error')
        }

        this.socket.onclose = (e) => {
            console.log('Websocket users closed', e.reason)
            if (e.code === 1007) {
                api.logout()
            }
            if (e.code === 1011) {
                Error.fatal('Already connected! Please close remaining windows.')
            }
        }

        this.socket.onmessage = (message: any) => {
            const data = JSON.parse(message.data)

            if (data.type === 'users') {
                this.speakers.value = []
                this.listeners.value = []
                for (const key in data.users) {
                    if (data.users[key].id === api.user_id()) {
                        this.hand_status.value = data.users[key].hand
                        this.speaker_status.value = data.users[key].speaker
                        this.host_status.value = data.users[key].host
                        this.user_name = data.users[key].name
                    }

                    if (data.users[key].name.startsWith('sip:'))
                        continue

                    if (data.users[key].speaker) {
                        if (data.users[key].pidx) {
                            this.vspeakers.value.push(data.users[key])
                            this.vspeakers.value.sort((a, b) => a.pidx - b.pidx)
                            continue
                        }
                        this.speakers.value?.push(data.users[key])
                    } else {
                        this.listeners.value?.push(data.users[key])
                    }
                }
                return
            }

            if (data.type === 'user') {
                // keep delete before add/update order!
                this.speakers.value = this.speakers.value?.filter((u) => u.id !== data.id)
                this.listeners.value = this.listeners.value?.filter((u) => u.id !== data.id)
                this.vspeakers.value = this.vspeakers.value?.filter((u) => u.id !== data.id)

                if (data.event === 'deleted')
                    return

                const user: User = {
                    id: data.id,
                    speaker_id: data.speaker_id,
                    pidx: data.pidx,
                    name: data.name,
                    host: data.host,
                    video: data.video,
                    audio: data.audio,
                    hand: data.hand,
                    webrtc: data.webrtc,
                    solo: data.solo,
                    talk: false,
                    stats: { artt: 0, vrtt: 0 }
                }

                if (user.id === api.user_id()) {
                    this.hand_status.value = user.hand
                    this.speaker_status.value = data.speaker
                    this.host_status.value = data.host

                    /* Only allow remote disable */
                    if (!data.speaker) {
                        if (!Webrtc.audio_muted.value)
                            Webrtc.audio_mute(true)

                        if (!Webrtc.video_muted.value)
                            Webrtc.video_mute(true)
                    }

                    /* Trigger Video Mute state - Avatar Workaround */
                    if (data.speaker && !user.video)
                        Webrtc.video_mute(true, true)
                }

                if (data.speaker) {
                    if (user.pidx) {
                        this.vspeakers.value.push(user)
                        this.vspeakers.value.sort((a, b) => a.pidx - b.pidx)
                        return
                    }
                    this.speakers.value?.unshift(user)
                } else {
                    this.listeners.value?.unshift(user)
                }

                return
            }

            if (data.type === 'chat') {
                const chat = {
                    user_id: data.user_id,
                    user_name: data.user_name,
                    time: data.time,
                    msg: data.msg,
                }
                this.chat_messages.value?.push(chat)

                if (!this.chat_active.value) {
                    document.dispatchEvent(new Event("chat"))
                    this.chat_unread.value++
                }

                return
            }

            if (data.type === 'rooms') {
                this.rooms.value = []
                for (const key in data.rooms) {
                    const room: Room = {
                        name: key, image: '',
                        url: data.rooms[key].url,
                        listeners: data.rooms[key].listeners,
                        show: data.rooms[key].show,
                    }
                    this.rooms.value.push(room)

                    if (data.rooms[key].url == location.pathname)
                        this.room.value = room
                }

                return
            }

            if (data.type === 'rec') {
                let time = parseInt(data.t)
                let speaker_id = parseInt(data.s)
                if (time) {
                    this.record.value = true
                    const h = Math.floor(time / (60 * 60))
                    time = time % (60 * 60)
                    const m = Math.floor(time / 60)
                    time = time % 60
                    const s = Math.floor(time)

                    this.record_timer.value = pad(h, 1) + ':' + pad(m, 2) + ':' + pad(s, 2)
                }
                else {
                    this.record.value = false
                }

                if (speaker_id) {
                    for (const key in this.vspeakers.value) {
                        if (this.vspeakers.value[key].speaker_id == speaker_id)
                            this.vspeakers.value[key].talk = true
                        else
                            this.vspeakers.value[key].talk = false
                    }
                }
                return
            }

            if (data.type === 'stats') {
                let speaker_id = parseInt(data.id)
                for (const key in this.vspeakers.value) {
                    if (this.vspeakers.value[key].speaker_id != speaker_id)
                        continue;

                    this.vspeakers.value[key].stats = data.stats
                }
                return
            }

            if (data.type === 'offer') {

                const src: Source = {
                    rtc: new WebRTCSource(data.id),
                    dev: data.dev,
                    solo: false
                }

                src.rtc?.setRemoteDescription(data)

                this.sources.value.push(src);

                return
            }

            if (data.type === 'source_close') {

                for (var i = 0; i < this.sources.value.length; i++) {
                    if (data.id !== this.sources.value[i].rtc?.id) {
                        continue
                    }

                    this.sources.value[i].rtc?.close()
                    this.sources.value.splice(i, 1)
                    return
                }

                return
            }

            if (data.type === 'emoji') {
                const newEmoji: Emoji = {
                    id: Date.now(),
                    reaction_id: data.id,
                    x: Math.random() * 80, // Random horizontal position
                    size: Math.random() * 20 + 30,
                };
                this.emojis.value.push(newEmoji);

                setTimeout(() => {
                    this.emojis.value = this.emojis.value.filter((emoji) => emoji.id !== newEmoji.id);
                }, 3000);

                return
            }
        }
    },
}
