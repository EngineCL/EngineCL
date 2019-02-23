.PHONY: build/debug build/release

export FROM_MAKEFILE=y

export CC ?= clang
export CXX ?= clang++

CMAKE ?= /usr/bin/cmake

DIR_DEBUG = build/debug
DIR_RELEASE = build/release

all: debug

build/debug-semaphore:
	mkdir -p $(DIR_DEBUG); cd $(DIR_DEBUG); \
	$(CMAKE) ../.. -DCMAKE_BUILD_TYPE=Debug && make Semaphore

dir/debug:
	mkdir -p $(DIR_DEBUG)

dir/release:
	mkdir -p $(DIR_RELEASE)

clean:
	rm -r build

build/debug: dir/debug
	cd $(DIR_DEBUG); \
	$(CMAKE) ../.. -DCMAKE_BUILD_TYPE=Debug -DTESTS=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && make

build/release: dir/release
	cd $(DIR_RELEASE); \
	$(CMAKE) ../.. -DECL_LOGGING=0 -DTESTS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && make

build/debug-test: dir/debug
	cd $(DIR_DEBUG); \
	$(CMAKE) ../.. -DCMAKE_BUILD_TYPE=Debug -DTESTS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && make
