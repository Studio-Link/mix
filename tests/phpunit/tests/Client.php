<?php

declare(strict_types=1);

namespace Tests;

define("SERVER", "http://127.0.0.1:9999");

use \GuzzleHttp\Client as HttpClient;


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

    function post($url, $body = NULL)
    {
        return $this->client->request('POST', $url, ['cookies' => $this->cookies, 'body' => $body]);
    }

    function getSession()
    {
        $this->post('/api/v1/client/connect');
        $session_id = $this->cookies->getCookieByName("mix_session")->getValue();
        $this->ws->addHeader("Cookie", "mix_session=" . $session_id);
    }
}
