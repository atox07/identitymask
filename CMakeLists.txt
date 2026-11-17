cmake_minimum_required(VERSION 3.20)
project(IdentityMask VERSION 0.4.0 LANGUAGES CXX)
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Build with MSVC x64 on Windows")
endif()
# Explicit loader dependency: GTA's ASI loader must load ScriptHookV.dll before
# DllMain, even when no other installed ASI has imported ScriptHookV yet.
set(SHV_IMPORT "${CMAKE_CURRENT_BINARY_DIR}/ScriptHookV.lib")
add_custom_command(OUTPUT "${SHV_IMPORT}"
  COMMAND "${CMAKE_AR}" /nologo /machine:x64
    /def:${CMAKE_CURRENT_SOURCE_DIR}/src/ScriptHookV.def /out:${SHV_IMPORT}
  DEPENDS src/ScriptHookV.def VERBATIM)
add_custom_target(shv_import DEPENDS "${SHV_IMPORT}")
function(add_identity_mask edition definition)
  set(target "IdentityMask_${edition}")
  add_library(${target} SHARED src/main.cpp src/Compatibility.h)
  add_dependencies(${target} shv_import)
  target_link_libraries(${target} PRIVATE "${SHV_IMPORT}")
  target_compile_features(${target} PRIVATE cxx_std_17)
  target_compile_definitions(${target} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX ${definition})
  target_compile_options(${target} PRIVATE /W4 /permissive- /EHsc)
  set_property(TARGET ${target} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  # Same installed filename in separate folders: never install both ASIs together.
  set_target_properties(${target} PROPERTIES
    PREFIX "" SUFFIX ".asi" OUTPUT_NAME "IdentityMask"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/${edition}"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/${edition}")
endfunction()

add_identity_mask(Legacy IDENTITYMASK_LEGACY)
add_identity_mask(Enhanced IDENTITYMASK_ENHANCED)
