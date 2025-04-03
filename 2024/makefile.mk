TARGETS := main

include ~/maker/variables.mk

OPTIONS := $(filter-out -static,$(OPTIONS)) -mwindows -lsfml-graphics -lsfml-window -lsfml-system

include ~/maker/rules.mk
