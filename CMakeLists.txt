# Check ESP-IDF version and error out if it is not in the supported range.
#
# Note for arduino-esp32 developers: to bypass the version check locally,
# set ARDUINO_SKIP_IDF_VERSION_CHECK environment variable to 1. For example:
#   export ARDUINO_SKIP_IDF_VERSION_CHECK=1
#   idf.py build

set(min_supported_idf_version "4.4.0")
set(max_supported_idf_version "4.4.99")
set(idf_version "${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}.${IDF_VERSION_PATCH}")

set(CORE_SRCS
  src/ThingerClient.h
  src/ThingerClient.cpp
  )

set(srcs ${CORE_SRCS})

idf_component_register(SRCS ${srcs} INCLUDE_DIRS "src/." REQUIRES "arduino")
