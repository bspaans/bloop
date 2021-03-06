
bloop:
	rm -rf out/linux
	mkdir -p out/linux
	gcc -D SOKOL_GLCORE33 src/*.c -o out/linux/bloop -lm -lpthread -ldl -lGL $$(pkg-config --static --libs x11 xi xcursor) -lasound -I./lib/sokol -I./lib/Nuklear

bloop_emscripten:
	rm -rf out/wasm
	mkdir -p out/wasm
	emcc -D SOKOL_GLES2 src/*.c  -o out/wasm/bloop.html -lm -I./lib/sokol -I./lib/Nuklear

run: bloop
	./out/linux/bloop

host: bloop_emscripten
	cd out/wasm && python -m SimpleHTTPServer
