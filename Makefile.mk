# ========================================
# Target name

TARGET = ntp_cli_test.exe

#========================================
# Compiler

CC = cl
LN = link

#========================================
# Options

CXXFLAGS = /nologo /EHsc /c
CXXMACRO = /DNTP_OUTPUT=1 /D_WIN32 /DWIN32 /DNDEBUG /D_CONSOLE /D_UNICODE /DUNICODE
LDFLAGS  = ws2_32.lib kernel32.lib /subsystem:console

#========================================
# Modules

CORE_SRC = *.cpp
CORE_OBJ = *.obj

SRCS = $(CORE_SRC)
OBJS = $(CORE_OBJ)

#========================================
# Build

$(TARGET) : $(OBJS)
	$(LN) $(LDFLAGS) $(OBJS) /out:$(TARGET)

.cpp.obj ::
	$(CC) $(CXXMACRO) $(CXXFLAGS) $<

#========================================
# Cleanup

clean:
	@del /f /s /q $(OBJS) $(TARGET)

#========================================

.PHONY:clean
