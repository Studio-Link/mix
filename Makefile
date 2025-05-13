##############################################################################
#
# Build
#

.PHONY: build
build: external
	@[ -f "build/build.ninja" ] || cmake -B build -G Ninja -DUSE_TRACE=ON \
		-DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS="-gdwarf-4 -g3 -DJBUF_STAT -DRE_RTP_PCAP"
	@cmake --build build --parallel

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
	cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_FLAGS="-g -DJBUF_STAT"
	make build

.PHONY: systemd
systemd: external
	make clean
	cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_FLAGS="-g -DJBUF_STAT" -DUSE_SD_SOCK=ON
	make build

.PHONY: unix
unix: external
	make clean
	cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_FLAGS="-g -DJBUF_STAT" -DUSE_UNIX_SOCK=ON
	make build

external:
	mkdir -p external
	git clone --depth 1 \
		https://github.com/baresip/re.git external/re
	git clone --depth 1 -b playout_time \
		https://github.com/baresip/baresip.git external/baresip
	cd external/re && \
		patch -p1 < ../../patches/re_aubuf_timestamp_order_fix.patch

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
asan: external
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


##############################################################################
#
# Helpers
#

.PHONY: cloc
cloc:
	cloc --exclude-dir='node_modules,external,build,env' --exclude-ext='json' .

.PHONY: update
update: external
	cd external/re && git pull
	cd external/rem && git pull
	cd external/baresip && git pull

.PHONY: avatars
avatars:
	mkdir -p webui/public/avatars

.PHONY: run
run: build avatars
	build/slmix -c config_example

.PHONY: gdb
gdb: build avatars
	gdb --args build/slmix -c config_example

.PHONY: clean
clean:
	rm -Rf build

.PHONY: cleaner
cleaner: clean
	rm -Rf external

.PHONY: fresh
fresh: clean build

.PHONY: ccheck
ccheck:
	tests/ccheck.py src modules

.PHONY: test
test:
	cd tests/phpunit && vendor/bin/phpunit
