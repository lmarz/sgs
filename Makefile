TARGET = sgs

SOURCES := $(wildcard *.c)
OBJECTS := $(SOURCES:%.c=%.o)

CFLAGS = -g -DDEBUG

all: $(TARGET)

$(TARGET): $(OBJECTS)

$(OBJECTS): %.o: %.c

clean:
	@rm $(TARGET) $(OBJECTS)