LIBFLUSH:=armageddon/libflush/build/x86/release/libflush.a

run: spectre
	./spectre

${LIBFLUSH}:
	make -C armageddon/libflush

spectre: spectre.cpp ${LIBFLUSH}
	g++ -I armageddon/libflush/libflush spectre.cpp ${LIBFLUSH} -O3 -o spectre

