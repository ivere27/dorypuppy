#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <thread>
#include <unistd.h>
#include "DoryProcessSpawn.hpp"

using namespace std;
using namespace spawn;

std::thread *n;
uv_loop_t* uv_loop = uv_default_loop();
map<int, shared_ptr<DoryProcessSpawn>> processList;
static uv_mutex_t mutex;

void loop(uv_loop_t* uv_loop) {
  uv_timer_t timer;
  int r;

  r = uv_timer_init(uv_loop, &timer);
  ASSERT(r==0);

  // repeat every 1000 millisec to make the loop lives forever
  // later, it will be used as whatchdog.
  r = uv_timer_start(&timer, [](uv_timer_t* timer){
    cout << "in timer. process cout : " << processList.size() << endl;
  }, 0, 1000);
  ASSERT(r == 0);
  uv_run(uv_loop, UV_RUN_DEFAULT);
}

int main() {
  //init
  int r = uv_mutex_init(&mutex);
  ASSERT(r == 0);

  n = new std::thread(loop, uv_loop);
  n->detach();
  srand(time(NULL));

  int i=0;
  while(1) {
    i++;

    if (i > 10) {
      usleep(10*1000);
      continue;
    }
    char buf[10];
    sprintf(buf,"%i",rand()%10000);

    char* args[3];
    args[0] = (char*)"./child";
    args[1] = buf;  // (char*)"2000";
    args[2] = NULL;

    // FIXME : leak
    DoryProcessSpawn *process = new DoryProcessSpawn(uv_loop, args);
    //process.timeout = 1000;
    int r = process->on("timeout", []() {
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
    .on("exit", [process](int64_t exitStatus, int termSignal) {
      cout << "exit code : " << exitStatus << endl;
      cout << "signal : " << termSignal << endl;

      uv_mutex_lock(&mutex);
      processList.erase(process->getPid());
      uv_mutex_unlock(&mutex);
    })
    .spawn();

    // check the result of spawn()
    if (r != 0)
      cout << uv_err_name(r) << " " << uv_strerror(r) << endl;
    else {
      uv_mutex_lock(&mutex);
      processList[process->getPid()] = std::make_shared<DoryProcessSpawn>(*process);
      cout << "child pid : " << process->getPid() << endl;
      uv_mutex_unlock(&mutex);
    }
  }

  return 0;
}