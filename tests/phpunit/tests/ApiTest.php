<?php

declare(strict_types=1);

use PHPUnit\Framework\TestCase;
use Tests\Client;
use PHPUnit\Framework\Attributes\TestDox;


class ApiTest extends TestCase
{
    public static function setUpBeforeClass(): void
    {
        $client = new Client();

        shell_exec("cd ../.. &&" .
            "{ build/slmix -c config_example > /tmp/slmix.log 2>&1 & }");
        $start_count = 0;

        while ($start_count++ < 1000) {
            usleep(10000);
            try {
                $client->get("/api/v1/sessions/connected");
                break;
            } catch (Exception $_) {
                continue;
            }
        }
    }

    public static function tearDownAfterClass(): void
    {
        system("pkill slmix");
    }

    protected function setUp(): void {}

    protected function tearDown(): void
    {

        if (!$this->status()->isSuccess()) {
            system("cat /tmp/slmix.log");
        }
    }

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

/*
    #[TestDox('POST /api/v1/social')]
    public function test_social()
    {
        $client = new Client();
        $client->getSession();

        // Positive Test
        $r = $client->post('/api/v1/social', "social@social.studio.link");
        $this->assertEquals(200, $r->getStatusCode());

        $json = json_decode((string)$r->getBody());
        $this->assertIsString($json->id);
        $this->assertEquals(200, $json->status);
        $this->assertEquals("Studio Link", $json->name);

        // Negative Test
        $r = $client->post('/api/v1/social', "test@example.net");
        $this->assertEquals(200, $r->getStatusCode());

        $json = json_decode((string)$r->getBody());
        $this->assertEquals(404, $json->status);
    }
*/

    #[TestDox('POST /api/v1/client/avatar')]
    public function test_client_avatar() {}

    #[TestDox('POST /api/v1/client/name')]
    public function test_client_name()
    {
        $client = new Client();
        $client->getSession();

        $r = $client->post('/api/v1/client/name', "sreimers");
        $this->assertEquals(204, $r->getStatusCode());
    }
    // ROUTE("/api/v1/client/avatar", "POST")
    // ROUTE("/api/v1/client/name", "POST")
    // ROUTE("/api/v1/webrtc/sdp/offer", "PUT")
    // ROUTE("/api/v1/webrtc/sdp/answer", "PUT")
    // ROUTE("/api/v1/webrtc/sdp/candidate", "PUT")
    // ROUTE("/api/v1/record/enable", "PUT")
    // ROUTE("/api/v1/record/audio/enable", "PUT")
    // ROUTE("/api/v1/record/disable", "PUT")
    // ROUTE("/api/v1/client/speaker", "POST")
    // ROUTE("/api/v1/client/listener", "POST")
    // ROUTE("/api/v1/hand/enable", "PUT")
    // ROUTE("/api/v1/hand/disable", "PUT")
    // ROUTE("/api/v1/webrtc/video/enable", "PUT")
    // ROUTE("/api/v1/webrtc/video/disable", "PUT")
    // ROUTE("/api/v1/webrtc/audio/enable", "PUT")
    // ROUTE("/api/v1/webrtc/audio/disable", "PUT")
    // ROUTE("/api/v1/webrtc/focus", "PUT")
    // ROUTE("/api/v1/webrtc/solo/enable", "PUT")
    // ROUTE("/api/v1/webrtc/solo/disable", "PUT")
    // ROUTE("/api/v1/webrtc/disp/enable", "PUT")
    // ROUTE("/api/v1/webrtc/disp/disable", "PUT")
    // ROUTE("/api/v1/webrtc/amix/enable", "PUT")
    // ROUTE("/api/v1/webrtc/amix/disable", "PUT")
    // ROUTE("/api/v1/webrtc/stats", "POST")
    // ROUTE("/api/v1/client/hangup", "POST")
    // ROUTE("/api/v1/client", "DELETE")
    // ROUTE("/api/v1/emoji", "PUT")
    // ROUTE("/api/v1/chat", "GET")
    // ROUTE("/api/v1/chat", "POST")
}
