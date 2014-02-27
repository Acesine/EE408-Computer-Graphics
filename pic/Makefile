# Please make sure you have the jpeg 
# library (sudo apt-get install libjpeg62-dev) 
# before compiling this 

CC = gcc

OBJS = pic.o xpic.o ppm.o adaptcm.o jpeg.o

LIB = libpicio.a

$(LIB): $(OBJS)
	ar cr $(LIB) $(OBJS)

clean:
	/bin/rm -f $(LIB) $(OBJS)
