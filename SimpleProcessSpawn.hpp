#ifndef SIMPLE_PROCESS_SPAWN_H_
#define SIMPLE_PROCESS_SPAWN_H_

#define SIMPLE_PROCESS_SPAWN_VERSION_MAJOR 0
#define SIMPLE_PROCESS_SPAWN_VERSION_MINOR 1
#define SIMPLE_PROCESS_SPAWN_VERSION_PATCH 0

#include <cstdio>
#include <cassert>
#include <functional>
#include <iostream>
#include <sstream>
#include <map>

#include "uv.h"

namespace spawn {
using namespace std;

// Forward declaration
class Error;
class Response;
class SimpleProcessSpawn;

#if defined(NDEBUG)
# define ASSERT(exp)
#else
# define ASSERT(exp)  assert(exp)
#endif

template<class... T>
using Callback = std::map<std::string, std::function<void(T...)>>;

static uv_alloc_cb uvAllocCb = [](uv_handle_t* handle, size_t size, uv_buf_t* buf) {
  buf->base = (char*)malloc(size);
  buf->len = size;
};

// Error
class Error {
public:
  Error(){};
  Error(const char* name, const char* message) : name(name), message(message) {};
  string name;
  string message;
};

// Response
class Response {
public:
  int64_t exitStatus;
  int termSignal;
  stringstream out;
  stringstream err;
};

class SimpleProcessSpawn {
public:
  SimpleProcessSpawn(char** args) : SimpleProcessSpawn(uv_default_loop(), args) {}
  SimpleProcessSpawn(uv_loop_t *loop, char** args) {
    this->uv_loop = loop;
    process.data = this;
    pipeOut.data = this;
    pipeErr.data = this;
    timer.data = this;

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
      SimpleProcessSpawn *child = (SimpleProcessSpawn*)req->data;
      if (child->timer.loop == child->uv_loop) // which is initialized
        child->_clearTimer();

      child->response.exitStatus = exit_status;
      child->response.termSignal = term_signal;

      uv_close((uv_handle_t*) req, NULL);
      uv_close((uv_handle_t*)&child->pipeOut, [](uv_handle_t*){});
      uv_close((uv_handle_t*)&child->pipeErr, [](uv_handle_t*){});

      child->emit("response");
    };

    this->args = args;
    options.file = args[0];
    options.args = args;
  }
  //FIXME : variadic ..Args using tuple.
  template <typename... Args>
  SimpleProcessSpawn& emit(const string name, Args... args) {
    if (name.compare("response") == 0 && responseCallback != nullptr)
      responseCallback(std::forward<Response>(response));
    else if (name.compare("error") == 0 && errorCallback != nullptr)
      errorCallback(Error(args...));
    else if (eventListeners.count(name))
      eventListeners[name]();

    return *this;
  }

  SimpleProcessSpawn& on(const string name, std::function<void(Response&&)> func) {
    if (name.compare("response") == 0)
      responseCallback = func;

    return *this;
  }
  SimpleProcessSpawn& on(const string name, std::function<void(Error&&)> func) {
    if (name.compare("error") == 0)
      errorCallback = func;

    return *this;
  }
  template <typename... Args>
  SimpleProcessSpawn& on(const string name, std::function<void(Args...)> func) {
    eventListeners[name] = func;

    return *this;
  }
  SimpleProcessSpawn& on(const string name, std::function<void()> func) {
    eventListeners[name] = func;

    return *this;
  }
  void spawn() {
    int r;

    if (timeout > 0) {
      r = uv_timer_init(uv_loop, &timer);
      ASSERT(r == 0);
      r = uv_timer_start(&timer, [](uv_timer_t* timer){
        SimpleProcessSpawn *child = (SimpleProcessSpawn*)timer->data;
        child->_clearTimer();

        uv_process_kill(&child->process, SIGKILL);

        int _err = -ETIMEDOUT;
        child->emit("error", uv_err_name(_err), uv_strerror(_err));
      }, timeout, 0);
      ASSERT(r == 0);
    }

    r = uv_spawn(uv_loop, &process, &options);
    if (r != 0) {
      this->emit("error", uv_err_name(r), uv_strerror(r));
      return;
    }

    // stdout pipe
    r = uv_read_start((uv_stream_t*) &pipeOut, uvAllocCb, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
      SimpleProcessSpawn *child = (SimpleProcessSpawn*)stream->data;
      if (nread > 0) {
        child->response.out.rdbuf()->sputn(buf->base, nread);
      } else if (nread < 0) {
        assert(nread == UV_EOF);
      }
    });

    if(r != 0) {
      this->emit("error", uv_err_name(r), uv_strerror(r));
      return;
    }

    // stderr pipe
    r = uv_read_start((uv_stream_t*) &pipeErr, uvAllocCb, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
      SimpleProcessSpawn *child = (SimpleProcessSpawn*)stream->data;
      if (nread > 0) {
        child->response.err.rdbuf()->sputn(buf->base, nread);
      } else if (nread < 0) {
        assert(nread == UV_EOF);
      }
    });
    if(r != 0) {
      this->emit("error", uv_err_name(r), uv_strerror(r));
      return;
    }

  }

  Response response;
  unsigned int timeout = 0; // forever in defaults
private:
  uv_loop_t* uv_loop;
  uv_process_t process;
  uv_process_options_t options = {0};
  uv_stdio_container_t stdio[3];
  char** args;
  uv_pipe_t pipeOut; // stdout
  uv_pipe_t pipeErr; // stderr
  uv_timer_t timer;

  Callback<> eventListeners;
  std::function<void(Response&&)> responseCallback = nullptr;
  std::function<void(Error&&)> errorCallback = nullptr;

  void _clearTimer() {
    if (!uv_is_closing((uv_handle_t*)&this->timer)) {
      uv_close((uv_handle_t*)&this->timer, [](uv_handle_t*){});
    }
  }
};

} // namespace spawn

#endif // SIMPLE_PROCESS_SPAWN_H_