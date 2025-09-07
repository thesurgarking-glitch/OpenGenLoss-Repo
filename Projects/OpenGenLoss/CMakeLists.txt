cmake_minimum_required(VERSION 3.20)
project(OpenGenLoss)

# Point to iPlug2 (as submodule or sibling). You can also pass -DIPLUG2_ROOT on the command line.
if(NOT DEFINED IPLUG2_ROOT)
  set(IPLUG2_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../iPlug2" CACHE PATH "Path to iPlug2 root")
endif()

# iPlug2 helper
include(${IPLUG2_ROOT}/CMake/functions.cmake)

# Basic plug-in settings
set(PLUG_NAME "OpenGenLoss")
set(PLUG_MFR "OpenDSP")
set(PLUG_UNIQUE_ID "OGL1")
set(PLUG_VERSION_HEX "0x00010000")
set(PLUG_VERSION_STR "1.0.0")

# Targets to build: set on CLI with -DIPLUG2_TARGETS="VST3;AUv2"
if(NOT IPLUG2_TARGETS)
  set(IPLUG2_TARGETS "VST3;AUv2")
endif()

set(SRC
  src/IPlugGenLoss.cpp
  src/GenLossDSP.cpp
)

set(HEADERS
  src/IPlugGenLoss.h
  src/GenLossDSP.h
)

# Resources (optional images/fonts in resources/img)
set(RESOURCES
  resources/config.h
)

iplug2_add_plugin(OpenGenLoss
  NAME ${PLUG_NAME}
  MFR ${PLUG_MFR}
  VERSION_HEX ${PLUG_VERSION_HEX}
  VERSION_STR ${PLUG_VERSION_STR}
  BUNDLE_NAME ${PLUG_NAME}
  BUNDLE_MFR ${PLUG_MFR}
  CHANNEL_IO "2-2"
  SRC ${SRC}
  HEADERS ${HEADERS}
  RESOURCES ${RESOURCES}
  GRAPHICS true
  PLUGINS ${IPLUG2_TARGETS}
)

# On macOS, enable hardened runtime if signing later (optional)
if(APPLE)
  target_compile_features(OpenGenLoss PUBLIC cxx_std_17)
endif()
