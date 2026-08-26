# Playerbots has direct core integration points and is intentionally static-only.
# Keeping it inside the native Tortoise module framework avoids reviving the
# historical BUILD_PLAYERBOTS top-level CMake fork from PR #79.

if(TORTOISE_MODULE_CMAKE_PHASE STREQUAL "DISCOVERY")
  if(NOT TORTOISE_CURRENT_MODULE_LINKAGE STREQUAL "static")
    message(FATAL_ERROR "mod-playerbots currently requires static linkage (-DMODULE_MOD_PLAYERBOTS=static)")
  endif()
endif()

if(TORTOISE_MODULE_CMAKE_PHASE STREQUAL "POST_TARGETS")
  target_compile_definitions(modules PRIVATE
    CMANGOS
    MANGOSBOT_ZERO
    ENABLE_PLAYERBOTS)

  target_include_directories(modules PRIVATE
    ${CMAKE_SOURCE_DIR}/modules/mod-playerbots/src
    ${CMAKE_SOURCE_DIR}/src/game/MapNodes
    ${CMAKE_SOURCE_DIR}/dep/recastnavigation
    ${CMAKE_SOURCE_DIR}/src/framework/Network)

  target_link_libraries(modules PRIVATE
    shared
    Detour
    g3dlite
    zlib)

  if(WIN32)
    target_link_libraries(modules PRIVATE ws2_32)
  endif()
endif()
