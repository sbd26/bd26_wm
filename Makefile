install:
	gcc -lX11 -lXcursor -lXft -lfontconfig -I/usr/include/freetype2 -O3 -ffast-math -Wall -Wextra -lXcomposite bd26.c -o bd26_beta 
	cp bd26_beta /usr/bin/bd26_beta
	cp bar_util.sh /bin

clean:
	rm -f bd26_beta
	rm -f /usr/bin/bd26_beta
