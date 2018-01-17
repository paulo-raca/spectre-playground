LIBFLUSH:=armageddon/libflush/build/x86/release/libflush.a

run: test_spectre
	./test_spectre

${LIBFLUSH}:
	make -C armageddon/libflush

test_spectre: test_spectre.cpp ${LIBFLUSH}
	g++ -I armageddon/libflush/libflush test_spectre.cpp ${LIBFLUSH} -O3 -o test_spectre

