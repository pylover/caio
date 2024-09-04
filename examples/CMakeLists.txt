list(APPEND examples
  pingpong
  generator
)


if(CAIO_EPOLL OR CAIO_SELECT)
  list(APPEND examples
	sleep
    timer
    tcpserver
  )
endif()


if(CAIO_IOURING)
  list(APPEND iouring_examples
	  cat
  )
  list(APPEND examples ${iouring_examples})
endif()


foreach (t IN LISTS examples) 
  # Test help
  add_executable(${t} 
    ${t}.c
    $<TARGET_OBJECTS:caio>
  )
  target_include_directories(${t} PUBLIC "${PROJECT_BINARY_DIR}")
  add_custom_target(${t}_exec COMMAND ./${t} $$CLI_ARGS)
  add_custom_target(${t}_debug COMMAND gdb 
    --command=${PROJECT_SOURCE_DIR}/examples/${t}.gdb
    ./${t} $$CLI_ARGS
  )

  add_custom_target(${t}_profile
    COMMAND "valgrind" ${VALGRIND_FLAGS} ./${t} $$CLI_ARGS
  )
endforeach()
 

if(CAIO_IOURING)
  foreach (t IN LISTS examples) 
    target_link_libraries(${t} PUBLIC uring)
  endforeach()
endif()
