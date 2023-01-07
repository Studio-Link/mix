from behave import then, given
import requests
import json
from websocket import create_connection


@given('"{user}" connects without a token')
def step_impl1(context, user):
    response = requests.post(context.base_url + '/api/v1/client/connect')
    context.sessid[user] = response.headers['Session-ID']
    assert response.headers['Session-ID'] is not None, response.headers[
        "Session-ID"]


@then('"{user}" WebSocket receives "{count}" users')
def step_impl2(context, user, count):
    ws = create_connection("ws://127.0.0.1:9999/ws/v1/users")
    ws.send(context.sessid[user])
    context.response[user] = ws.recv()
    context.ws[user] = ws
    resp = json.loads(context.response[user])
    assert resp["type"] == 'users', f'users type: {resp}'
    assert len(resp["users"]) == int(count), f'users count: {resp}'


@given('"{user}" set client name')
def step_impl3(context, user):
    headers = {'Session-ID': context.sessid[user]}
    response = requests.post(context.base_url + '/api/v1/client/name',
                             headers=headers,
                             data=user)
    assert response.ok, f'Error: {response.status_code}'


@given('"{user}" upload a avatar')
def step_impl5(context, user):
    headers = {'Session-ID': context.sessid[user]}

    with open('steps/data/avatar.base64', 'r') as f:
        avatar = f.read().replace('\n', '')

    response = requests.post(context.base_url + '/api/v1/client/avatar',
                             headers=headers,
                             data=avatar)
    assert response.ok, f'Error: {response.status_code}'
    context.response[user] = response


@then('"{user}" authorized as audience user')
def step_impl6(context, user):
    resp = context.response[user].json()
    assert resp["type"] == 'user', f'Error user: {resp}'
    assert resp["event"] == 'added', f'Error event: {resp}'
    assert not resp["speaker"], f'Error speaker: {resp}'
    assert not resp["host"], f'Error host: {resp}'
    assert not resp["audio"], f'Error audio: {resp}'
    assert not resp["video"], f'Error video: {resp}'
    assert not resp["hand"], f'Error hand: {resp}'
    assert not resp["webrtc"], f'Error webrtc: {resp}'
    assert resp["id"], f'Error id: {resp}'


@then('"{user}" closes WebSocket')
def step_impl7(context, user):
    context.ws[user].close()


@then('"{user}" WebSocket receives added "{updated_user}" user')
def step_impl8(context, user, updated_user):
    response = context.ws[user].recv()
    resp = json.loads(response)
    assert resp["type"] == 'user', f'type: {resp}'
    assert resp["event"] == 'added', f'event: {resp}'
    assert resp["name"] == updated_user, f'name: {resp}'


@then('"{user}" WebSocket receives "{delete_user}" delete')
def step_impl9(context, user, delete_user):
    response = context.ws[user].recv()
    resp = json.loads(response)
    assert resp["type"] == 'user', f'user type: {resp}'
    assert resp["event"] == 'deleted', f'deleted event: {resp}'
    assert resp["name"] == delete_user, f'user name: {resp}'


@then('"{user}" logouts')
def step_impl10(context, user):
    headers = {'Session-ID': context.sessid[user]}
    resp = requests.delete(context.base_url + '/api/v1/client',
                           headers=headers)
    assert resp.ok, f'Error: {resp.status_code}'
