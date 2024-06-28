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


if(CAIO_EPOLL)
  list(APPEND examples
	# epoll_tcpserver
  )
endif()


# if(CAIO_SELECT)
#   list(APPEND examples
# 	select_tcpserver
#   )
# endif()


foreach (t IN LISTS examples) 
  # Test help
  add_executable(${t} 
    ${t}.c
    $<TARGET_OBJECTS:caio>
  )
  target_include_directories(${t} PUBLIC "${PROJECT_BINARY_DIR}")
  add_custom_target(${t}_exec COMMAND ${t})
  add_custom_target(${t}_debug COMMAND gdb 
    --command=${PROJECT_SOURCE_DIR}/examples/${t}.gdb
    ./${t}
  )

  add_custom_target(${t}_profile
    COMMAND "valgrind" ${VALGRIND_FLAGS} ./${t}
  )
endforeach()
