TARGET = sgs

SOURCES := $(wildcard src/*.c)
OBJECTS := $(SOURCES:%.c=%.o)

CFLAGS = -g -DDEBUG
LDLIBS = -lssl -lcrypto -lsqlite3

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDLIBS)

$(OBJECTS): %.o: %.c

clean:
	@rm $(TARGET) $(OBJECTS)