# Docker

```bash
docker pull ghcr.io/studio-link/mix/slmix:v1.0.0-beta
docker run --name slmix -it -p8080:80 \
    -e TOKENHOST=host_token \
    -e TOKENDOWNLOAD=downloadsecret \
    -e TOKENGUEST=guest_token \
    -e TOKENAPI=api_token \
    slmix:v1.0.0-beta
```

::: tip
Please only use letters and numbers in your token. Do not use any special characters.
You can generate a token using the command line tool `pwgen 24 4`, or via the
[random.org](https://www.random.org/strings/?num=4&len=24&digits=on&upperalpha=on&loweralpha=on&unique=on&format=html&rnd=new) website.
:::

## Nginx reverse proxy example

```nginx
server {
    listen 80 default_server;
    listen [::]:80 ipv6only=on default_server;
    server_name slmix.mydomain.com;
    location / {
        return 301 https://$host$request_uri;
    }
}
server {
    listen 443 default_server;
    listen [::]:443 ipv6only=on default_server;
    server_name slmix.mydomain.com;

    ssl on;
    ssl_certificate /path/to/cert.chain.pem;
    ssl_certificate_key /path/to/key.pem;

    location /ws {
        proxy_pass http://localhost:8080;
        proxy_redirect off;

        # Allow the use of websockets
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_cache_bypass $http_upgrade;
    }

    location / {
        proxy_pass http://localhost:8080;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto https;
        proxy_set_header Host $http_host;
    }
}
```
