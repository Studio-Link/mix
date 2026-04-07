import api from './api'

const pc_configuration: RTCConfiguration = {
    bundlePolicy: 'balanced',
    iceCandidatePoolSize: 0,
    iceServers: [],
    iceTransportPolicy: 'all',
};

const QualityLimitationReasons = {
    none: 0,
    bandwidth: 1,
    cpu: 2,
    other: 3,
};

async function pc_stats(pc: RTCPeerConnection | null, id: number) {

    if (!pc || pc.connectionState === 'closed')
        return

    const state = pc.connectionState

    const stats = await pc.getStats()

    // @ts-ignore
    const values = [...stats.values()].filter(
        (v) =>
            ["peer-connection", "inbound-rtp", "outbound-rtp", "remote-inbound-rtp"].indexOf(v.type) !== -1,
    );

    let data = "";
    const sentTypes = new Set();


    values.forEach((value: any) => {
        const type = value.type.replace(/-/g, "_");
        const labels: any = [];
        const metrics: any = [];

        if (value.type === "peer-connection") {
            labels.push(`state="${state}"`);
        }

        labels.push(`user="${api.user_id()}"`);
        labels.push(`source="${id}"`);

        Object.entries(value).forEach(([key, v]: any) => {
            if (typeof v === "number") {
                metrics.push([key, v]);
            } else if (typeof v === "object") {
                Object.entries(v).forEach(([subkey, subv]) => {
                    if (typeof subv === "number") {
                        metrics.push([`${key}_${subkey}`, subv]);
                    }
                });
            } else if (
                key === "qualityLimitationReason" &&
                QualityLimitationReasons[v as keyof typeof QualityLimitationReasons] !== undefined
            ) {
                metrics.push([key, QualityLimitationReasons[v as keyof typeof QualityLimitationReasons]]);
            } else {
                labels.push(`${key}="${v}"`);
            }
        });

        metrics.forEach(([key, v]: any) => {
            const name = `${type}_${key.replace(/-/g, "_")}`;
            let typeDesc = "";

            if (!sentTypes.has(name)) {
                typeDesc = `# TYPE ${name} gauge\n`;
                sentTypes.add(name);
            }
            data += `${typeDesc}${name}{${labels.join(",")}} ${v}\n`;
        });
    });

    await api.rtc_stats(data)
    setTimeout(pc_stats, 5000, pc, id)
}

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

                pc_stats(this.pc, this.id)

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
