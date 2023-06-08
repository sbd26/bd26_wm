install:
	gcc -lX11 -lXcursor -O3 -ffast-math -Wall -Wextra -lXcomposite -lXft -lfontconfig -I /usr/include/freetype2 bd26.c -o bd26_beta

clean:
	rm -f bd26
	rm -f /usr/bin/bd26
