# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Linux g++, and Mac OS X

EXE := build/QuakePrism
IMGUI_DIR := ./lib/imgui
SRC_DIR := ./src
BUILD_DIR := ./build
RES_DIR := $(BUILD_DIR)/.res
SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_sdl2.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
OBJS := $(addprefix $(BUILD_DIR)/, $(notdir $(SOURCES:.cpp=.o)))
UNAME_S := $(shell uname -s)
LINUX_GL_LIBS := -lGL -lGLU -lGLEW

CXX ?= g++  # Default compiler
CXXFLAGS := -std=c++17 -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I$(SRC_DIR)  -g -Wall -Wformat
LIBS :=

ifeq ($(UNAME_S), Linux)
    ECHO_MESSAGE := "Linux"
    LIBS += $(LINUX_GL_LIBS) -ldl -lSDL2
    CXXFLAGS += `sdl2-config --cflags`
endif

ifeq ($(UNAME_S), Darwin)
    ECHO_MESSAGE := "Mac OS X"
    LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo `sdl2-config --libs`
    LIBS += -L/usr/local/lib -L/opt/local/lib
    CXXFLAGS += `sdl2-config --cflags`
    CXXFLAGS += -I/usr/local/include -I/opt/local/include
endif

ifeq ($(OS), Windows_NT)
    ECHO_MESSAGE := "MinGW"
    LIBS += -lgdi32 -lopengl32 -limm32 `pkg-config --static --libs sdl2`
    CXXFLAGS += `pkg-config --cflags sdl2`
endif

# Cross-compilation settings for MinGW on Linux
ifeq ($(UNAME_S), Linux)
    ifeq ($(CROSS_COMPILE),1)
        CXX := x86_64-w64-mingw32-g++
        EXE := build/QuakePrism.exe
        LIBS := -lmingw32 -lSDL2main -lSDL2 -lgdi32 -lopengl32 -limm32 -lglew32 -lglu32
        CXXFLAGS := -std=c++17 -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I$(SRC_DIR) -I/usr/x86_64-w64-mingw32/include -I/usr/x86_64-w64-mingw32/include/SDL2 -D_UNICODE -DUNICODE -g -Wall -Wformat
    endif
endif

## BUILD RULES

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o: $(IMGUI_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o: $(IMGUI_DIR)/backends/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o: $(TGA_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE) copy_resources
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

copy_resources:
	@echo "Copying resources directory..."
	@cp -r $(SRC_DIR)/.res $(BUILD_DIR)
ifeq ($(CROSS_COMPILE),1)
	@cp -a $(SRC_DIR)/.dlls/* $(BUILD_DIR)/
endif
	@cp -r imgui.ini $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) $(EXE)

