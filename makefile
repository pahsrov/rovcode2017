CC = g++

SOURCES = $(wildcard src/**/*.cpp src/*.cpp src/**/*.c src/*.c)

rov: $(SOURCES)
	$(CC) -o rov $(SOURCES)
