.PHONY: build
build: external
	@[ -f "build/build.ninja" ] || cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
	@cmake --build build --parallel

.PHONY: run
run: build
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
	make cleaner
	make external
	cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release
	make build

external:
	mkdir -p external
	git clone https://github.com/baresip/re.git external/re
	git clone https://github.com/baresip/rem.git external/rem
	git clone https://github.com/baresip/baresip.git external/baresip
