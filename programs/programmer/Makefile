OUTPUT := bin/prog_otter
SOURCES := src/prog_otter.c

.PHONY: all
all: $(OUTPUT) ;

$(OUTPUT): $(SOURCES)
	$(CC) $^ -o $@

.PHONY: clean
clean:
	$(RM) bin/prog_otter
