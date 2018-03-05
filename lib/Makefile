LIBFLUSH:=../armageddon/libflush/build/x86/release/libflush.a

run: spectre
	./spectre

${LIBFLUSH}:
	make -C ../armageddon/libflush

spectre: *.cpp *.h ${LIBFLUSH}
	g++ -I libflush *.cpp ${LIBFLUSH} -O3 -o spectre

