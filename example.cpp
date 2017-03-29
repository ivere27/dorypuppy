#include <cstdio>
#include <cassert>
#include "uv.h"

static uv_alloc_cb uvAllocCb = [](uv_handle_t* handle, size_t size, uv_buf_t* buf) {
  buf->base = (char*)malloc(size);
  buf->len = size;
};

uv_process_t process;
uv_process_options_t options = {0};
uv_stdio_container_t stdio[3];
uv_pipe_t pipeOut; // stdout
uv_pipe_t pipeErr; // stderr

int main() {
  int r;
  uv_loop_t *uv_loop = uv_default_loop();

  uint64_t start;
  start = uv_hrtime();
  printf("hrtime : %llu\n", start);

  // pipes
  uv_pipe_init(uv_loop, &pipeOut, 0);
  uv_pipe_init(uv_loop, &pipeErr, 0);

  options.stdio = stdio;
  options.stdio[0].flags = UV_IGNORE; // FIXME : supports input
  options.stdio[1].flags = (uv_stdio_flags)(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
  options.stdio[1].data.stream = (uv_stream_t*)&pipeOut;
  options.stdio[2].flags = (uv_stdio_flags)(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
  options.stdio[2].data.stream = (uv_stream_t*)&pipeErr;
  options.stdio_count = 3;

  options.exit_cb = [](uv_process_t *req, int64_t exit_status, int term_signal) {
    printf("exitStatus : %lld, termSignal : %d\n", exit_status, term_signal);

    uv_close((uv_handle_t*)req, NULL);
    uv_close((uv_handle_t*)&pipeOut, [](uv_handle_t*){});
    uv_close((uv_handle_t*)&pipeErr, [](uv_handle_t*){});
  };

  char* args[3];
  args[0] = (char*)"ls";
  args[1] = (char*)"/";
  args[2] = NULL;

  options.file = args[0];
  options.args = args;

  r = uv_spawn(uv_loop, &process, &options);
  if (r != 0)
    printf("ERROR!");

  // stdout pipe
  r = uv_read_start((uv_stream_t*) &pipeOut, uvAllocCb, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    //printf("nread : %d\n", nread);
    if (nread > 0) {
      for(int i = 0; i<nread;i++)
        printf("%c", buf->base[i]);
    } else if (nread < 0) {
      assert(nread == UV_EOF);
    }
  });
  if (r != 0)
    printf("ERROR!");

  // stderr pipe
  r = uv_read_start((uv_stream_t*) &pipeErr, uvAllocCb, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    //printf("nread : %d\n", nread);
    if (nread > 0) {
      for(int i = 0; i<nread;i++)
        printf("%c", buf->base[i]);
    } else if (nread < 0) {
      assert(nread == UV_EOF);
    }
  });
  if (r != 0)
    printf("ERROR!");


  return uv_run(uv_loop, UV_RUN_DEFAULT);
}