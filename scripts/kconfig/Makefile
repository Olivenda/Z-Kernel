HOSTCC ?= cc
CFLAGS ?= -Wall -Wextra -g

all: conf mconf

conf: conf.c kconfig_common.c kconfig_common.h
	$(HOSTCC) $(CFLAGS) -o $@ conf.c kconfig_common.c

mconf: mconf.c kconfig_common.c kconfig_common.h
	$(HOSTCC) $(CFLAGS) -o $@ mconf.c kconfig_common.c -lncurses

clean:
	rm -f conf mconf
