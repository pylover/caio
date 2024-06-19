list(APPEND examples
    pingpong
    generator
    timer
    sleep
    tcpserver_epoll
)


foreach (t IN LISTS examples) 
    # Test help
    add_executable(${t} 
        ${t}.c
        $<TARGET_OBJECTS:caio>
    )
    target_include_directories(${t} PUBLIC "${PROJECT_BINARY_DIR}")
    target_link_libraries(${t} PUBLIC mrb)
    add_custom_target(${t}_exec COMMAND ${t})
    add_custom_target(${t}_debug COMMAND gdb 
        --command=${PROJECT_SOURCE_DIR}/examples/${t}.gdb
        ./${t}
    )

    add_custom_target(${t}_profile
        COMMAND "valgrind" ${VALGRIND_FLAGS} ./${t}
    )
endforeach()
