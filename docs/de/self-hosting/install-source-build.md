# Bauen und konfigurieren

## Checkout und make

```bash
sudo git clone https://github.com/Studio-Link/mix.git /opt/slmix
sudo useradd slmix -d /opt/slmix -s /bin/bash -U
sudo chown -R slmix:slmix /opt/slmix
sudo su - slmix
make release
make webui
```

## Config - /opt/slmix/config

```
mix_token_host          TOKENREPLACEME # can start record
mix_token_guests        TOKENREPLACEME # invite url 
mix_token_download      TOKENREPLACEME # protected download folder 
```

::: tip
Bitte verwende nur Buchstaben und Zahlen als Token (keine Sonderzeichen!).
Du kannst ein solches Token mit dem Kommandozeilentool `pwgen 24 4` oder über die Webseite
[random.org](https://www.random.org/strings/?num=4&len=24&digits=on&upperalpha=on&loweralpha=on&unique=on&format=html&rnd=new) generieren.
:::

## Nginx Config  <Badge type="warning" text="SSL is needed for WebRTC" />

```nginx
server {
        listen 443 http2 ssl;
        listen [::]:443 http2 ssl;
        server_name mix.example.net;

        ssl_certificate /path/to/signed_cert_plus_intermediates;
        ssl_certificate_key /path/to/private_key;
        ssl_trusted_certificate /path/to/root_CA_cert_plus_intermediates;

        ssl_stapling on;
        ssl_stapling_verify on;

        add_header X-XSS-Protection "1; mode=block";
        add_header X-Content-Type-Options "nosniff";
        add_header Strict-Transport-Security max-age=15768000;

        root /opt/slmix/webui/dist;

        location /api {
                proxy_pass http://127.0.0.1:9999;
                proxy_set_header X-Forwarded-Host $host;
                proxy_set_header X-Forwarded-Server $host;
                proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
                proxy_set_header Host $http_host;
        }

        location /ws {
                proxy_pass http://127.0.0.1:9999;
                proxy_redirect off;

                # Allow the use of websockets
                proxy_http_version 1.1;
                proxy_set_header Upgrade $http_upgrade;
                proxy_set_header Connection 'upgrade';
                proxy_set_header Host $host;
                proxy_cache_bypass $http_upgrade;
        }

        location ~* \.(?:ico|css|js|gif|jpe?g|png|webp)$ {
                expires 90d;
                add_header Vary Accept-Encoding;
                access_log off;
        }

        location / {
                expires off;
                add_header Cache-Control "public, max-age=0, s-maxage=0, must-revalidate" always;
                try_files $uri /index.html =404;
        }
}
```

## Systemd (/etc/systemd/system/slmix.service)
```systemd
[Unit]
Description=slmix
After=syslog.target network.target

[Service]
Type=simple
User=slmix
Group=slmix
WorkingDirectory=/opt/slmix/
ExecStart=/opt/slmix/build/slmix -c /opt/slmix/config 
LimitNOFILE=2048

[Install]
WantedBy=multi-user.target
```

```bash
systemctl enable slmix
systemctl start slmix
```

::: tip Weitere Informationen zur Bedienung
[Bedienung - Howto](/de/hosted/howto/login)
:::
