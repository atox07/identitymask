cmake_minimum_required(VERSION 3.20)
project(IdentityMask VERSION 0.3.0 LANGUAGES CXX)
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Build with MSVC x64 on Windows")
endif()
add_library(IdentityMask SHARED src/main.cpp)
# Explicit loader dependency: GTA's ASI loader must load ScriptHookV.dll before
# DllMain, even when no other installed ASI has imported ScriptHookV yet.
set(SHV_IMPORT "${CMAKE_CURRENT_BINARY_DIR}/ScriptHookV.lib")
add_custom_command(OUTPUT "${SHV_IMPORT}"
  COMMAND "${CMAKE_AR}" /nologo /machine:x64
    /def:${CMAKE_CURRENT_SOURCE_DIR}/src/ScriptHookV.def /out:${SHV_IMPORT}
  DEPENDS src/ScriptHookV.def VERBATIM)
add_custom_target(shv_import DEPENDS "${SHV_IMPORT}")
add_dependencies(IdentityMask shv_import)
target_link_libraries(IdentityMask PRIVATE "${SHV_IMPORT}")
target_compile_features(IdentityMask PRIVATE cxx_std_17)
target_compile_definitions(IdentityMask PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(IdentityMask PRIVATE /W4 /permissive- /EHsc)
set_property(TARGET IdentityMask PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_target_properties(IdentityMask PROPERTIES PREFIX "" SUFFIX ".asi")
