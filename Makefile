.PHONY: build
build: external/re
	@[ ! -f "build/build.ninja" ] && cmake -B build -G Ninja || true
	@cmake --build build --parallel

external/re:
	mkdir -p external
	git clone https://github.com/baresip/re.git external/re
	git clone https://github.com/baresip/rem.git external/rem
	git clone https://github.com/baresip/baresip.git external/baresip

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
