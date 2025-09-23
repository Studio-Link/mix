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
	//ignore global events if input field is focused
	const inputs = document.getElementsByTagName('input');
	for (let i = 0, len = inputs.length; i < len; i++) {
		const input = inputs[i];
		if (input === document.activeElement)
			return
	}
	const textareas = document.getElementsByTagName('textarea');
	for (let i = 0, len = textareas.length; i < len; i++) {
		const textarea = textareas[i];
		if (textarea === document.activeElement)
			return
	}
	// --- Track shortcuts ---
	if (event.code == 'Digit1') {
        solo_source(0)
	}
	if (event.code == 'Digit2') {
        solo_source(1)
	}
	if (event.code == 'Digit3') {
        solo_source(2)
	}
	if (event.code == 'Digit4') {
        solo_source(3)
	}
	if (event.code == 'Digit5') {
        solo_source(4)
	}
	if (event.code == 'Digit6') {
        solo_source(5)
	}
}
