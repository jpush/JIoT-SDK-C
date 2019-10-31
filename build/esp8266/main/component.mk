#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

DEMO_SRC := ../../..
COMPONENT_ADD_INCLUDEDIRS += $(DEMO_SRC)/examples/esp8266/main
COMPONENT_SRCDIRS := $(DEMO_SRC)/examples/esp8266/main