list(APPEND examples
    pingpong
    generator
    timer
    sleep
    tcpserver_epoll
    tcpserver_iouring
)


foreach (t IN LISTS examples) 
    # Test help
    add_executable(${t} 
        ${t}.c
        $<TARGET_OBJECTS:caio>
    )
    target_include_directories(${t} PUBLIC "${PROJECT_BINARY_DIR}")
    target_link_libraries(${t} PUBLIC clog mrb)
    add_custom_target(${t}_exec COMMAND ${t})
    add_custom_target(${t}_debug COMMAND gdb ./${t})
    add_custom_target(${t}_profile
        COMMAND "valgrind" ${VALGRIND_FLAGS} ./${t}
    )
endforeach()
