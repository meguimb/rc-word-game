make: udp_server.c word_image.c
	gcc -o udp_server udp_server.c word_image.c -I.
clean:
	rm -f GAMES/*.txt