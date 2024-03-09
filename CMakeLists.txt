cmake_minimum_required(VERSION 3.7)
project(caio 
    VERSION 3.1.0
    LANGUAGES C
)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fms-extensions")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE=1")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")


configure_file(caio/manifest.h.in caio/manifest.h)
include_directories(${PROJECT_SOURCE_DIR})


add_library(caio STATIC 
    caio/caio.c caio/caio.h
    caio/sleep.c
    caio/taskpool.c caio/taskpool.h
)


# Install
install(TARGETS caio DESTINATION "lib")
install(FILES caio.h DESTINATION "include/caio")
install(FILES sleep.h DESTINATION "include/caio")
install(FILES generic.h DESTINATION "include/caio")
install(FILES generic.c DESTINATION "include/caio")


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
