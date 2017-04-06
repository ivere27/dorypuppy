#include <iostream>
#include "SimpleProcessSpawn.hpp"

using namespace std;
using namespace spawn;

int main() {
  uv_loop_t* uv_loop = uv_default_loop();
  char* args[3];
  args[0] = (char*)"./child";
  args[1] = (char*)"200";
  args[2] = NULL;

  SimpleProcessSpawn process(uv_loop, args);
  process.timeout = 1000;
  process.on("error", [](Error &&error){
    cout << error.name << endl;
    cout << error.message << endl;
  })
  .on("response", [](Response &&response){
    cout << "exit code : " << response.exitStatus << endl;
    cout << "signal : " << response.termSignal << endl;
    cout << response.out.str();
    cout << response.err.str();
  })
  .spawn();

  return uv_run(uv_loop, UV_RUN_DEFAULT);
}