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

    constructor() {
        this.pc = new RTCPeerConnection(pc_configuration)

        this.audio = null
        this.video = null

        this.pc.onicecandidate = (event) => {
            console.log('WebRTCSource/icecandidate: ' + event.candidate?.type + ' IP: ' + event.candidate?.candidate)
        }

        this.pc.ontrack = (event) => {
            const track = event.track

            if (track.kind == 'audio') {
                this.audio = event.streams[0]
            }

            if (track.kind == 'video') {
                this.video = event.streams[0]
            }
        }
    }

    async setRemoteDescription(sdp_json: string) {
        const descr = JSON.parse(sdp_json)

        try {
            await this.pc.setRemoteDescription(descr)

            const answer = await this.pc.createAnswer()
            await this.pc.setLocalDescription(answer)

            await api.sdp_answer(answer)

        } catch (error) {
            console.error('Error setting remote description:', error)
        }
    }
}
