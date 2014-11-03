# Work in progress (mostly due to my complete lack of CMake knowledge)... As 
# far as best practices go this file is probably the complete opposite. 
# Patches welcome. :)

project(hadesmem)

cmake_minimum_required(VERSION 3.0)

add_definitions(
  -DBOOST_USE_WINDOWS_H 
  -DBOOST_SYSTEM_NO_DEPRECATED 
  -DBOOST_FILESYSTEM_NO_DEPRECATED 
  -DSTRICT 
  -DSTRICT_TYPED_ITEMIDS 
  -DUNICODE 
  -D_UNICODE 
  -D_CRT_SECURE_NO_WARNINGS 
  -D_SCL_SECURE_NO_WARNINGS 
  -DWINVER=_WIN32_WINNT_VISTA 
  -D_WIN32_WINNT=_WIN32_WINNT_VISTA
)

link_libraries(
  advapi32 
  comctl32 
  dxguid 
  gdi32 
  ole32 
  shell32 
  shlwapi 
  user32 
)

set(CMAKE_CXX_FLAGS_RELEASE 
  "${CMAKE_CXX_FLAGS_RELEASE} -DHADESMEM_NO_TRACE")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO 
  "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DHADESMEM_NO_TRACE")
set(CMAKE_C_FLAGS_MINSIZEREL 
  "${CMAKE_C_FLAGS_MINSIZEREL} -DHADESMEM_NO_TRACE")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set_property(GLOBAL APPEND_STRING PROPERTY COMPILE_FLAGS 
    "-std=c++1y -ftrapv")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set_property(GLOBAL APPEND_STRING PROPERTY COMPILE_FLAGS 
    "-std=c++1y -ftrapv")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  set_property(GLOBAL APPEND_STRING PROPERTY COMPILE_FLAGS 
    "/Qstd=c++11 /Qwd869 /Qwd2586 /Qwd3346")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set_property(GLOBAL APPEND_STRING PROPERTY COMPILE_FLAGS 
    "/LARGEADDRESSAWARE /volatile:iso /Zc:rvalueCast /wd4345 /wd4503")
  set(CMAKE_CXX_FLAGS_RELEASE 
    "${CMAKE_CXX_FLAGS_RELEASE} /Gw /Gy /d2Zi+")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO 
    "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Gw /Gy /d2Zi+")
  set(CMAKE_C_FLAGS_MINSIZEREL 
    "${CMAKE_C_FLAGS_MINSIZEREL} /Gw /Gy /d2Zi+")
endif()

find_package(Boost)

include_directories(
  ./include/memory 
  ./src/anttweakbar/anttweakbar/include 
  ./src/anttweakbar/anttweakbar/obj 
  ./src/asmjit/asmjit/src 
  ./src/pugixml/pugixml/src 
  ./src/tclap/tclap/include 
  ./src/udis86/udis86 
  ${Boost_INCLUDE_DIR} 
)

set(hadesmem_include_dir ./include/memory/hadesmem)

set(hadesmem_headers 
  ${hadesmem_include_dir}/alloc.hpp
  ${hadesmem_include_dir}/call.hpp
  ${hadesmem_include_dir}/config.hpp
  ${hadesmem_include_dir}/debug_privilege.hpp
  ${hadesmem_include_dir}/error.hpp
  ${hadesmem_include_dir}/find_pattern.hpp
  ${hadesmem_include_dir}/find_procedure.hpp
  ${hadesmem_include_dir}/flush.hpp
  ${hadesmem_include_dir}/injector.hpp
  ${hadesmem_include_dir}/module.hpp
  ${hadesmem_include_dir}/module_list.hpp
  ${hadesmem_include_dir}/patcher.hpp
  ${hadesmem_include_dir}/process.hpp
  ${hadesmem_include_dir}/process_entry.hpp
  ${hadesmem_include_dir}/process_helpers.hpp
  ${hadesmem_include_dir}/process_list.hpp
  ${hadesmem_include_dir}/protect.hpp
  ${hadesmem_include_dir}/read.hpp
  ${hadesmem_include_dir}/region.hpp
  ${hadesmem_include_dir}/region_list.hpp
  ${hadesmem_include_dir}/thread.hpp
  ${hadesmem_include_dir}/thread_entry.hpp
  ${hadesmem_include_dir}/thread_helpers.hpp
  ${hadesmem_include_dir}/thread_list.hpp
  ${hadesmem_include_dir}/write.hpp
)

set(hadesmem_detail_headers 
  ${hadesmem_include_dir}/detail/alias_cast.hpp
  ${hadesmem_include_dir}/detail/argv_quote.hpp
  ${hadesmem_include_dir}/detail/assert.hpp
  ${hadesmem_include_dir}/detail/detour_ref_counter.hpp
  ${hadesmem_include_dir}/detail/filesystem.hpp
  ${hadesmem_include_dir}/detail/find_procedure.hpp
  ${hadesmem_include_dir}/detail/force_initialize.hpp
  ${hadesmem_include_dir}/detail/last_error_preserver.hpp
  ${hadesmem_include_dir}/detail/optional.hpp
  ${hadesmem_include_dir}/detail/protect_guard.hpp
  ${hadesmem_include_dir}/detail/protect_region.hpp
  ${hadesmem_include_dir}/detail/pugixml_helpers.hpp
  ${hadesmem_include_dir}/detail/query_region.hpp
  ${hadesmem_include_dir}/detail/read_impl.hpp
  ${hadesmem_include_dir}/detail/recursion_protector.hpp
  ${hadesmem_include_dir}/detail/region_alloc_size.hpp
  ${hadesmem_include_dir}/detail/remote_thread.hpp
  ${hadesmem_include_dir}/detail/scope_warden.hpp
  ${hadesmem_include_dir}/detail/self_path.hpp
  ${hadesmem_include_dir}/detail/smart_handle.hpp
  ${hadesmem_include_dir}/detail/srw_lock.hpp
  ${hadesmem_include_dir}/detail/static_assert.hpp
  ${hadesmem_include_dir}/detail/static_assert_x86.hpp
  ${hadesmem_include_dir}/detail/str_conv.hpp
  ${hadesmem_include_dir}/detail/thread_aux.hpp
  ${hadesmem_include_dir}/detail/to_upper_ordinal.hpp
  ${hadesmem_include_dir}/detail/toolhelp.hpp
  ${hadesmem_include_dir}/detail/trace.hpp
  ${hadesmem_include_dir}/detail/type_traits.hpp
  ${hadesmem_include_dir}/detail/warning_disable_prefix.hpp
  ${hadesmem_include_dir}/detail/warning_disable_suffix.hpp
  ${hadesmem_include_dir}/detail/winapi.hpp
  ${hadesmem_include_dir}/detail/winternl.hpp
  ${hadesmem_include_dir}/detail/write_impl.hpp
)

set(hadesmem_pelib_headers 
  ${hadesmem_include_dir}/pelib/bound_import_desc.hpp
  ${hadesmem_include_dir}/pelib/bound_import_desc_list.hpp
  ${hadesmem_include_dir}/pelib/bound_import_fwd_ref.hpp
  ${hadesmem_include_dir}/pelib/bound_import_fwd_ref_list.hpp
  ${hadesmem_include_dir}/pelib/dos_header.hpp
  ${hadesmem_include_dir}/pelib/export.hpp
  ${hadesmem_include_dir}/pelib/export_dir.hpp
  ${hadesmem_include_dir}/pelib/export_list.hpp
  ${hadesmem_include_dir}/pelib/import_dir.hpp
  ${hadesmem_include_dir}/pelib/import_dir_list.hpp
  ${hadesmem_include_dir}/pelib/import_thunk.hpp
  ${hadesmem_include_dir}/pelib/import_thunk_list.hpp
  ${hadesmem_include_dir}/pelib/nt_headers.hpp
  ${hadesmem_include_dir}/pelib/pe_file.hpp
  ${hadesmem_include_dir}/pelib/relocation.hpp
  ${hadesmem_include_dir}/pelib/relocation_block.hpp
  ${hadesmem_include_dir}/pelib/relocation_block_list.hpp
  ${hadesmem_include_dir}/pelib/relocation_list.hpp
  ${hadesmem_include_dir}/pelib/section.hpp
  ${hadesmem_include_dir}/pelib/section_list.hpp
  ${hadesmem_include_dir}/pelib/tls_dir.hpp
)

set(hadesmem_all_headers 
  ${hadesmem_headers} 
  ${hadesmem_detail_headers} 
  ${hadesmem_pelib_headers}
)

add_library(hadesmem STATIC ${hadesmem_all_headers})
set_target_properties(hadesmem PROPERTIES LINKER_LANGUAGE CXX)

add_definitions(
  -DTW_STATIC 
  -DTW_NO_LIB_PRAGMA 
)

add_library(anttweakbar STATIC 
  ./src/anttweakbar/anttweakbar/src/TwEventWin.c
  ./src/anttweakbar/anttweakbar/src/AntPerfTimer.h
  ./src/anttweakbar/anttweakbar/src/d3d10vs2003.h
  ./src/anttweakbar/anttweakbar/src/LoadOGL.h
  ./src/anttweakbar/anttweakbar/src/LoadOGLCore.h
  ./src/anttweakbar/anttweakbar/src/MiniGLFW.h
  ./src/anttweakbar/anttweakbar/src/MiniGLUT.h
  ./src/anttweakbar/anttweakbar/src/MiniSDL12.h
  ./src/anttweakbar/anttweakbar/src/MiniSDL13.h
  ./src/anttweakbar/anttweakbar/src/MiniSFML16.h
  ./src/anttweakbar/anttweakbar/src/TwBar.h
  ./src/anttweakbar/anttweakbar/src/TwColors.h
  ./src/anttweakbar/anttweakbar/src/TwDirect3D9.h
  ./src/anttweakbar/anttweakbar/src/TwDirect3D10.h
  ./src/anttweakbar/anttweakbar/src/TwDirect3D11.h
  ./src/anttweakbar/anttweakbar/src/TwFonts.h
  ./src/anttweakbar/anttweakbar/src/TwGraph.h
  ./src/anttweakbar/anttweakbar/src/TwMgr.h
  ./src/anttweakbar/anttweakbar/src/TwOpenGL.h
  ./src/anttweakbar/anttweakbar/src/TwOpenGLCore.h
  ./src/anttweakbar/anttweakbar/src/TwPrecomp.h
  ./src/anttweakbar/anttweakbar/src/LoadOGL.cpp
  ./src/anttweakbar/anttweakbar/src/LoadOGLCore.cpp
  ./src/anttweakbar/anttweakbar/src/TwBar.cpp
  ./src/anttweakbar/anttweakbar/src/TwColors.cpp
  ./src/anttweakbar/anttweakbar/src/TwDirect3D9.cpp
  ./src/anttweakbar/anttweakbar/src/TwDirect3D10.cpp
  ./src/anttweakbar/anttweakbar/src/TwDirect3D11.cpp
  ./src/anttweakbar/anttweakbar/src/TwEventSFML.cpp
  ./src/anttweakbar/anttweakbar/src/TwFonts.cpp
  ./src/anttweakbar/anttweakbar/src/TwMgr.cpp
  ./src/anttweakbar/anttweakbar/src/TwOpenGL.cpp
  ./src/anttweakbar/anttweakbar/src/TwOpenGLCore.cpp
  ./src/anttweakbar/anttweakbar/src/TwPrecomp.cpp
)

add_definitions(
  -DASMJIT_STATIC 
  -DASMJIT_BUILD_X86 
  -DASMJIT_BUILD_X64 
)

add_library(asmjit STATIC 
  ./src/asmjit/asmjit/src/asmjit/apibegin.h
  ./src/asmjit/asmjit/src/asmjit/apiend.h
  ./src/asmjit/asmjit/src/asmjit/asmjit.h
  ./src/asmjit/asmjit/src/asmjit/base.h
  ./src/asmjit/asmjit/src/asmjit/build.h
  ./src/asmjit/asmjit/src/asmjit/config.h
  ./src/asmjit/asmjit/src/asmjit/contrib.h
  ./src/asmjit/asmjit/src/asmjit/host.h
  ./src/asmjit/asmjit/src/asmjit/x86.h
  ./src/asmjit/asmjit/src/asmjit/base/assembler.h
  ./src/asmjit/asmjit/src/asmjit/base/codegen.h
  ./src/asmjit/asmjit/src/asmjit/base/compiler.h
  ./src/asmjit/asmjit/src/asmjit/base/constpool.h
  ./src/asmjit/asmjit/src/asmjit/base/context_p.h
  ./src/asmjit/asmjit/src/asmjit/base/cpuinfo.h
  ./src/asmjit/asmjit/src/asmjit/base/cputicks.h
  ./src/asmjit/asmjit/src/asmjit/base/error.h
  ./src/asmjit/asmjit/src/asmjit/base/func.h
  ./src/asmjit/asmjit/src/asmjit/base/globals.h
  ./src/asmjit/asmjit/src/asmjit/base/intutil.h
  ./src/asmjit/asmjit/src/asmjit/base/lock.h
  ./src/asmjit/asmjit/src/asmjit/base/logger.h
  ./src/asmjit/asmjit/src/asmjit/base/operand.h
  ./src/asmjit/asmjit/src/asmjit/base/podlist.h
  ./src/asmjit/asmjit/src/asmjit/base/podvector.h
  ./src/asmjit/asmjit/src/asmjit/base/runtime.h
  ./src/asmjit/asmjit/src/asmjit/base/string.h
  ./src/asmjit/asmjit/src/asmjit/base/vectypes.h
  ./src/asmjit/asmjit/src/asmjit/base/vmem.h
  ./src/asmjit/asmjit/src/asmjit/base/zone.h
  ./src/asmjit/asmjit/src/asmjit/base/assembler.cpp
  ./src/asmjit/asmjit/src/asmjit/base/codegen.cpp
  ./src/asmjit/asmjit/src/asmjit/base/compiler.cpp
  ./src/asmjit/asmjit/src/asmjit/base/constpool.cpp
  ./src/asmjit/asmjit/src/asmjit/base/context.cpp
  ./src/asmjit/asmjit/src/asmjit/base/cpuinfo.cpp
  ./src/asmjit/asmjit/src/asmjit/base/cputicks.cpp
  ./src/asmjit/asmjit/src/asmjit/base/error.cpp
  ./src/asmjit/asmjit/src/asmjit/base/func.cpp
  ./src/asmjit/asmjit/src/asmjit/base/globals.cpp
  ./src/asmjit/asmjit/src/asmjit/base/intutil.cpp
  ./src/asmjit/asmjit/src/asmjit/base/logger.cpp
  ./src/asmjit/asmjit/src/asmjit/base/operand.cpp
  ./src/asmjit/asmjit/src/asmjit/base/podvector.cpp
  ./src/asmjit/asmjit/src/asmjit/base/runtime.cpp
  ./src/asmjit/asmjit/src/asmjit/base/string.cpp
  ./src/asmjit/asmjit/src/asmjit/base/vmem.cpp
  ./src/asmjit/asmjit/src/asmjit/base/zone.cpp
  ./src/asmjit/asmjit/src/asmjit/x86/x86assembler.h
  ./src/asmjit/asmjit/src/asmjit/x86/x86compiler.h
  ./src/asmjit/asmjit/src/asmjit/x86/x86context_p.h
  ./src/asmjit/asmjit/src/asmjit/x86/x86cpuinfo.h
  ./src/asmjit/asmjit/src/asmjit/x86/x86func.h
  ./src/asmjit/asmjit/src/asmjit/x86/x86inst.h
  ./src/asmjit/asmjit/src/asmjit/x86/x86operand.h
  ./src/asmjit/asmjit/src/asmjit/x86/x86regs.h
  ./src/asmjit/asmjit/src/asmjit/x86/x86util.h
  ./src/asmjit/asmjit/src/asmjit/x86/x86assembler.cpp
  ./src/asmjit/asmjit/src/asmjit/x86/x86compiler.cpp
  ./src/asmjit/asmjit/src/asmjit/x86/x86context.cpp
  ./src/asmjit/asmjit/src/asmjit/x86/x86cpuinfo.cpp
  ./src/asmjit/asmjit/src/asmjit/x86/x86func.cpp
  ./src/asmjit/asmjit/src/asmjit/x86/x86inst.cpp
  ./src/asmjit/asmjit/src/asmjit/x86/x86operand.cpp
  ./src/asmjit/asmjit/src/asmjit/x86/x86regs.cpp
  ./src/asmjit/asmjit/src/asmjit/x86/x86util.cpp
)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set_property(TARGET asmjit APPEND_STRING PROPERTY LINK_FLAGS "/ignore:4221")
endif()

add_definitions(
  -DPUGIXML_HEADER_ONLY 
  -DPUGIXML_WCHAR_MODE 
)

add_library(udis86 STATIC 
  ./src/udis86/udis86/libudis86/decode.c
  ./src/udis86/udis86/libudis86/itab.c
  ./src/udis86/udis86/libudis86/syn.c
  ./src/udis86/udis86/libudis86/syn-att.c
  ./src/udis86/udis86/libudis86/syn-intel.c
  ./src/udis86/udis86/libudis86/udis86.c
  ./src/udis86/udis86/libudis86/extern.h
  ./src/udis86/udis86/libudis86/itab.h
  ./src/udis86/udis86/libudis86/syn.h
  ./src/udis86/udis86/libudis86/types.h
  ./src/udis86/udis86/libudis86/udint.h
)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set_property(TARGET udis86 APPEND_STRING PROPERTY COMPILE_FLAGS "/wd4013")
endif()

add_library(cerberus SHARED 
  ./examples/cerberus/callbacks.hpp
  ./examples/cerberus/d3d9.hpp
  ./examples/cerberus/d3d10.hpp
  ./examples/cerberus/d3d11.hpp
  ./examples/cerberus/dxgi.hpp
  ./examples/cerberus/dxgi_helpers.hpp
  ./examples/cerberus/exception.hpp
  ./examples/cerberus/helpers.hpp
  ./examples/cerberus/input.hpp
  ./examples/cerberus/main.hpp
  ./examples/cerberus/module.hpp
  ./examples/cerberus/plugin.hpp
  ./examples/cerberus/process.hpp
  ./examples/cerberus/render.hpp
  ./examples/cerberus/d3d9.cpp
  ./examples/cerberus/d3d10.cpp
  ./examples/cerberus/d3d11.cpp
  ./examples/cerberus/dxgi.cpp
  ./examples/cerberus/dxgi_helpers.cpp
  ./examples/cerberus/exception.cpp
  ./examples/cerberus/helpers.cpp
  ./examples/cerberus/input.cpp
  ./examples/cerberus/main.cpp
  ./examples/cerberus/module.cpp
  ./examples/cerberus/plugin.cpp
  ./examples/cerberus/process.cpp
  ./examples/cerberus/render.cpp
)
target_link_libraries(cerberus anttweakbar asmjit udis86)

add_library(cxdivinity SHARED 
  ./examples/cxdivinity/dump.hpp
  ./examples/cxdivinity/gui.hpp
  ./examples/cxdivinity/dump.cpp
  ./examples/cxdivinity/gui.cpp
  ./examples/cxdivinity/main.cpp
  ./examples/cxdivinity/sdk/ability.hpp
  ./examples/cxdivinity/sdk/character.hpp
  ./examples/cxdivinity/sdk/character_manager.hpp
  ./examples/cxdivinity/sdk/character_template.hpp
  ./examples/cxdivinity/sdk/flagged.hpp
  ./examples/cxdivinity/sdk/hash_table.hpp
  ./examples/cxdivinity/sdk/inventory.hpp
  ./examples/cxdivinity/sdk/inventory_manager.hpp
  ./examples/cxdivinity/sdk/item.hpp
  ./examples/cxdivinity/sdk/item_manager.hpp
  ./examples/cxdivinity/sdk/item_template.hpp
  ./examples/cxdivinity/sdk/offset.hpp
  ./examples/cxdivinity/sdk/static_assert.hpp
  ./examples/cxdivinity/sdk/status.hpp
  ./examples/cxdivinity/sdk/std_string.hpp
  ./examples/cxdivinity/sdk/talent.hpp
  ./examples/cxdivinity/sdk/trait.hpp
  ./examples/cxdivinity/sdk/translated_string_repository.hpp
  ./examples/cxdivinity/sdk/tri_string.hpp
  ./examples/cxdivinity/sdk/vec3f.hpp
  ./examples/cxdivinity/sdk/character.cpp
  ./examples/cxdivinity/sdk/character_manager.cpp
  ./examples/cxdivinity/sdk/character_template.cpp
  ./examples/cxdivinity/sdk/inventory.cpp
  ./examples/cxdivinity/sdk/inventory_manager.cpp
  ./examples/cxdivinity/sdk/item.cpp
  ./examples/cxdivinity/sdk/item_manager.cpp
  ./examples/cxdivinity/sdk/item_template.cpp
  ./examples/cxdivinity/sdk/status.cpp
  ./examples/cxdivinity/sdk/translated_string_repository.cpp
  ./examples/cxdivinity/sdk/tri_string.cpp
)

add_library(cxexample SHARED 
  ./examples/cxexample/radar.hpp
  ./examples/cxexample/root_window.hpp
  ./examples/cxexample/window.cpp
  ./examples/cxexample/main.cpp
  ./examples/cxexample/radar.cpp
  ./examples/cxexample/root_window.cpp
  ./examples/cxexample/window.cpp
)

add_library(cxwormsclanwars SHARED 
  ./examples/cxwormsclanwars/gui.hpp
  ./examples/cxwormsclanwars/gui.cpp
  ./examples/cxwormsclanwars/main.cpp
  ./examples/cxwormsclanwars/sdk/placeholder.hpp
  ./examples/cxwormsclanwars/sdk/placeholder.cpp
)

add_executable(divinity_trainer 
  ./examples/divinity_trainer/character.hpp
  ./examples/divinity_trainer/character_manager.hpp
  ./examples/divinity_trainer/game_object_manager.hpp
  ./examples/divinity_trainer/hash_table.hpp
  ./examples/divinity_trainer/inventory.hpp
  ./examples/divinity_trainer/inventory_manager.hpp
  ./examples/divinity_trainer/item.hpp
  ./examples/divinity_trainer/item_manager.hpp
  ./examples/divinity_trainer/offsets.hpp
  ./examples/divinity_trainer/std_string.hpp
  ./examples/divinity_trainer/translated_string_repository.hpp
  ./examples/divinity_trainer/tri_string.hpp
  ./examples/divinity_trainer/vec3f.hpp
  ./examples/divinity_trainer/character.cpp
  ./examples/divinity_trainer/character_manager.cpp
  ./examples/divinity_trainer/inventory.cpp
  ./examples/divinity_trainer/inventory_manager.cpp
  ./examples/divinity_trainer/item.cpp
  ./examples/divinity_trainer/item_manager.cpp
  ./examples/divinity_trainer/main.cpp
  ./examples/divinity_trainer/translated_string_repository.cpp
  ./examples/divinity_trainer/tri_string.cpp
)

add_executable(dump
  ./examples/dump/bound_imports.hpp
  ./examples/dump/disassemble.hpp
  ./examples/dump/exports.hpp
  ./examples/dump/filesystem.hpp
  ./examples/dump/headers.hpp
  ./examples/dump/imports.hpp
  ./examples/dump/main.hpp
  ./examples/dump/memory.hpp
  ./examples/dump/print.hpp
  ./examples/dump/relocations.hpp
  ./examples/dump/sections.hpp
  ./examples/dump/strings.hpp
  ./examples/dump/tls.hpp
  ./examples/dump/warning.hpp
  ./examples/dump/bound_imports.cpp
  ./examples/dump/disassemble.cpp
  ./examples/dump/exports.cpp
  ./examples/dump/filesystem.cpp
  ./examples/dump/headers.cpp
  ./examples/dump/imports.cpp
  ./examples/dump/main.cpp
  ./examples/dump/memory.cpp
  ./examples/dump/relocations.cpp
  ./examples/dump/sections.cpp
  ./examples/dump/strings.cpp
  ./examples/dump/tls.cpp
  ./examples/dump/warning.cpp
)
target_link_libraries(dump udis86)

add_executable(esomod
  ./examples/esomod/3d.hpp
  ./examples/esomod/camera_distance.hpp
  ./examples/esomod/fader.hpp
  ./examples/esomod/fog.hpp
  ./examples/esomod/fov.hpp
  ./examples/esomod/time.hpp
  ./examples/esomod/tone_mapping.hpp
  ./examples/esomod/view_distances.hpp
  ./examples/esomod/3d.cpp
  ./examples/esomod/camera_distance.cpp
  ./examples/esomod/fader.cpp
  ./examples/esomod/fog.cpp
  ./examples/esomod/fov.cpp
  ./examples/esomod/main.cpp
  ./examples/esomod/time.cpp
  ./examples/esomod/tone_mapping.cpp
  ./examples/esomod/view_distances.cpp
)

add_executable(inject ./examples/inject/main.cpp)
target_link_libraries(inject asmjit udis86)

add_library(injecttestdep SHARED ./examples/injecttestdep/main.cpp)

add_library(injecttest SHARED ./examples/injecttest/main.cpp)
target_link_libraries(injecttest injecttestdep)


if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(hadesmem_clang_warning_flags "-Wall -Weverything -Wstrict-aliasing -Wstrict-overflow=5 -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-disabled-macro-expansion")
  set_property(TARGET hadesmem APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_clang_warning_flags})
  set_property(TARGET cerberus APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_clang_warning_flags})
  set_property(TARGET cxdivinity APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_clang_warning_flags})
  set_property(TARGET cxexample APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_clang_warning_flags})
  set_property(TARGET cxwormsclanwars APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_clang_warning_flags})
  set_property(TARGET divinity_trainer APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_clang_warning_flags})
  set_property(TARGET dump APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_clang_warning_flags})
  set_property(TARGET esomod APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_clang_warning_flags})
  set_property(TARGET inject APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_clang_warning_flags})
  set_property(TARGET injecttest APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_clang_warning_flags})
  set_property(TARGET injecttestdep APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_clang_warning_flags})
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(hadesmem_gcc_warning_flags "-Wall -ansi -Wpedantic -Wextra -Weffc++ -Wshadow -Wconversion -Winit-self -Wmissing-include-dirs -Wstrict-aliasing -Wstrict-overflow=5 -Wno-effc++ -Wold-style-cast -Wnon-virtual-dtor -Woverloaded-virtual -Winvalid-pch -Wno-multichar -Wno-missing-field-initializers -Wno-unused-but-set-parameter")
  set_property(TARGET hadesmem APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_gcc_warning_flags})
  set_property(TARGET cerberus APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_gcc_warning_flags})
  set_property(TARGET cxdivinity APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_gcc_warning_flags})
  set_property(TARGET cxexample APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_gcc_warning_flags})
  set_property(TARGET cxwormsclanwars APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_gcc_warning_flags})
  set_property(TARGET divinity_trainer APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_gcc_warning_flags})
  set_property(TARGET dump APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_gcc_warning_flags})
  set_property(TARGET esomod APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_gcc_warning_flags})
  set_property(TARGET inject APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_gcc_warning_flags})
  set_property(TARGET injecttest APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_gcc_warning_flags})
  set_property(TARGET injecttestdep APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_gcc_warning_flags})
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  set(hadesmem_intel_warning_flags "/W4")
  set_property(TARGET hadesmem APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_intel_warning_flags})
  set_property(TARGET cerberus APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_intel_warning_flags})
  set_property(TARGET cxdivinity APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_intel_warning_flags})
  set_property(TARGET cxexample APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_intel_warning_flags})
  set_property(TARGET cxwormsclanwars APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_intel_warning_flags})
  set_property(TARGET divinity_trainer APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_intel_warning_flags})
  set_property(TARGET dump APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_intel_warning_flags})
  set_property(TARGET esomod APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_intel_warning_flags})
  set_property(TARGET inject APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_intel_warning_flags})
  set_property(TARGET injecttest APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_intel_warning_flags})
  set_property(TARGET injecttestdep APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_intel_warning_flags})
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set(hadesmem_msvc_warning_flags "/W4 /analyze /sdl")
  set_property(TARGET hadesmem APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_msvc_warning_flags})
  set_property(TARGET cerberus APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_msvc_warning_flags})
  set_property(TARGET cxdivinity APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_msvc_warning_flags})
  set_property(TARGET cxexample APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_msvc_warning_flags})
  set_property(TARGET cxwormsclanwars APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_msvc_warning_flags})
  set_property(TARGET divinity_trainer APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_msvc_warning_flags})
  set_property(TARGET dump APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_msvc_warning_flags})
  set_property(TARGET esomod APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_msvc_warning_flags})
  set_property(TARGET inject APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_msvc_warning_flags})
  set_property(TARGET injecttest APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_msvc_warning_flags})
  set_property(TARGET injecttestdep APPEND_STRING PROPERTY COMPILE_FLAGS ${hadesmem_msvc_warning_flags})
endif()
