<?php

declare(strict_types=1);

use Tests\Client;
use Tests\TestCase;
use PHPUnit\Framework\Attributes\TestDox;


class ApiTest extends TestCase
{

    #[TestDox('GET /api/v1/sessions/connected')]
    public function test_sessions_connected()
    {
        $client = new Client();
        $response = $client->get('/api/v1/sessions/connected');
        $this->assertEquals(200, $response->getStatusCode());
    }

    #[TestDox('POST /api/v1/client/connect and SESSION')]
    public function test_client_connect()
    {
        $client = new Client();
        $r = $client->post('/api/v1/client/connect');
        $this->assertEquals(201, $r->getStatusCode());

        $session_id = $client->cookies->getCookieByName("mix_session")->getValue();
        $this->assertIsString($session_id);
    }

    #[TestDox('POST /api/v1/social')]
    public function test_social()
    {
        $client = new Client();
        $client->login();

        // Positive Test
        $r = $client->post('/api/v1/social', "social@social.studio.link");
        $this->assertEquals(200, $r->getStatusCode());

        $json = json_decode((string)$r->getBody());
        $this->assertIsString($json->id);
        $this->assertEquals(200, $json->status);
        $this->assertEquals("Studio Link", $json->name);

        // Negative Test
        $r = $client->post('/api/v1/social', "test@localhost");
        $this->assertEquals(200, $r->getStatusCode());

        $json = json_decode((string)$r->getBody());
        $this->assertEquals(404, $json->status);
    }

    #[TestDox('POST /api/v1/client/avatar')]
    public function test_client_avatar()
    {
        $client = new Client();
        $client->login();

        $image_webp =
            "\"data:image/png;base64,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
xVbaqOT/A9h1gw4GklKaAAAAAElFTkSuQmCC\"";

        $r = $client->post('/api/v1/client/avatar', str_replace(array("\n", "\r"), '', $image_webp));
        $this->assertEquals(201, $r->getStatusCode());
    }

    #[TestDox('POST /api/v1/client/name')]
    public function test_client_name()
    {
        $client = new Client();
        $client->login();

        $msg = $client->ws->receive();
        $json = json_decode($msg->getContent());
        $this->assertIsString($json->users[0]->id);

        $r = $client->post('/api/v1/client/name', "sreimers");
        $this->assertEquals(204, $r->getStatusCode());

        try {
            $r = $client->post('/api/v1/client/name', "extraloooooooooooooooooooooooong");
        } catch (Exception $error) {
            $this->assertEquals(400, $error->getCode());
        }
    }

    // ROUTE("/api/v1/chat", "GET")
    // ROUTE("/api/v1/chat", "POST")
}
