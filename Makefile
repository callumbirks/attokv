.PHONY: configure build run

configure:
	cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1
build:
	cmake --build build
run:
	./build/kvcli
clean:
	rm -rf build
