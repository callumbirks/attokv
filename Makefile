.PHONY: configure configure_release build build_release build_server build_client build_benchmark clean

configure:
	cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1
build:
	cmake --build build
configure_release:
	cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=1
build_release:
	cmake --build build-release
build_server:
	cmake --build build --target attokv_server
build_client:
	cmake --build build --target attokv_client
build_benchmark:
	cmake --build build --target attokv_benchmark
clean:
	rm -rf build build-release
