cmake_minimum_required(VERSION 3.7)
project(caio 
    VERSION 5.2.0
    LANGUAGES C
)
include(CMakeDependentOption)


# GCC and it's flags
# set(CMAKE_C_COMPILER gcc)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fms-extensions")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE=1")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")


# Build type and debug symbols
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release)
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Include debug symbols" FORCE)
endif ()
if (CMAKE_BUILD_TYPE!=Release)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
endif ()


# Semaphore
option(CONFIG_CAIO_SEMAPHORE "Enable caio semaphore." ON)


# Builtin modules 
option(CONFIG_CAIO_MODULES "Enable caio modules system." ON)

if (CONFIG_CAIO_MODULES)
  set(CONFIG_CAIO_MODULES_MAX 8 CACHE STRING "Maximum allowed modules.")
  set_property(CACHE CONFIG_CAIO_MODULES_MAX PROPERTY 
    STRINGS 4 8 32 64 128 256)

  set(CONFIG_CAIO_MODULES_TICKTIMEOUT_SHORT_US 1000 CACHE 
    STRING "Modules short timeout in microseconds.")
  
  set(CONFIG_CAIO_MODULES_TICKTIMEOUT_LONG_US 10000 CACHE 
    STRING "Modules short timeout in microseconds.")

else ()
  unset(CONFIG_CAIO_MODULES_MAX CACHE)
endif ()


# Builtin IO modules 
cmake_dependent_option(CONFIG_CAIO_SIGNAL 
  "Enable caio signal modules." 
  ON "CONFIG_CAIO_MODULES" OFF)
cmake_dependent_option(CONFIG_CAIO_FDMON 
  "Enable caio IO modules." 
  ON "CONFIG_CAIO_MODULES" OFF)
cmake_dependent_option(CONFIG_CAIO_EPOLL 
  "Build and link epoll(7) caio IO module." 
  ON "CONFIG_CAIO_FDMON" OFF)
cmake_dependent_option(CONFIG_CAIO_SELECT 
  "Build and link select(2) caio IO module."
  ON "CONFIG_CAIO_FDMON" OFF)
cmake_dependent_option(CONFIG_CAIO_URING 
  "Build and link io_uring(7) caio IO module."
  ON "CONFIG_CAIO_FDMON" OFF)


# Maximum allowed uring jobs per caio task 
if (CONFIG_CAIO_URING)
  set(CONFIG_CAIO_URING_TASK_MAXWAITING 8 CACHE 
    STRING "Maximum allowed io_uring jobs per caio task.")
  set_property(CACHE CONFIG_CAIO_URING_TASK_MAXWAITING PROPERTY 
    STRINGS 4 8 32 64 128 256)
endif()


# Menu config
add_custom_target(menu COMMAND ccmake ${PROJECT_BINARY_DIR} )
add_custom_target(fresh COMMAND cmake ${PROJECT_SOURCE_DIR} --fresh)


configure_file(caio/config.h.in caio/config.h @ONLY)
include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR})


add_library(caio SHARED)
target_sources(caio 
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/caio/caio.h
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/caio/caio.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/caio/taskpool.h
    ${CMAKE_CURRENT_SOURCE_DIR}/caio/taskpool.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/caio/sleep.h
    ${CMAKE_CURRENT_SOURCE_DIR}/caio/sleep.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/caio/semaphore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/caio/semaphore.c 
)
add_library(caio_generic INTERFACE 
    caio/generic.h
    caio/generic.c
)


if (CONFIG_CAIO_SEMAPHORE)
  target_sources(caio 
    INTERFACE
      ${CMAKE_CURRENT_SOURCE_DIR}/caio/semaphore.h
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/caio/semaphore.c 
  )
  install(FILES caio/semaphore.h DESTINATION "include/caio")
endif ()


if (CONFIG_CAIO_SIGNAL)
  target_sources(caio
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/caio/signal.h
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/caio/signal.c
  )
  install(FILES caio/signal.h DESTINATION "include/caio")
endif ()


if (CONFIG_CAIO_FDMON)
  target_sources(caio
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/caio/fdmon.h
    ${CMAKE_CURRENT_SOURCE_DIR}/caio/sleep.h
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/caio/sleep.c
  )
  install(FILES caio/fdmon.h DESTINATION "include/caio")
  install(FILES caio/sleep.h DESTINATION "include/caio")

  if (CONFIG_CAIO_EPOLL)
    target_sources(caio
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/caio/epoll.h
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/caio/epoll.c
    )
    install(FILES caio/epoll.h DESTINATION "include/caio")
  endif ()
  
  if (CONFIG_CAIO_SELECT)
    target_sources(caio
    INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}/caio/select.h
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/caio/select.c
    )
    install(FILES caio/select.h DESTINATION "include/caio")
  endif ()
endif ()


if (CONFIG_CAIO_URING)
  target_sources(caio
  INTERFACE 
      ${CMAKE_CURRENT_SOURCE_DIR}/caio/uring.h
  PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/caio/uring.c
  )
  target_link_libraries(caio PUBLIC uring) 
  install(FILES caio/uring.h DESTINATION "include/caio")
endif ()


# Install
install(TARGETS caio DESTINATION "lib")
install(FILES ${CMAKE_BINARY_DIR}/caio/config.h DESTINATION "include/caio")
install(FILES caio/caio.h DESTINATION "include/caio")
install(FILES caio/generic.h DESTINATION "include/caio")
install(FILES caio/generic.c DESTINATION "include/caio")


# Uninstall
add_custom_target("uninstall" COMMENT "Uninstall installed files")
add_custom_command(
  TARGET "uninstall"
  POST_BUILD
  COMMENT "Uninstall files with install_manifest.txt"
  COMMAND xargs rm -vf < install_manifest.txt || echo Nothing in
            install_manifest.txt to be uninstalled!
)


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
option(CAIO_EXAMPLES "Build caio examples." ON)
if (CAIO_EXAMPLES) 
  add_subdirectory(examples)
endif ()
