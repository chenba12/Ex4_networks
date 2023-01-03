all: ping watchdog better_ping
ping: ping.c
	gcc ping.c -o ping
watchdog: watchdog.c
	gcc watchdog.c -o watchdog
better_ping:betterping.c
	gcc betterping.c -o better_ping
clean:
	rm -f *.o ping watchdog betterping
