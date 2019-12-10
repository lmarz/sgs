TARGET = sgs
OBJECTS := sgs.o

all: $(TARGET)

$(TARGET): $(OBJECTS)

$(OBJECTS): %.o: %.c

clean:
	@rm $(TARGET) $(OBJECTS)