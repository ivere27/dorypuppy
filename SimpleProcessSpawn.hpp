#ifndef SIMPLE_PROCESS_SPAWN_H_
#define SIMPLE_PROCESS_SPAWN_H_

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

      uv_close((uv_handle_t*) req, NULL);
      uv_close((uv_handle_t*)&child->pipeOut, [](uv_handle_t*){});
      uv_close((uv_handle_t*)&child->pipeErr, [](uv_handle_t*){});

      child->emit("exit", exit_status, term_signal);
    };

    this->args = args;
    options.file = args[0];
    options.args = args;
  }

  // FIXME : look ugly
  // work around emit and on.
  SimpleProcessSpawn& emit(const string name, int64_t exit_status, int term_signal) {
    if (name.compare("exit") == 0 && exitCallback != nullptr)
      exitCallback(exit_status, term_signal);
    return *this;
  }
  SimpleProcessSpawn& emit(const string name, char* buf, ssize_t nread) {
    if (name.compare("stdout") == 0 && stdoutCallback != nullptr)
      stdoutCallback(buf, nread);
    if (name.compare("stderr") == 0 && stderrCallback != nullptr)
      stderrCallback(buf, nread);
    return *this;
  }
  SimpleProcessSpawn& emit(const string name, const char* _name, const char* message) {
    if (name.compare("error") == 0 && exitCallback != nullptr)
      errorCallback(_name, message);
    return *this;
  }

  //FIXME : variadic ..Args using tuple.
  template <typename... Args>
  SimpleProcessSpawn& emit(const string name, Args&&... args) {
    if (eventListeners.count(name))
      eventListeners[name]();

    return *this;
  }

  SimpleProcessSpawn& on(const string name, std::function<void(const char*, const char*)> func) {
    if (name.compare("error") == 0)
      errorCallback = func;

    return *this;
  }
  SimpleProcessSpawn& on(const string name, std::function<void(int64_t,int)> func) {
    if (name.compare("exit") == 0)
      exitCallback = func;

    return *this;
  }
  SimpleProcessSpawn& on(const string name, std::function<void(char*,ssize_t)> func) {
    if (name.compare("stdout") == 0)
      stdoutCallback = func;
    else if (name.compare("stderr") == 0)
        stderrCallback = func;

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
        child->emit("stdout", buf->base, nread);
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
        child->emit("stderr", buf->base, nread);
      } else if (nread < 0) {
        assert(nread == UV_EOF);
      }
    });
    if(r != 0) {
      this->emit("error", uv_err_name(r), uv_strerror(r));
      return;
    }

  }

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
  std::function<void(const char*, const char*)> errorCallback = nullptr;
  std::function<void(int64_t,int)> exitCallback = nullptr;
  std::function<void(char*, ssize_t)> stdoutCallback = nullptr;
  std::function<void(char*, ssize_t)> stderrCallback = nullptr;

  void _clearTimer() {
    if (!uv_is_closing((uv_handle_t*)&this->timer)) {
      uv_close((uv_handle_t*)&this->timer, [](uv_handle_t*){});
    }
  }
};

} // namespace spawn

#endif // SIMPLE_PROCESS_SPAWN_H_