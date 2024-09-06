list(APPEND examples
  pingpong
  generator
)


if (CAIO_EPOLL OR CAIO_SELECT)
  list(APPEND examples
    fdmon_sleep
    fdmon_timer
    fdmon_tcpserver
  )
endif ()


if (CAIO_URING)
  list(APPEND examples
    uring_cat
    uring_tcpserver
  )
endif ()


foreach (t IN LISTS examples) 
  # Test help
  add_executable(${t} 
    ${t}.c
    $<TARGET_OBJECTS:caio>
  )
  target_link_libraries(${t} PUBLIC clog)
  if (CAIO_URING)
  target_link_libraries(${t} PUBLIC uring)
  endif ()
  target_include_directories(${t} PUBLIC "${PROJECT_BINARY_DIR}")

  add_custom_target(${t}_exec 
  COMMAND ./${t} $$CLI_ARGS
  DEPENDS ${t}
  )
  add_custom_target(${t}_debug 
  COMMAND gdb 
  DEPENDS ${t}
    --command=${PROJECT_SOURCE_DIR}/examples/${t}.gdb
    ./${t} $$CLI_ARGS
  )

  add_custom_target(${t}_profile
    COMMAND "valgrind" ${VALGRIND_FLAGS} ./${t} $$CLI_ARGS
    DEPENDS ${t}
  )
endforeach ()
