# config
SOURCES := main.c src/*

# auto
OS := $(shell uname -s)
LUA_DEP := $(patsubst api_repo/tactics/%.lua,build/%.lua,$(wildcard api_repo/tactics/*.lua))

# detect os
ifeq ($(OS), Linux)
    CC=gcc
    LIBS=-lm -ldl -lGL -lglfw -llua
    FLAGS=-L/usr/lib -I/usr/include -Iinclude
else ifeq ($(OS), Darwin)
    CC=clang
    LIBS=-framework OpenGL -lglfw -llua
    FLAGS=-L/opt/homebrew/lib -I/opt/homebrew/include -Iinclude
else
    $(error Unsupported OS)
endif

# main
build/sim: $(SOURCES) include/* $(LUA_DEP)
	@mkdir -p $(dir $@)
	$(CC) $(SOURCES) $(FLAGS) $(LIBS) -o build/sim
	cp ./api_repo/tactics/* ./build
	@echo "Built Simulator"

# copy lua files
build/%.lua: api_repo/tactics/%.lua
	@mkdir -p $(dir $@)
	cp $< $@

# run target
run: build/sim
	cd build && ./sim

# misc
clean:
	rm -r build
	@echo "Clean Finished"
	
.PHONY: clean
