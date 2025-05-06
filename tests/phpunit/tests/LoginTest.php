<?php

declare(strict_types=1);

use Tests\Client;
use Tests\TestCase;

class LoginTest extends TestCase
{
    public function test_Alice_sees_Bob_on_websocket()
    {
        $alice = new Client();
        $bob   = new Client();

        $alice->getSession();
        $bob->getSession();

        $alice->login("alice");
        $bob->login("bob");

        $msg_bob = json_decode($bob->ws->receive()->getContent());
        $this->assertEquals("bob", $msg_bob->users[0]->name);

        $msg_alice = json_decode($alice->ws->receive()->getContent());
        $this->assertEquals("alice", $msg_alice->users[0]->name);
        $this->assertEquals("bob", $msg_alice->users[1]->name);
    }
}
