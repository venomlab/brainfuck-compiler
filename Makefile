.PHONY: \
	build \
	build-debug \
	build-release \
	build-sanitize \
	build-tsanitize \
	gcc-release-w64 \
	release \
	clean \
	lldb \
	run \
	run-debug \
	run-release \
	run-sanitize \
	run-tsanitize \
	sanitize \
	test \
	test-sanitize \
	test-tsanitize \
	tidy

CMAKE_CONFIG_INPUTS := \
	CMakeLists.txt \
	CMakePresets.json \
	$(wildcard cmake/platforms/*.cmake) \
	$(wildcard cmake/toolchains/*.cmake) \
	$(wildcard cmake/vcpkg/*/vcpkg.json)

clean:
	@rm -rf build

build/debug/build.ninja: $(CMAKE_CONFIG_INPUTS)
	@cmake --preset debug

build-debug: build/debug/build.ninja
	@cmake --build --preset debug

run-debug: build-debug
	@echo "-- PROGRAM OUTPUT:"
	@./build/debug/bfc

lldb: build-debug
	@lldb ./build/debug/bfc

build/clang-sanitize/build.ninja: $(CMAKE_CONFIG_INPUTS)
	@cmake --preset clang-sanitize

build-sanitize: build/clang-sanitize/build.ninja
	@cmake --build --preset clang-sanitize

run-sanitize: build-sanitize
	@echo "-- PROGRAM OUTPUT:"
	@./build/clang-sanitize/bfc

build/clang-tsanitize/build.ninja: $(CMAKE_CONFIG_INPUTS)
	@cmake --preset clang-tsanitize

build-tsanitize: build/clang-tsanitize/build.ninja
	@cmake --build --preset clang-tsanitize

run-tsanitize: build-tsanitize
	@echo "-- PROGRAM OUTPUT:"
	@./build/clang-tsanitize/bfc

build/clang-release/build.ninja: $(CMAKE_CONFIG_INPUTS)
	@cmake --preset clang-release

build-release: build/clang-release/build.ninja
	@cmake --build --preset clang-release

run-release: build-release
	@echo "-- PROGRAM OUTPUT:"
	@./build/clang-release/bfc

build/gcc-release-w64/build.ninja: $(CMAKE_CONFIG_INPUTS)
	@cmake --preset gcc-release-w64

gcc-release-w64: build/gcc-release-w64/build.ninja
	@cmake --build --preset gcc-release-w64

release: build-release gcc-release-w64
	@cmake \
		-DBFC_LINUX_BINARY="$(CURDIR)/build/clang-release/bfc" \
		-DBFC_WINDOWS_BINARY="$(CURDIR)/build/gcc-release-w64/bfc.exe" \
		-DBFC_CMAKE_CACHE="$(CURDIR)/build/clang-release/CMakeCache.txt" \
		-DBFC_DIST_DIR="$(CURDIR)/dist" \
		-P "$(CURDIR)/cmake/package-release.cmake"

test: build
	@ctest --preset tests

test-sanitize: build-sanitize
	@ctest --preset tests-sanitize

test-tsanitize: build-tsanitize
	@ctest --preset tests-tsanitize

sanitize: test-sanitize test-tsanitize

build/clang-tidy/build.ninja: $(CMAKE_CONFIG_INPUTS)
	@cmake --preset clang-tidy

tidy: build/clang-tidy/build.ninja
	@cmake --build --preset clang-tidy
	@ctest --test-dir build/clang-tidy --output-on-failure

build: build-debug
run: run-debug
