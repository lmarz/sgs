TARGET = sgs

SOURCES := $(wildcard src/*.c)
OBJECTS := $(SOURCES:%.c=%.o)

CFLAGS = -g -DDEBUG

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

$(OBJECTS): %.o: %.c

clean:
	@rm $(TARGET) $(OBJECTS)