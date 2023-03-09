objects = server.o getTimestamp.o processInput.o sendErrorResponse.o
all: $(objects)

$(objects): %.o: %.c
	cc -c $< -o $@

server: $(objects)
	cc -lcurl -o server $(objects)

debug: $(objects)
	cc -c $< -o $@

clean:
	rm -f *.o
	rm -f server
