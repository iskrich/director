MODULE := engines/director

MODULE_OBJS = \
	detection.o \
	dib.o \
	director.o \
	movie.o \
	resource.o \
	score.o \
	sound.o \
	lingo/lingo-gr.o \
	lingo/lingo.o \
	lingo/lingo-builtins.o \
	lingo/lingo-code.o \
	lingo/lingo-codegen.o \
	lingo/lingo-funcs.o \
	lingo/lingo-lex.o \
	lingo/lingo-the.o

engines/director/lingo/lingo-lex.cpp: engines/director/lingo/lingo-lex.l
	flex -o engines/director/lingo/lingo-lex.cpp engines/director/lingo/lingo-lex.l

engines/director/lingo/lingo-gr.cpp: engines/director/lingo/lingo-gr.y
	bison -dv -o engines/director/lingo/lingo-gr.cpp engines/director/lingo/lingo-gr.y
	mv engines/director/lingo/lingo-gr.hpp engines/director/lingo/lingo-gr.h

# This module can be built as a plugin
ifeq ($(ENABLE_DIRECTOR), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk
