cmake_minimum_required(VERSION 3.24)
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(anno2070_modloader VERSION 0.1.0 LANGUAGES C CXX)

if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Use x64 MSVC or Clang targeting x86_64-pc-windows-msvc")
endif()

include(FetchContent)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build dependencies statically" FORCE)
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "Build spdlog statically" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(PUGIXML_BUILD_TESTS OFF CACHE BOOL "" FORCE)
# main mirrors upstream; anno2070 has independent history. Pin main's revision.
FetchContent_Declare(xmlcore
  URL https://github.com/lukts30/anno2070-mod-loader/archive/5a4464dc2da222844f439ac0463174d2da97b087.tar.gz
  URL_HASH SHA256=c9a165c5d921d0f95d8790b00d6a9bc9516af1aff1a3c1d59a7b51f571af8313
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(pugixml
  URL https://github.com/zeux/pugixml/archive/ee86beb30e4973f5feffe3ce63bfa4fbadf72f38.tar.gz
  URL_HASH SHA256=51c102d4187fac99daa38af281b0772c5e6c586f65004cdc63f8f2e011a21492
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(spdlog
  URL https://github.com/gabime/spdlog/archive/76fb40d95455f249bd70824ecfcae7a8f0930fa3.tar.gz
  URL_HASH SHA256=4530d53b86d45228a5b8982a8f4fe7d95d3ba8e1944858d3f663732c7e949cb7
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(minhook
  URL https://github.com/TsudaKageyu/minhook/archive/c3fcafdc10146beb5919319d0683e44e3c30d537.tar.gz
  URL_HASH SHA256=31757833475be039c16359d4e276e718db6ba2ba61a6b870053a112106015822
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(xmlcore pugixml spdlog minhook)

file(GLOB core_sources CONFIGURE_DEPENDS "${xmlcore_SOURCE_DIR}/libs/xml-operations/src/*.cc")
add_library(version SHARED src/loader.cpp src/proxy.cpp src/version.def ${core_sources})
target_compile_features(version PRIVATE cxx_std_17)
target_compile_definitions(version PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
target_compile_options(version PRIVATE /EHsc /utf-8 /FI${CMAKE_CURRENT_SOURCE_DIR}/cmake/xmlcore-compat.h)
target_include_directories(version PRIVATE "${xmlcore_SOURCE_DIR}/libs/xml-operations/include")
target_link_libraries(version PRIVATE pugixml spdlog::spdlog_header_only minhook user32)
set_target_properties(version PROPERTIES PREFIX "")

include(CTest)
if(BUILD_TESTING)
  add_executable(proxy-smoke tests/proxy-smoke.cpp)
  target_compile_features(proxy-smoke PRIVATE cxx_std_17)
  target_compile_definitions(proxy-smoke PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
  target_compile_options(proxy-smoke PRIVATE /EHsc)
  add_test(NAME proxy-smoke COMMAND proxy-smoke $<TARGET_FILE:version>)
endif()

install(TARGETS version RUNTIME DESTINATION . COMPONENT Runtime)
install(FILES README.md LICENSE DESTINATION . COMPONENT Runtime)
install(FILES "${xmlcore_SOURCE_DIR}/LICENSE" DESTINATION licenses RENAME xmlcore.txt COMPONENT Runtime)
install(FILES "${pugixml_SOURCE_DIR}/LICENSE.md" DESTINATION licenses RENAME pugixml.txt COMPONENT Runtime)
install(FILES "${spdlog_SOURCE_DIR}/LICENSE" DESTINATION licenses RENAME spdlog.txt COMPONENT Runtime)
install(FILES "${minhook_SOURCE_DIR}/LICENSE.txt" DESTINATION licenses RENAME minhook.txt COMPONENT Runtime)

set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_FILE_NAME "anno2070-mod-loader-${PROJECT_VERSION}-windows-x64")
set(CPACK_COMPONENTS_ALL Runtime)
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)
include(CPack)
