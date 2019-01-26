TARGET := clock
LIBS := -lSDL2main -lSDL2 -lSDL2_ttf

DEBUG := $(if $(shell git symbolic-ref --short HEAD | grep master), , -g)
SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(patsubst src/%.cpp, build/%.o, $(SOURCES))

CC := g++

.PHONY: clean, install, uninstall

all: build

Windows: TARGET := clock.exe
Windows: CC := x86_64-w64-mingw32-g++ -lmingw32 -static-libstdc++ -static-libgcc
Windows: build

build: $(OBJECTS) $(SHADERS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LIBS) -no-pie -Wl,--format=binary -Wl,res/Carlito-Bold.ttf -Wl,--format=default

clean:
	$(RM) -r build/
	$(RM) -r $(TARGET)

install:
	sudo cp $(TARGET) /usr/local/sbin/

uninstall:
	sudo $(RM) /usr/local/sbin/$(TARGET)

define OBJECT_RULE
build/$(subst $() \,,$(shell $(CC) -MM $(1)))
	mkdir -p build/
	$$(CC) $(DEBUG) -c -o $$@ $$<
endef
$(foreach src, $(SOURCES), $(eval $(call OBJECT_RULE,$(src))))
