# Studio Link - Mix

Next generation decentralized social network with voice/video and broadcasting support.


## Roadmap

- OAuth2 support
- ActivityPub
- WebRTC
- RTMP Broadcasting
- Server/Client Recording

## Development

Compile and start backend

```sh
$ make
$ build/slmix
```

Start frontend

```sh
$ cd webui
$ npm install
$ npm run dev
```


## Requirements

### Debian/Ubuntu

```bash
apt install curl wget ninja-build pkg-config clang cmake make git patch ca-certificates \
	libgd-dev libopus-dev libz-dev libssl-dev libavformat-dev libavcodec-dev libflac-dev
```

Node.js v18.x LTS or v19.x (Current)
https://github.com/nodesource/distributions/blob/master/README.md#debinstall


### Arch Linux

```bash
pacman -S curl wget ninja pkgconf clang cmake make git patch ca-certificates \
	gd opus zlib ffmpeg flac nodejs npm
```

## Installation from Source

```bash
git clone https://github.com/Studio-Link/mix.git /opt/slmix
useradd slmix -d /opt/slmix -s /bin/bash
groupadd slmix
chown -R slmix:slmix /opt/slmix
su - slmix
make release
cd webui && npm install && npm run build
```

### Nginx Config

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

### Systemd (/etc/systemd/system/slmix.service)
```/etc/systemd/system/slmix.service
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

### Config - /opt/slmix/config

```
mix_token_host          TOKENREPLACEME # can start record
mix_token_guests        TOKENREPLACEME # invite url 
mix_token_download      TOKENREPLACEME # protected download folder 
```

## License

[MIT](https://github.com/Studio-Link/mix/blob/main/LICENSE)
