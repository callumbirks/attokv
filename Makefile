.PHONY: configure build run

configure:
	cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1
build:
	cmake --build build
build_server:
	cmake --build --target server
build_client:
	cmake --build --target client
clean:
	rm -rf build
