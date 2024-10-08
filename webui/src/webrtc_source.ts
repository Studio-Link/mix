import api from './api'

const pc_configuration: RTCConfiguration = {
    bundlePolicy: 'balanced',
    iceCandidatePoolSize: 0,
    iceServers: [],
    iceTransportPolicy: 'all',
};

export class WebRTCSource {
    private pc: RTCPeerConnection
    public audio: MediaStream | null
    public video: MediaStream | null
    public id: number

    constructor(id: number) {
        this.pc = new RTCPeerConnection(pc_configuration)

        this.audio = null
        this.video = null
        this.id = id

        this.pc.onicecandidate = (event) => {
            if (event.candidate)
                api.sdp_candidate(event.candidate, this.id)
        }

        this.pc.ontrack = (event) => {
            const track = event.track

            if (track.kind == 'audio') {
                this.audio = event.streams[0]
                console.log("WebRTCSource: audio track added")
            }

            if (track.kind == 'video') {
                this.video = event.streams[0]
                const video: HTMLVideoElement | null = document.querySelector('video#source' + this.id)

                if (!video) {
                    return
                }

                video.srcObject = this.video
                video.play()

                console.log("WebRTCSource: video track added")
            }
        }
    }

    async setRemoteDescription(descr: any) {
        try {
            await this.pc.setRemoteDescription(descr)

            const answer = await this.pc.createAnswer()
            await this.pc.setLocalDescription(answer)

            await api.sdp_answer(answer, this.id)

        } catch (error) {
            console.error('Error setting remote description:', error)
        }
    }

    close() {
        this.pc.close()
    }
}
