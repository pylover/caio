## now
- caio_uring_reset(umod, task) function
- change timeout_ms to timeout_us

## later
- USE CLOG 
- readme about uring feature
- pre-emptive using an auxiliary parent process.
- rename all ifdef, ifndef, elifdef, elifndef with if defined syntax.
- Prevent compile on kernel <= 2.6.9
- fixed callstack allocation per task (option)
- Dynamic(infinite) stack size (option)
- All todos
- Preserve filename, function name and line number on CAIO_THROW
- Readme & documentation
- cmake find_packages

- io_uring
  - readme: cmake CAIO_URING_ENABLED
  - readme: install liburing
  - Huge memory allocation instead of mmap
  - Use atomic store and load as the liburing way
  - Fixed files and buffers

