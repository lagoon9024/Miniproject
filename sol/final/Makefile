OBJECTS=client.o server.o
SRCS=$(OBJECTS: .o=.c)

GFLAGS=-W
TARGET=project
LIBS=-lpthread -lmysqlclient


$(TARGET) : $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LIBS)

clean:
	rm $(OBJECTS) $(TARGET)

$(SRCS):
	$(CC) $(CFLAGS) $(LIBS) -c $*.c

all:$(TARGET)
