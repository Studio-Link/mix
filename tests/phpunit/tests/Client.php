<?php

declare(strict_types=1);

namespace Tests;

define("SERVER", "http://127.0.0.1:9999");

use \GuzzleHttp\Client as HttpClient;


class Client
{
    private HttpClient $client;
    public $cookies;

    function __construct()
    {
        $this->cookies = new \GuzzleHttp\Cookie\CookieJar;
        $this->client = new HttpClient(['base_uri' => 'http://127.0.0.1:9999']);
    }

    function get($url) {
        return $this->client->request('GET', $url, ['cookies' => $this->cookies]);
    }

    function post($url, $body = NULL) {
        return $this->client->request('POST', $url, ['cookies' => $this->cookies, 'body' => $body]);
    }

    function getSession() {
        $this->post('/api/v1/client/connect');
    }
}
