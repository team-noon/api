OS := $(shell uname -s)

ifeq ($(OS), Linux)
    CC=gcc
    LIBS=-lm -ldl -lGL -lglfw -llua
    FLAGS=-L/usr/lib -I/usr/include -Iinclude
    $(warning Untested OS)
else ifeq ($(OS), Darwin)
    CC=clang
    LIBS=-framework OpenGL -lglfw -llua
    FLAGS=-L/opt/homebrew/lib -I/opt/homebrew/include -Iinclude
else
    $(error Unsupported OS)
endif

SOURCES=main.c src/*

build/sim: $(SOURCES) include/*
	mkdir -p build
	$(CC) $(SOURCES) $(FLAGS) $(LIBS) -o build/sim
	cp ./lua/* ./build
	echo "Built Simulator"

clean:
	rm -r build
	echo "Clean Finished"
	
.PHONY: clean
