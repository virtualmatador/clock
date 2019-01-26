LIBS := -lSDL2main -lSDL2 -lSDL2_ttf
DEBUG := $(if $(shell git symbolic-ref --short HEAD | grep master), , -g)
SOURCES := $(wildcard src/*.cpp)

BIN_LINUX := bin_linux
TARGET_LINUX := $(BIN_LINUX)/clock
BUILD_LINUX := build_linux
OBJECTS_LINUX := $(patsubst src/%.cpp, $(BUILD_LINUX)/%.o, $(SOURCES))
CC_LINUX := g++
LIBS_LINUX := $(LIBS)

BIN_WINDOWS := bin_windows
TARGET_WINDOWS := $(BIN_WINDOWS)/Clock.exe
BUILD_WINDOWS := build_windows
OBJECTS_WINDOWS := $(patsubst src/%.cpp, $(BUILD_WINDOWS)/%.o, $(SOURCES))
CC_WINDOWS := x86_64-w64-mingw32-g++
LIBS_WINDOWS := -lmingw32 -static-libstdc++ -static-libgcc $(LIBS)

.PHONY: clean

all: $(TARGET_LINUX) $(TARGET_WINDOWS)

define PROGRAM_template
$(1): $(2) $(SHADERS)
	mkdir -p $(3)
	$(4) -o $$@ $(2) $(5) -no-pie -Wl,--format=binary -Wl,res/Carlito-Bold.ttf -Wl,--format=default
endef

$(eval $(call PROGRAM_template, $(TARGET_LINUX), $(OBJECTS_LINUX), $(BIN_LINUX), $(CC_LINUX), $(LIBS_LINUX)))
$(eval $(call PROGRAM_template, $(TARGET_WINDOWS), $(OBJECTS_WINDOWS), $(BIN_WINDOWS), $(CC_WINDOWS), $(LIBS_WINDOWS)))

define OBJECT_RULE
$(1)/$(subst $() \,,$(shell $(3) -MM $(2)))
	mkdir -p $(1)/
	$(3) $(DEBUG) -c -o $$@ $$<
endef
$(foreach src, $(SOURCES), $(eval $(call OBJECT_RULE, $(BUILD_LINUX),$(src), $(CC_LINUX))))
$(foreach src, $(SOURCES), $(eval $(call OBJECT_RULE, $(BUILD_WINDOWS),$(src), $(CC_WINDOWS))))

clean:
	$(RM) -r $(BUILD_LINUX)/
	$(RM) -r $(BIN_LINUX)/
	$(RM) -r $(BUILD_WINDOWS)/
	$(RM) -r $(BIN_WINDOWS)/
