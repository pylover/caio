cmake_minimum_required(VERSION 3.7)
project(caio 
    VERSION 3.1.0
    LANGUAGES C
)
#include(CMakeDependentOption)
#cmake_dependent_option(CAIO_USE_EPOLL "Use epoll(7)" ON "CAIO_USE_MODULES" OFF)

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fms-extensions")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE=1")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")


if(CMAKE_BUILD_TYPE!=Release)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
endif()


# Menu config

# Modules
# option(CAIO_USE_MODULES "Use caio modules system." ON)

# max modules
set(CAIO_MODULES_MAX 8 CACHE STRING "Maximum allowed modules.")
set_property(CACHE CAIO_MODULES_MAX PROPERTY STRINGS 4 8 32 64 128 256)

# epoll
option(CAIO_USE_EPOLL "Use epoll(7)" ON)

add_custom_target(menu COMMAND ccmake ${PROJECT_BINARY_DIR} )
add_custom_target(fresh COMMAND cmake ${PROJECT_SOURCE_DIR} --fresh)


configure_file(caio/manifest.h.in caio/manifest.h)
configure_file(caio/options.h.in caio/options.h @ONLY)
include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR})


add_library(caio STATIC 
  caio/taskpool.c caio/taskpool.h
  caio/caio.c caio/caio.h
)


if(CAIO_USE_EPOLL)
  target_sources(caio
    PUBLIC 
      ${CMAKE_CURRENT_SOURCE_DIR}/caio/epoll.c
      ${CMAKE_CURRENT_SOURCE_DIR}/caio/epoll.h
	  # ${CMAKE_CURRENT_SOURCE_DIR}/caio/sleep.h
      # ${CMAKE_CURRENT_SOURCE_DIR}/caio/sleep.c
  )
  install(FILES caio/epoll.h DESTINATION "include/caio")
  # install(FILES caio/sleep.h DESTINATION "include/caio")
endif()


# Install
install(TARGETS caio DESTINATION "lib")
install(FILES caio/caio.h DESTINATION "include/caio")
install(FILES caio/generic.h DESTINATION "include/caio")
install(FILES caio/generic.c DESTINATION "include/caio")


# Lint
set(PRETTYC_FLAGS
    --recursive
    --verbose=0
    --repository=.
    --extensions=c,h,in
    --linelength=80
    --headers=h,in
    --includeorder=standardcfirst
    --root=.
    #--quiet
    ${PROJECT_SOURCE_DIR}
)
add_custom_target(lint
    COMMAND prettyc
    ${PRETTYC_FLAGS}
)


# CPack
set(CPACK_PACKAGE_FILE_NAME "libcaio-${PROJECT_VERSION}")
set(CPACK_SET_DESTDIR true)
set(CPACK_PACKAGE_NAME libcaio)
set(CPACK_PACKAGE_CONTACT "Vahid Mardani <vahid.mardani@gmail.com>")
set(CPACK_GENERATOR DEB)
include(CPack)


# Valgrind, Profiling
set(VALGRIND_FLAGS
    -s
    --tool=memcheck 
    --leak-check=yes 
    --show-reachable=yes 
    --num-callers=20 
    --track-fds=yes 
)


# Examples
if (NOT DEFINED ENV{SKIP_EXAMPLES})
  add_subdirectory(examples)
endif()
