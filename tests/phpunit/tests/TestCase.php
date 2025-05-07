<?php

declare(strict_types=1);

namespace Tests;

use PHPUnit\Framework\TestCase as BaseTestCase;
use Tests\Client;


class TestCase extends BaseTestCase
{
    public static function setUpBeforeClass(): void
    {
        $client = new Client();

        shell_exec("cd ../.. &&" .
            "{ build/slmix -c config_example > /tmp/slmix.log 2>&1 & }");
        $start_count = 0;

        while ($start_count++ < 1000) {
            usleep(5000);
            try {
                $client->get("/api/v1/sessions/connected");
                break;
            } catch (\Exception $_) {
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
}
