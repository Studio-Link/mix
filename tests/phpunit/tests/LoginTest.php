<?php

declare(strict_types=1);

use Tests\Client;
use Tests\ClientAuth;
use Tests\TestCase;

class LoginTest extends TestCase
{
    public function test_Alice_sees_Bob_on_websocket()
    {
        $alice = new Client();
        $bob   = new Client();

        $alice->login();
        $bob->login();

        $alice->login("alice");
        $bob->login("bob");

        $msg_bob = json_decode($bob->ws->receive()->getContent());
        $this->assertEquals("bob", $msg_bob->users[0]->name);
        $this->assertFalse($msg_bob->users[0]->speaker);

        $msg_alice = json_decode($alice->ws->receive()->getContent());
        $this->assertEquals("alice", $msg_alice->users[0]->name);
        $this->assertFalse($msg_alice->users[0]->speaker);
        $this->assertEquals("bob", $msg_alice->users[1]->name);
    }


    public function test_Alice_logins_as_host()
    {
        $alice = new Client();
        $alice->login("alice", ClientAuth::Host);

        $msg = json_decode($alice->ws->receive()->getContent());

        $this->assertTrue($msg->users[0]->host);
    }


    public function test_Alice_re_auth_as_host()
    {
        $alice = new Client();
        $alice->login("alice", ClientAuth::Guest);
        $alice->login("alice", ClientAuth::Host);

        $msg = json_decode($alice->ws->receive()->getContent());

        $this->assertTrue($msg->users[0]->host);
    }


    public function test_Alice_logout()
    {
        $alice = new Client();
        $alice->login("alice");

        $bob = new Client();
        $bob->login("bob");

        $msg_bob = $bob->ws_next();
        $this->assertEquals("users", $msg_bob->type);

        $alice->ws->close();
        $r = $alice->logout();
        $this->assertEquals(204, $r->getStatusCode());

        $msg_bob = $bob->ws_next("user", "deleted");

        $this->assertEquals("user", $msg_bob->type);
        $this->assertEquals("deleted", $msg_bob->event);

    }
}
