.PHONY: build
build: external
	@[ -f "build/build.ninja" ] || cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
	@cmake --build build --parallel

.PHONY: avatars
avatars:
	mkdir -p webui/public/avatars

.PHONY: run
run: build avatars
	build/slmix -c config_example

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
	cd webui && npm install && npm run build-only
	rm -Rf webui/dist/avatars
	rm -Rf webui/dist/download
	mkdir -p webui/public/avatars
	ln -s ../public/avatars webui/dist/
	ln -s ../public/download webui/dist/

.PHONY: release
release: external
	make clean
	cmake -B build -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo
	make build

.PHONY: systemd
systemd: external
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
		patch -p1 < ../../patches/baresip_stream_enable.patch && \
		patch -p1 < ../../patches/baresip_video_remove_sendq_empty.patch

.PHONY: cloc
cloc:
	cloc --exclude-dir='node_modules,external,build' .

##############################################################################
#
# Sanitizers
#

.PHONY: run_san
run_san:
	ASAN_OPTIONS=fast_unwind_on_malloc=0 \
	# TSAN_OPTIONS="suppressions=tsan.supp" \
	make run

.PHONY: asan
asan:
	make clean
	cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_C_FLAGS="-fsanitize=undefined,address \
		-fno-omit-frame-pointer" \
		-DHAVE_THREADS=
	make build

.PHONY: tsan
tsan:
	make clean
	cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_C_FLAGS="-fsanitize=undefined,thread \
		-fno-omit-frame-pointer" \
		-DHAVE_THREADS=
	make build
