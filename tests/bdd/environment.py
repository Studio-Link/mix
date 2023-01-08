import subprocess
import time


def before_all(context):
    context.base_url = 'http://127.0.0.1:9999'
    context.ws = {}
    context.sessid = {}
    context.response = {}
    context.proc = subprocess.Popen(["./build/slmix"], cwd="../../.")
    time.sleep(0.2)


def after_all(context):
    context.proc.terminate()
    context.proc.communicate()
