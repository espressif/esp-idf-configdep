VERSION = 0.1.1
NAME := esp-idf-configdep

O ?= build
DIST ?= dist
STAGE ?= stage
CFLAGS ?= -Wall -Werror -std=c99 -pedantic
LDFLAGS ?=

T ?= t/
PFLAGS ?=
T_OUT ?= t_out

CROSS_COMPILE := $(subst $(lastword $(subst -, ,$(CC))),,$(CC))
WINDRES := $(CROSS_COMPILE)windres

BUILD_CFLAGS = $(CFLAGS) $(CFLAGS_APPEND)
BUILD_LDFLAGS = $(LDFLAGS) $(LDFLAGS_APPEND)
BUILD_DEFINES := -DVERSION=\"$(VERSION)\"

COMPILER_VERSION := $(shell $(CC) --version)
CONTEXT := "$(COMPILER_VERSION) $(CFLAGS) $(LDFLAGS)"

DUMPMACHINE := $(shell $(CC) -dumpmachine)

ifneq ($(findstring x86_64,$(DUMPMACHINE)),)
	ARCH := x86_64
else ifneq ($(findstring i686,$(DUMPMACHINE)),)
	ARCH := x86
else ifneq ($(findstring aarch64,$(DUMPMACHINE)),)
	ARCH := arm64
else ifneq ($(findstring arm64,$(DUMPMACHINE)),)
	ARCH := arm64
else ifneq ($(findstring arm,$(DUMPMACHINE)),)
	# Check for Hard Float vs Soft Float
	ifneq ($(findstring gnueabihf,$(DUMPMACHINE)),)
		ARCH := armhf
	else
		ARCH := armel
	endif
endif

ifneq ($(findstring mingw,$(DUMPMACHINE)),)
	S ?= win
else ifneq ($(findstring windows,$(DUMPMACHINE)),)
	S ?= win
else ifneq ($(findstring apple,$(DUMPMACHINE)),)
	S ?= macos
else
	S ?= linux
endif

PKG_NAME := $(NAME)-$(VERSION)-$(S)-$(ARCH)$(PKG_SUFFIX)

SRCS := configdep.c \
	port.c \
	utils.c \
	membuf.c

ifeq ($(S), win)

SRCS += win.c
CFLAGS += -municode
LDFLAGS += -municode
BINARY := $(O)/$(NAME).exe

else

SRCS += posix.c
BINARY := $(O)/$(NAME)

endif

OBJS := $(SRCS:%.c=$(O)/%.o)
ifeq ($(S), win)
	OBJS += $(O)/manifest.o
endif

all: $(BINARY)

define MANIFEST_XML
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
  <assemblyIdentity
    type="win32"
    name="$(NAME)"
    version="$(VERSION).0"
    processorArchitecture="*"
  />
  <application xmlns="urn:schemas-microsoft-com:asm.v3">
    <windowsSettings xmlns:ws2="http://schemas.microsoft.com/SMI/2016/WindowsSettings">
      <ws2:longPathAware>true</ws2:longPathAware>
    </windowsSettings>
  </application>
</assembly>
endef

$(O)/manifest.xml: Makefile | $(O)
	$(file >$@,$(MANIFEST_XML))

$(O)/manifest.rc: $(O)/manifest.xml
	$(file >$@,1 24 "$<")

$(O)/manifest.o: $(O)/manifest.rc $(O)/context
	$(WINDRES) --input=$< --output=$@

$(O):
	mkdir -p $@

$(DIST):
	mkdir -p $@

$(O)/context: FORCE | $(O)
	@echo $(CONTEXT) | md5sum | cmp -s - $@ || echo $(CONTEXT) | md5sum > $@

$(O)/%.o: %.c Makefile $(O)/context | $(O)
	$(CC) $(BUILD_DEFINES) $(BUILD_CFLAGS) -MD -MP -o $@ -c $<

$(BINARY): $(OBJS) | $(O)
	$(CC) $(BUILD_LDFLAGS) -o $@ $^

.PHONY: clean \
	targz-pkg \
	tarxz-pkg \
	zip-pkg \
	clang-format \
	clang-tidy \
	test \
	cscope \
	ctags \
	tags \
	help

FORCE:

targz-pkg: $(DIST)/$(PKG_NAME).tar.gz
tarxz-pkg: $(DIST)/$(PKG_NAME).tar.xz
zip-pkg: $(DIST)/$(PKG_NAME).zip

$(STAGE): $(BINARY)
	@rm -rf $@
	@install -d $@/$(NAME)-$(VERSION)/bin
	@install $(BINARY) $(STAGE)/$(NAME)-$(VERSION)/bin
	@touch $@

$(DIST)/$(PKG_NAME).tar.gz: $(STAGE) | $(DIST)
	cd $(STAGE) && tar -cvzf $(abspath $@) $(NAME)-$(VERSION)

$(DIST)/$(PKG_NAME).tar.xz: $(STAGE) | $(DIST)
	cd $(STAGE) && tar -cvJf $(abspath $@) $(NAME)-$(VERSION)

$(DIST)/$(PKG_NAME).zip: $(STAGE) | $(DIST)
	cd $(STAGE) && zip -r $(abspath $@) $(NAME)-$(VERSION)

clean:
	rm -rf $(O) $(DIST) $(STAGE) $(T_OUT)

clang-format:
	clang-format --style=file -i *.[ch]

clang-tidy:
	clang-tidy \
		--extra-arg="--target=$(DUMPMACHINE)" \
		$(SRCS) \
		-- \
		$(BUILD_DEFINES) \
		$(BUILD_CFLAGS)

BINARY_ABS := $(abspath $(BINARY))
test: $(BINARY)
	T_OUT=$(T_OUT) BINARY=$(BINARY_ABS) CC=${CC} prove $(PFLAGS) $(T)


cscope:
	find . \( -name "*.c" -o -name "*.h" \) >cscope.files
	cscope -b

ctags:
	find . \( -name "*.c" -o -name "*.h" \) | ctags -L -

tags: cscope ctags

help:
	@echo 'build variables:'
	@echo ' CC               - compiler to use e.g. gcc-i686-linux-gnu or x86_64-w64-mingw32-gcc (default: $(CC))'
	@echo ' O                - output build directory (default: $(O))'
	@echo ' S                - operating system win/posix/macos (default: autodetected)'
	@echo ' CFLAGS           - compiler options (default: $(CFLAGS))'
	@echo ' LDFLAGS          - linker options (default: $(LDFLAGS))'
	@echo ' CFLAGS_APPEND    - additional compiler options to append after CFLAGS'
	@echo ' LDFLAGS_APPEND   - additional linker options to append after LDFLAGS'
	@echo ''
	@echo 'test variables:'
	@echo ' T                - directory with tests using TAP(default: $(T))'
	@echo ' T_OUT            - directory with test artifacts (default: $(T_OUT))'
	@echo ' PFLAGS           - prove options for running tests through TAP harness  (default: $(PFLAGS))'
	@echo ''
	@echo 'test targets:'
	@echo ' test             - run all tests  (default: $(PFLAGS))'
	@echo ''
	@echo 'distribution variables:'
	@echo ' DIST             - output directory for distribution files (default: $(DIST))'
	@echo ' STAGE            - stagging directory for distribution preparation (default: $(STAGE))'
	@echo ''
	@echo 'distribution targets:'
	@echo ' targz-pkg        - create tar.gz archive for distribution'
	@echo ' tarxz-pkg        - create tar.xz archive for distribution'
	@echo ' zip-pkg          - create zip archive for distribution'
	@echo ''
	@echo 'lint targets:'
	@echo ' clang-format     - run clang formatter'
	@echo ' clang-tidy       - run clang tidy'
	@echo ''
	@echo 'misc targets:'
	@echo ' clean            - remove: $(O) $(DIST) $(STAGE) $(T_OUT)'
	@echo ' cscope           - generate cscope tags'
	@echo ' ctags            - generate ctags'
	@echo ' tags             - alias for cscope and tags'

-include $(O)/*.d
