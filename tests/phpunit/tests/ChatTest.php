<?php

declare(strict_types=1);

use Tests\Client;
use Tests\ClientAuth;
use Tests\TestCase;

class ChatTest extends TestCase
{
    public function test_Alice_sees_Bobs_Chat_msgs()
    {
        $alice = new Client();
        $alice->login("alice", ClientAuth::Host);

        $bob = new Client();
        $bob->login("bob", ClientAuth::Audience);

        $msg = $alice->ws_next(); /* connect websocket */
        $this->assertEquals("users", $msg->type);

        $bob->post("/api/v1/chat", "\"Hello World\"");
        $msg = $alice->ws_next("chat", "chat_added");
        $this->assertEquals("Hello World", $msg->msg);
    }
}
