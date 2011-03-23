CC = g++
DEPS = 
C_OBJ=xmt_book.o ebp_client.o local_book_manager.o xmt_zlib.o ioapi.o unzip.o local_book_manager.o xmt_engine.o xmt_util.o
S_OBJ=ebp_server.o xmt_book_stock_manager.o
CFLAGS=`pkg-config --cflags gtk+-2.0`

all:ebook_reader ebook_server

ebook_server:$(S_OBJ)
	g++ -g -o $@ server.c++ $^
ebook_reader: $(C_OBJ)
	g++ -g -o $@ client.c++ $^ $(CFLAGS) -lpthread -lz `pkg-config --libs gtk+-2.0` -lsqlite3

%.o: %.c++ $(DEPS)
	g++ -g -c -o $@ $< $(CFLAGS)

clean:
	rm *.o
