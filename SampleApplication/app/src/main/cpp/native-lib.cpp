#include <jni.h>
#include <string>
#include <cinttypes>
#include <iostream>
#include <thread>

#include <android/log.h>
#include "uv.h"

#include "DoryProcessSpawn.hpp"

using namespace std;
using namespace spawn;

#define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, "dorypuppy::", __VA_ARGS__))

std::thread *n;
uv_loop_t* uv_loop = uv_default_loop();
map<int, shared_ptr<DoryProcessSpawn>> processList;

void loop(uv_loop_t* uv_loop) {
    uv_timer_t timer;
    int r;

    r = uv_timer_init(uv_loop, &timer);
    ASSERT(r==0);

    // repeat every 1000 millisec to make the loop lives forever
    // later, it will be used as whatdog.
    r = uv_timer_start(&timer, [](uv_timer_t* timer){
        LOGI("in timer. process cout : %d", processList.size());
    }, 0, 1000);
    ASSERT(r == 0);
    uv_run(uv_loop, UV_RUN_DEFAULT);
}

extern "C"
JNIEXPORT void JNICALL
Java_io_tempage_dorypuppy_MainActivity_doryInit() {
    //init
    n = new std::thread(loop, uv_loop);
    n->detach();
}


extern "C"
JNIEXPORT void JNICALL
Java_io_tempage_dorypuppy_MainActivity_doryTest(
        JNIEnv *env,
        jobject /* this */) {

    int err;
    double uptime;
    err = uv_uptime(&uptime);
    LOGI("uv_uptime: %" PRIu64, uptime);

    char *args[3];
    args[0] = (char *) "/system/bin/top";
    args[1] = NULL; //(char *) "/";
    args[2] = NULL;

    DoryProcessSpawn *process = new DoryProcessSpawn(uv_loop, args);
    //process.timeout = 10;
    int r = process->on("timeout", []() {
        LOGI("timeout fired");
    })
    .on("stdout", [](char* buf, ssize_t nread) {
        std::string str(buf, nread);
        LOGI("%s", str.c_str());
    })
    .on("stderr", [](char* buf, ssize_t nread) {
        std::string str(buf, nread);
        LOGI("%s", str.c_str());
    })
    .on("exit", [process](int64_t exitStatus, int termSignal) {
        processList.erase(process->getPid());
        LOGI("exit code : %lld , signal : %d", exitStatus, termSignal);
    })
    .spawn();

    // check the result of spawn()
    if (r != 0) {
        LOGI("%s %s", uv_err_name(r), uv_strerror(r));
    } else {
        processList[process->getPid()] = std::make_shared<DoryProcessSpawn>(*process);
        LOGI("child pid : %d", process->getPid());
    }
}

extern "C"
JNIEXPORT jstring JNICALL
Java_io_tempage_dorypuppy_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";

    return env->NewStringUTF(hello.c_str());
}
