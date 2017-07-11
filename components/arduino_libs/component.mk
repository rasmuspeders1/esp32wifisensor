ARDUINO_LIBS := $(patsubst $(COMPONENT_PATH)/%,%,$(wildcard $(COMPONENT_PATH)/*/src)) $(patsubst $(COMPONENT_PATH)/%,%,$(filter-out $(COMPONENT_PATH)/component.mk, $(wildcard $(COMPONENT_PATH)/*/)))

COMPONENT_ADD_INCLUDEDIRS := $(ARDUINO_LIBS)
COMPONENT_SRCDIRS := $(ARDUINO_LIBS)
CXXFLAGS += -fno-rtti
