.PHONY: build
build: external
	@[ -f "build/build.ninja" ] || cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
	@cmake --build build --parallel

.PHONY: avatars
avatars:
	mkdir -p webui/public/avatars

.PHONY: run
run: build avatars
	build/slmix

.PHONY: clean
clean:
	rm -Rf build

.PHONY: cleaner
cleaner: clean
	rm -Rf external

.PHONY: fresh
fresh: clean build

.PHONY: update
update: external
	cd external/re && git pull
	cd external/rem && git pull
	cd external/baresip && git pull

.PHONY: webui
webui:
	cd webui && npm run dev

.PHONY: release
release:
	make clean
	cmake -B build -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo
	make build

.PHONY: systemd
systemd:
	make clean
	cmake -B build -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUSE_SD_SOCK=ON
	make build

external:
	mkdir -p external
	git clone https://github.com/baresip/re.git external/re && cd external/re && git checkout unix_sockets
	git clone https://github.com/baresip/rem.git external/rem
	git clone https://github.com/baresip/baresip.git external/baresip && \
		cd external/baresip && \
		patch -p1 < ../../patches/baresip_packet_dup_handler.patch && \
		patch -p1 < ../../patches/mediatrack_audio_video.patch && \
		patch -p1 < ../../patches/avcodec_simple_decode_recorder.patch
