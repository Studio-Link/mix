import api from "./api"

let pc: RTCPeerConnection
const configuration: RTCConfiguration = {
    bundlePolicy: 'balanced',

    iceTransportPolicy: 'relay',
    iceServers: [
        {
            urls: 'turn:195.201.63.86:3478',
            username: 'turn200301',
            credential: 'choh4zeem3foh1',
        },
    ],
}

function pc_offer() {
    const offerOptions = {
        iceRestart: false,
    };
    pc.createOffer(offerOptions)
        .then(function(desc) {
            console.log("got local description: %s", desc.type);

            // console.log(desc.sdp)

            pc.setLocalDescription(desc).then(
                () => { },
                function(error) {
                    console.log("setLocalDescription: %s", error.toString());
                }
            );
        })
        .catch(function(error) {
            console.log(
                "Failed to create session description: %s",
                error.toString()
            );
        });
}

function pc_setup() {

    pc = new RTCPeerConnection(configuration)

    pc.onicecandidate = (event) => {
        console.log('webrtc/icecandidate: ' + event.candidate?.type + ' IP: ' + event.candidate?.candidate)
    }

    pc.ontrack = function(event) {
        const track = event.track
        let audio: HTMLAudioElement | null = document.querySelector('audio#audio')
        console.log('got remote track: kind=%s', track.kind);

        if (!audio) {
            return;
        }

        if (audio.srcObject !== event.streams[0]) {
            audio.srcObject = event.streams[0];
            console.log('received remote audio stream');
        }
    }

    pc.onicegatheringstatechange = () => {
        console.log('webrtc/iceGatheringState: ' + pc.iceGatheringState)
        switch (pc.iceGatheringState) {
            case 'new':
                /* gathering is either just starting or has been reset */
                break
            case 'gathering':
                /* gathering has begun or is ongoing */
                break
            case 'complete':
                api.sdp(pc.localDescription);
                break
        }
    }

    pc.onsignalingstatechange = () => {
        console.log('webrtc/signalingState: ' + pc.signalingState);
    }

    pc.onicecandidateerror = (event: any) => {
        console.log('ICE Candidate Error: ' + event.errorCode + ' ' + event.errorText + ' ' + event.url);
    }

    pc.addTransceiver('audio', { direction: 'recvonly' });
    pc.addTransceiver('video', { direction: 'recvonly' });
    pc_offer();
}

export default {
    listen() {
        pc_setup();
    },
}
