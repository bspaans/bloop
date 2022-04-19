
bloop:
	rm -rf out/linux
	mkdir -p out/linux
	gcc -D SOKOL_GLCORE33 src/*.c  -o out/linux/bloop -lm -lpthread -ldl -lGL $$(pkg-config --static --libs x11 xi xcursor) -lasound -I./lib/sokol

bloop_emscripten:
	rm -rf out/wasm
	mkdir -p out/wasm
	emcc -D SOKOL_GLES2 bloop.c  -o out/wasm/bloop.html -lm -I./lib/sokol

run: bloop
	./out/linux/bloop
