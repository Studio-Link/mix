<?php

declare(strict_types=1);

namespace Tests;

use \GuzzleHttp\Client as HttpClient;

enum ClientAuth: String
{
    case Host = "TOKENHOST";
    case Guest = "TOKENGUEST";
    case Audience = "";
}

class Client
{
    private HttpClient $client;
    public $cookies;
    public $ws;

    function __construct()
    {
        $this->cookies = new \GuzzleHttp\Cookie\CookieJar;
        $this->client = new HttpClient(['base_uri' => 'http://127.0.0.1:9999']);
        $this->ws = new \WebSocket\Client("ws://127.0.0.1:9999/ws/v1/users");
    }

    function get($url)
    {
        return $this->client->request('GET', $url, ['cookies' => $this->cookies]);
    }

    function delete($url)
    {
        return $this->client->request('DELETE', $url, ['cookies' => $this->cookies]);
    }

    function post($url, $body = NULL)
    {
        return $this->client->request('POST', $url, ['cookies' => $this->cookies, 'body' => $body]);
    }

    function login($name = NULL, ClientAuth $auth = ClientAuth::Audience)
    {
        $this->post('/api/v1/client/connect', $auth->value);
        $session_id = $this->cookies->getCookieByName("mix_session")->getValue();
        $this->ws->addHeader("Cookie", "mix_session=" . $session_id);

        if ($name)
            $this->post('/api/v1/client/name', $name);
    }

    function logout()
    {
        return $this->delete('/api/v1/client');
    }

    function ws_next(string | NULL $type = NULL, string | NULL $event = NULL)
    {
        if (!$type) {
            $json = json_decode($this->ws->receive()->getContent());
            return $json;
        }

        $cnt = 10;
        while ($cnt--) {
            $json = json_decode($this->ws->receive()->getContent());
            if ($type === $json->type) {
                if (!$event)
                    return $json;
                else if ($json->event === $event)
                    return $json;
            }
        }

        return NULL;
    }
}
