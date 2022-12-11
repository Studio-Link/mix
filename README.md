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
apt install curl wget ninja pkg-config clang cmake make git patch ca-certificates \
	libgd-dev libopus-dev libz-dev libssl-dev libavformat-dev libavcodec-dev libflac-dev
```

Node.js v18.x LTS or v19.x (Current)
https://github.com/nodesource/distributions/blob/master/README.md#debinstall


### Arch Linux

```bash
pacman -S curl wget ninja pkgconf clang cmake make git patch ca-certificates \
	gd opus zlib ffmpeg flac nodejs npm
```

## Installation

```bash
git clone https://github.com/Studio-Link/mix.git
cd mix
make release
make webui
```

## License

[MIT](https://github.com/Studio-Link/mix/blob/main/LICENSE)
