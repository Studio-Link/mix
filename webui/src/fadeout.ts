let timeoutId: NodeJS.Timeout;

function startTimer(): void {
    timeoutId = setTimeout(fadeOut, 4000);
    fadeIn()
}

function resetTimer(): void {
    // Reset the timer when there is any user activity
    clearTimeout(timeoutId);
    startTimer();
}

function fadeIn(): void {
    const elements = document.querySelectorAll(".fadeout") as NodeListOf<HTMLElement>;
    elements.forEach(element => {
        element.style.opacity = "1";
    });
}

function fadeOut(): void {
    const elements = document.querySelectorAll(".fadeout") as NodeListOf<HTMLElement>;
    elements.forEach(element => {
        element.style.transition = "opacity 0.5s ease";
        element.style.opacity = "0";
    });
}

export const Fadeout = {
    init() {
        document.addEventListener("mousemove", resetTimer);
        document.addEventListener("mousedown", resetTimer);
        document.addEventListener("touchstart", resetTimer);
        document.addEventListener("touchmove", resetTimer);
        document.addEventListener("keydown", resetTimer);
    }
}
