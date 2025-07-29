TARGETS := main

include ~/maker/variables.mk

OPTIONS := $(OPTIONS) -mwindows -lSDL3 -lSDL3_ttf -lstdc++exp

include ~/maker/rules.mk
