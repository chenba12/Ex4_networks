all: ping watchdog better_ping
ping: ping.c
	gcc ping.c -o ping
watchdog: watchdog.c
	gcc watchdog.c -o watchdog
better_ping:newPing.c
	gcc newPing.c -o better_ping
clean:
	rm -f *.o ping watchdog partb
