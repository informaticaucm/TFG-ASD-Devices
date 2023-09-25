COMPONENT_ADD_INCLUDEDIRS := esp32-camera/driver/include esp32-camera/conversions/include
COMPONENT_PRIV_INCLUDEDIRS := esp32-camera/driver/private_include esp32-camera/conversions/private_include esp32-camera/sensors/private_include esp32-camera/target/private_include
COMPONENT_SRCDIRS := esp32-camera/driver esp32-camera/conversions esp32-camera/sensors esp32-camera/target esp32-camera/target/esp32
CXXFLAGS += -fno-rtti
