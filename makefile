OS := $(shell uname -s)

# Config OS-Specific Variables
ifeq ($(OS), Linux)
	CC=gcc
    LIBS=-lGL -lglfw -llua
	$(warning Untested OS)
else ifeq ($(OS), Darwin)
	CC=clang
    LIBS=-framework OpenGL -lglfw -llua
else ifeq ($(OS), Windows_NT)
	CC=mingw
    LIBS=-lopengl -lglfw -llua
	$(warning Untested OS)
else
    $(error Unsupported OS)
endif

SOURCES=main.c src/render.c

FLAGS=-L/opt/homebrew/lib -I/opt/homebrew/include -Iinclude

build/sim: $(SOURCES) include/*
	@mkdir -p build
	@$(CC) $(SOURCES) $(FLAGS) $(LIBS) -o build/sim
	@echo "Built Simulator"

clean:
	@rm -r build
	@echo "Clean Finished"

.PHONY: clean
