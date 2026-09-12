.PHONY: build run test sanitize tidy clean
clean:
	@rm -rf build
build-debug:
	@cmake --preset debug
	@cmake --build --preset debug
run-debug: build-debug
	@echo "-- PROGRAM OUTPUT:"
	@./build/debug/bfc
lldb: build-debug
	@lldb ./build/debug/bfc
build-sanitize:
	@cmake --preset clang-sanitize
	@cmake --build --preset clang-sanitize
run-sanitize: build-sanitize
	@echo "-- PROGRAM OUTPUT:"
	@./build/clang-sanitize/bfc
build-tsanitize:
	@cmake --preset clang-tsanitize
	@cmake --build --preset clang-tsanitize
run-tsanitize: build-tsanitize
	@echo "-- PROGRAM OUTPUT:"
	@./build/clang-tsanitize/bfc
build-release:
	@cmake --preset clang-release
	@cmake --build --preset clang-release
run-release: build-release
	@echo "-- PROGRAM OUTPUT:"
	@./build/clang-release/bfc
test: build
	@ctest --preset tests
test-sanitize: build-sanitize
	@ctest --preset tests-sanitize
test-tsanitize: build-tsanitize
	@ctest --preset tests-tsanitize
sanitize: test-sanitize test-tsanitize
tidy:
	@cmake --preset clang-tidy
	@cmake --build --preset clang-tidy
	@ctest --test-dir build/clang-tidy --output-on-failure

build: build-debug
run: run-debug
