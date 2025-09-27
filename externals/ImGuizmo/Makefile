# ImGuizmo Static Library Makefile for MSVC
# Usage: nmake [debug|release|all|clean]

# Compiler and tools
CC = cl
LIB = lib

# Common flags
CPPFLAGS = /I. /Iexample /nologo
CXXFLAGS_COMMON = /std:c++17 /EHsc

# Debug configuration
CXXFLAGS_DEBUG = $(CXXFLAGS_COMMON) /Od /Zi /D_DEBUG /MDd
DEBUG_DIR = debug
DEBUG_LIB = ImGuizmoDebug.lib

# Release configuration  
CXXFLAGS_RELEASE = $(CXXFLAGS_COMMON) /O2 /DNDEBUG /MD
RELEASE_DIR = release
RELEASE_LIB = ImGuizmo.lib

# Object files
DEBUG_OBJS = \
    $(DEBUG_DIR)\ImGuizmo.obj \
    $(DEBUG_DIR)\GraphEditor.obj \
    $(DEBUG_DIR)\ImCurveEdit.obj \
    $(DEBUG_DIR)\ImGradient.obj \
    $(DEBUG_DIR)\ImSequencer.obj

RELEASE_OBJS = \
    $(RELEASE_DIR)\ImGuizmo.obj \
    $(RELEASE_DIR)\GraphEditor.obj \
    $(RELEASE_DIR)\ImCurveEdit.obj \
    $(RELEASE_DIR)\ImGradient.obj \
    $(RELEASE_DIR)\ImSequencer.obj

# Default target
all: debug-lib release-lib

# Debug library
debug-lib: $(DEBUG_DIR) $(DEBUG_LIB)

$(DEBUG_LIB): $(DEBUG_OBJS)
    $(LIB) /OUT:$@ /NOLOGO $**

# Release library
release-lib: $(RELEASE_DIR) $(RELEASE_LIB)

$(RELEASE_LIB): $(RELEASE_OBJS)
    $(LIB) /OUT:$@ /NOLOGO $**

# Debug object compilation rules
$(DEBUG_DIR)\ImGuizmo.obj: ImGuizmo.cpp
    $(CC) $(CPPFLAGS) $(CXXFLAGS_DEBUG) /c ImGuizmo.cpp /Fo$@

$(DEBUG_DIR)\GraphEditor.obj: GraphEditor.cpp
    $(CC) $(CPPFLAGS) $(CXXFLAGS_DEBUG) /c GraphEditor.cpp /Fo$@

$(DEBUG_DIR)\ImCurveEdit.obj: ImCurveEdit.cpp
    $(CC) $(CPPFLAGS) $(CXXFLAGS_DEBUG) /c ImCurveEdit.cpp /Fo$@

$(DEBUG_DIR)\ImGradient.obj: ImGradient.cpp
    $(CC) $(CPPFLAGS) $(CXXFLAGS_DEBUG) /c ImGradient.cpp /Fo$@

$(DEBUG_DIR)\ImSequencer.obj: ImSequencer.cpp
    $(CC) $(CPPFLAGS) $(CXXFLAGS_DEBUG) /c ImSequencer.cpp /Fo$@

# Release object compilation rules
$(RELEASE_DIR)\ImGuizmo.obj: ImGuizmo.cpp
    $(CC) $(CPPFLAGS) $(CXXFLAGS_RELEASE) /c ImGuizmo.cpp /Fo$@

$(RELEASE_DIR)\GraphEditor.obj: GraphEditor.cpp
    $(CC) $(CPPFLAGS) $(CXXFLAGS_RELEASE) /c GraphEditor.cpp /Fo$@

$(RELEASE_DIR)\ImCurveEdit.obj: ImCurveEdit.cpp
    $(CC) $(CPPFLAGS) $(CXXFLAGS_RELEASE) /c ImCurveEdit.cpp /Fo$@

$(RELEASE_DIR)\ImGradient.obj: ImGradient.cpp
    $(CC) $(CPPFLAGS) $(CXXFLAGS_RELEASE) /c ImGradient.cpp /Fo$@

$(RELEASE_DIR)\ImSequencer.obj: ImSequencer.cpp
    $(CC) $(CPPFLAGS) $(CXXFLAGS_RELEASE) /c ImSequencer.cpp /Fo$@

# Create directories
$(DEBUG_DIR):
    if not exist $(DEBUG_DIR) mkdir $(DEBUG_DIR)

$(RELEASE_DIR):
    if not exist $(RELEASE_DIR) mkdir $(RELEASE_DIR)

# Clean target
clean:
    if exist $(DEBUG_DIR) rmdir /s /q $(DEBUG_DIR)
    if exist $(RELEASE_DIR) rmdir /s /q $(RELEASE_DIR)
    if exist $(DEBUG_LIB) del $(DEBUG_LIB)
    if exist $(RELEASE_LIB) del $(RELEASE_LIB)
