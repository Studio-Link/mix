import subprocess
import time
import socket


def is_port_reachable(host, port):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((host, port))
        return True
    except Exception:
        return False
    finally:
        s.close()


def wait_until_port_reachable(host, port, timeout):
    start_time = time.time()
    while not is_port_reachable(host, port):
        if time.time() - start_time > timeout:
            raise TimeoutError(
                'Timed out waiting for port to become reachable')
        time.sleep(0.01)


def before_all(context):
    context.base_url = 'http://127.0.0.1:9999'
    context.ws = {}
    context.sessid = {}
    context.response = {}
    context.proc = subprocess.Popen(["./build/slmix"], cwd="../../.")
    wait_until_port_reachable('127.0.0.1', 9999, 1)


def after_all(context):
    context.proc.terminate()
    context.proc.communicate()
