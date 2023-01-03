all: PartA watchdog PartB
PartA: ping.c
	gcc ping.c -o PartA
watchdog: watchdog.c
	gcc watchdog.c -o watchdog
PartB:betterping.c
	gcc betterping.c -o PartB
clean:
	rm -f *.o PartA watchdog PartB
