all: ping watchdog
ping: ping.c
	gcc ping.c -o ping
watchdog: watchdog.c
	gcc watchdog.c -o watchdog
clean:
	rm -f *.o ping watchdog partb
