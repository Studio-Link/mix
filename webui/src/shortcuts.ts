import api from './api'

document.onkeydown = (event) => {
    if (document.activeElement?.tagName === 'INPUT')
        return

    if (document.activeElement?.tagName === 'TEXTAREA')
        return

    switch (event.code) {
        case "Digit1":
            api.track_focus(1)
            break
        case "Digit2":
            api.track_focus(2)
            break
        case "Digit3":
            api.track_focus(3)
            break
        case "Digit4":
            api.track_focus(4)
            break
        case "Digit5":
            api.track_focus(5)
            break
        case "Digit6":
            api.track_focus(6)
            break
        case "Digit7":
            api.track_focus(7)
            break
        case "Digit8":
            api.track_focus(8)
            break
    }
}
