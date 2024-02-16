CFLAGS= -Wall -Werror -std=c99

gauss : gauss.c
	gcc $^ ${CFLAGS} -lpthread -o $@

diekert : diekert.c
	gcc $^ ${CFLAGS} -o $@

generate : gauss in.txt
	./$< > matrix_out.txt

draw : diekert
	./$< $(size) > draw.dot

clean :
	-rm diekert{,.png,.dot} gauss matrix_out.txt
