import { reactive } from 'vue'
import config from '../config'

export enum LocalTrackStates {
    Setup = 0,
    SelectAudio,
    Ready,
}

export enum TrackStatus {
    IDLE = 0,
    LOCAL_REGISTERING = 1,
    LOCAL_REGISTER_OK = 2,
    LOCAL_REGISTER_FAIL = 3,
    LOCAL_AUDIO_READY = 4,
    REMOTE_CONNECTED = 5,
    REMOTE_CALLING = 6,
    REMOTE_INCOMING = 7
}

interface Track {
    id: number
    name: string
    status: TrackStatus
    muted: boolean
    error: string
}

interface State extends Track {
    selected: boolean
    local: LocalTrackStates
}

interface AudioDevice {
    idx: number
    name: string
}

interface AudioList {
    src: AudioDevice[]
    play: AudioDevice[]
    src_dev: number
    play_dev: number
}

interface LocalTrack extends Track {
    audio: AudioList
}

interface Tracks {
    socket?: WebSocket
    state: State[]
    local_tracks: LocalTrack[]
    remote_tracks: Track[]
    selected_debounce: boolean
    clear_tracks(): void
    /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
    update(tracks: any): void
    getTrackName(id: number): string
    websocket(): void
    isValid(id: number): boolean
    isSelected(id: number): boolean
    isMuted(id: number): boolean
    localState(id: number): LocalTrackStates
    select(id: number): void
    selected(): number,
}

export const Tracks: Tracks = {
    state: reactive([]),
    remote_tracks: reactive([]),
    local_tracks: reactive([]),
    selected_debounce: false,

    getTrackName(id: number): string {
        if (this.state[id] != undefined) {
            return this.state[id].name.replace('sip:', '')
        }
        return 'error'
    },

    websocket(): void {
        this.socket = new WebSocket(config.ws_host() + config.base() + 'ws/v1/tracks')
        this.socket.onerror = function () {
            console.log('Websocket error')
        }
        this.socket.onmessage = (message) => {
            const tracks = JSON.parse(message.data)
            this.update(tracks)
        }
        this.selected_debounce = false
    },

    clear_tracks(): void {
        this.local_tracks.length = 0
        this.remote_tracks.length = 0
    },

    update(tracks): void {
        let last_key = 0
        this.clear_tracks()

        for (const key in tracks) {
            tracks[key].id = parseInt(key)

            /* Initialize frontend state only once */
            if (this.state[tracks[key].id] === undefined) {
                this.state[tracks[key].id] = {
                    id: tracks[key].id,
                    selected: false,
                    status: TrackStatus.IDLE,
                    error: "",
                    local: LocalTrackStates.Setup,
                    name: tracks[key].name,
                    muted: tracks[key].muted
                }
                last_key = parseInt(key)
            }

            this.state[tracks[key].id].name = tracks[key].name
            this.state[tracks[key].id].status = tracks[key].status
            this.state[tracks[key].id].muted = tracks[key].muted

            if (tracks[key].type == 'local') {
                this.local_tracks.push(tracks[key])
            }

            if (tracks[key].type == 'remote') {
                this.remote_tracks.push(tracks[key])
            }
        }

        /* Cleanup state for deleted tracks */
        for (const key in this.state) {
            if (tracks[key] === undefined) {
                delete this.state[key]
            }
        }

        /* select last added track */
        if (last_key > 0) {
            this.select(last_key)
        }
    },

    isValid(id: number): boolean {
        if (this.state[id]) {
            return true
        }
        return false
    },

    isSelected(id: number): boolean {
        if (this.state[id]) {
            return this.state[id].selected
        }
        return false
    },

    isMuted(id: number): boolean {
        return this.state[id]?.muted
    },

    localState(id: number): LocalTrackStates {
        return this.state[id].local
    },

    select(id: number): void {
        if (this.state[id] === undefined)
            return

        //Workaround for mouseenter event after focus change
        if (this.selected_debounce) return
        this.selected_debounce = true
        setTimeout(() => {
            this.selected_debounce = false
        }, 20)

        this.state.forEach((el) => {
            el.selected = false
        })

        this.state[id].selected = true
    },

    selected(): number {
        let id = -1
        this.state.forEach((el) => {
            if (el.selected) {
                id = el.id
            }
        })

        return id
    },
}
