# Sets up an interface library for compiling with asan and ubsan.
# Also sets some env variables useful for adding to tests.
macro( setup_sanitizer_interface )

  message(STATUS "Sanitizer selection: ${PROJECT_SANITIZER_SELECTION}")

  if(PROJECT_SANITIZER_SELECTION STREQUAL "address" OR
      PROJECT_SANITIZER_SELECTION STREQUAL "all")
    set(SANITIZER_COMPILER_FLAGS
      -fno-omit-frame-pointer
      -fsanitize-recover=all
      -fsanitize=address
      -fsanitize=undefined
      -fno-sanitize=vptr
      )
    set(SANITIZER_LINKER_FLAGS
      -fsanitize=address
      -fsanitize=undefined
      )

    # internal variable
    set(BUILD_WITH_SANITIZERS ON)
  else()
    set(BUILD_WITH_SANITIZERS OFF)
  endif()

  set(LSAN_BLACKLIST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lsan-blacklist.txt")

  set(ASAN_OPTIONS "halt_on_error=true:fast_unwind_on_malloc=1:detect_odr_violation=1" )
  set(LSAN_OPTIONS "suppressions=\"${LSAN_BLACKLIST_PATH}\"")
  set(UBSAN_OPTIONS "print_stacktrace=1")

  # for cmake invoke the following as well
  if(${ARGC} EQUAL 0)

    message(STATUS "Setting up ASAN interface")
    add_library(sanitizers INTERFACE IMPORTED)
    
    target_compile_options(sanitizers INTERFACE
      $<$<AND:$<CXX_COMPILER_ID:Clang,GNU>,$<BOOL:${BUILD_WITH_SANITIZERS}>>:${SANITIZER_COMPILER_FLAGS}>
      )
    target_link_options(sanitizers INTERFACE
      $<$<AND:$<CXX_COMPILER_ID:Clang,GNU>,$<BOOL:${BUILD_WITH_SANITIZERS}>>:${SANITIZER_LINKER_FLAGS}>
      )
  endif()

endmacro()

# Sets sanitizer environment variables for a test target
function( set_sanitizer_env_variables_for_test test_name )

  if(BUILD_WITH_SANITIZERS)
    set_property(TEST "${test_name}" PROPERTY ENVIRONMENT "ASAN_OPTIONS=${ASAN_OPTIONS};LSAN_OPTIONS=${LSAN_OPTIONS};UBSAN_OPTIONS=${UBSAN_OPTIONS};GEMMA_MAXRANKS=${gemmaMaxRanks}")
  endif()

endfunction()
