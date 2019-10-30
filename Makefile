alsadump.so: alsadump.c
	gcc -shared -fPIC -o alsadump.so alsadump.c

clean:
	rm alsadump.so