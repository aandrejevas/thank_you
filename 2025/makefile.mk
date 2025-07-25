TARGETS := main

include ~/maker/variables.mk

OPTIONS := $(OPTIONS) -mwindows -lSDL3 -lImm32 -lole32 -lWinmm -lSetupapi -lVersion -lOleAut32 -luuid -lSDL3_ttf -lfreetype -lharfbuzz -lpng -lgraphite2 -lz -lRpcrt4 -lDwrite -lbz2 -lbrotlidec -lbrotlicommon

include ~/maker/rules.mk
