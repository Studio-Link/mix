<?php

declare(strict_types=1);

namespace Tests;

use PHPUnit\Framework\TestCase as BaseTestCase;
use Tests\Client;


class TestCase extends BaseTestCase
{
    protected function setUp(): void {
        $client = new Client();

        shell_exec("cd ../.. &&" .
            "{ build/slmix -c config_example > /tmp/slmix.log 2>&1 & }");
        $start_count = 0;

        while ($start_count++ < 1000) {
            try {
                $client->get("/api/v1/sessions/connected");
                break;
            } catch (\Exception $_) {
                usleep(100);
                continue;
            }
        }
    }

    protected function tearDown(): void
    {
        system("pkill slmix");
        if (!$this->status()->isSuccess()) {
            system("cat /tmp/slmix.log");
        }
    }
}
