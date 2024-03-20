- All todos
- Preserve filename, function name and line number on CAIO_THROW
- Use pre allocated callstack instead of multiple malloc/free
- Readme
  - zero dependencies
  - example dependencies: mrb

- io_uring
  - Huge memory allocation instead of mmap
  - Use atomic store and load as the liburing way
  - Fixed files and buffers

- Macros to enable io_uring and epoll
