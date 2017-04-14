#include <iostream>
#include <thread>
#include "DoryProcessSpawn.hpp"

using namespace std;
using namespace spawn;

void loop(uv_loop_t* uv_loop) {
  uv_timer_t timer;
  int r;

  r = uv_timer_init(uv_loop, &timer);
  ASSERT(r==0);

  // repeat every 1000 millisec to make the loop lives forever
  // later, it will be used as whatdog.
  r = uv_timer_start(&timer, [](uv_timer_t* timer){
    cout << "in timer" << endl;
  }, 0, 1000);
  ASSERT(r == 0);
  uv_run(uv_loop, UV_RUN_DEFAULT);
}

int main() {
  uv_loop_t* uv_loop = uv_default_loop();
  char* args[3];
  args[0] = (char*)"./child";
  args[1] = (char*)"200";
  args[2] = NULL;

  DoryProcessSpawn process(uv_loop, args);
  //process.timeout = 1000;
  int r = process.on("timeout", []() {
    cout << "timeout fired" << endl;
  })
  .on("stdout", [](char* buf, ssize_t nread) {
    for(int i=0;i<nread;i++)
      printf("%c", buf[i]);
  })
  .on("stderr", [](char* buf, ssize_t nread) {
    for(int i=0;i<nread;i++)
      printf("%c", buf[i]);
  })
  .on("exit", [](int64_t exitStatus, int termSignal) {
    cout << "exit code : " << exitStatus << endl;
    cout << "signal : " << termSignal << endl;
  })
  .spawn();

  // check the result of spawn()
  if (r != 0)
    cout << uv_err_name(r) << " " << uv_strerror(r) << endl;

  std::thread n(loop, uv_loop);
  n.join();

  return 0;
}