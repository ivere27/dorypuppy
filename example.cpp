#include <iostream>
#include <thread>
#include "DoryProcessSpawn.hpp"

using namespace std;
using namespace spawn;

void loop(uv_loop_t* uv_loop) {
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