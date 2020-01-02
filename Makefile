TARGET = sgs
OBJECTS := sgs.o
OBJECTS += config.o
OBJECTS += request_parser.o

all: $(TARGET)

$(TARGET): $(OBJECTS)

$(OBJECTS): %.o: %.c

clean:
	@rm $(TARGET) $(OBJECTS)