all:
	cmake . -B build && make -C build && cp build/compile_commands.json .

run:
	./build/Vulk-bin
