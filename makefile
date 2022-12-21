make: server.c word_image.c client.c clientTCP.c
	gcc -o server server.c word_image.c -I.
	gcc -o client client.c clientTCP.c
clean:
	rm -f GAMES/*.txt
	rm udp_server server client 