import api from './api'

export { }

declare var window: any;

window.console = (function (origConsole: any) {
    if (!window.console || !origConsole) {
        origConsole = {};
    }

    let isDebug = true;

    return {
        log: function () {
            this.addLog(arguments, "info");
            isDebug && origConsole.log && origConsole.log.apply(origConsole, arguments);
        },
        warn: function () {
            this.addLog(arguments, "warning");
            isDebug && origConsole.warn && origConsole.warn.apply(origConsole, arguments);
        },
        error: function () {
            this.addLog(arguments, "error");
            isDebug && origConsole.error && origConsole.error.apply(origConsole, arguments);
        },
        info: function () {
            this.addLog(arguments, "info");
            isDebug && origConsole.info && origConsole.info.apply(origConsole, arguments);
        },
        addLog: function (args: any, type: string) {
            api.log(args, type)
        },
        debug: function (bool: boolean) {
            isDebug = bool;
        },
    };
})(window.console);
