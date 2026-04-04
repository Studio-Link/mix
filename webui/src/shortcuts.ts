import api from './api'
import { State } from './ws/state'

function solo_source(index: number) {
    const dev = State.sources.value[index].dev
    api.video_solo(dev, true)
    State.sources.value.forEach((item) => {
        if (dev === item.dev) item.solo = true
        else item.solo = false
    })
}

document.onkeydown = (event) => {
    if (document.activeElement?.tagName === 'INPUT')
        return

    if (document.activeElement?.tagName === 'TEXTAREA')
        return

    switch (event.code) {
        case "Digit1":
            solo_source(0)
            break
        case "Digit2":
            solo_source(1)
            break
        case "Digit3":
            solo_source(2)
            break
        case "Digit4":
            solo_source(3)
            break
        case "Digit5":
            solo_source(4)
            break
        case "Digit6":
            solo_source(5)
            break
        case "Digit7":
            solo_source(6)
            break
        case "Digit8":
            solo_source(7)
            break
    }
}
