#include <jni.h>
#include <string>
#include <cinttypes>
#include <iostream>

#include <android/log.h>
#include "uv.h"

#include "DoryProcessSpawn.hpp"

using namespace std;
using namespace spawn;

#define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, "dorypuppy::", __VA_ARGS__))

extern "C"
JNIEXPORT jstring JNICALL
Java_io_tempage_dorypuppy_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {

    std::string hello = "Hello from C++";

    int err;
    double uptime;
    err = uv_uptime(&uptime);
    LOGI("uv_uptime: %" PRIu64, uptime);

    uv_loop_t *uv_loop = uv_default_loop();
    char *args[3];
    args[0] = (char *) "/system/bin/ls";
    args[1] = (char *) "/";
    args[2] = NULL;

    DoryProcessSpawn process(uv_loop, args);
    //process.timeout = 10;
    int r = process.on("timeout", []() {
        LOGI("timeout fired");
    })
    .on("stdout", [&hello](char* buf, ssize_t nread) {
        std::string str(buf, nread);
        LOGI("%s", str.c_str());

        hello += str;
    })
    .on("stderr", [&hello](char* buf, ssize_t nread) {
        std::string str(buf, nread);
        LOGI("%s", str.c_str());

        hello += str;
    })
    .on("exit", [](int64_t exitStatus, int termSignal) {
        LOGI("exit code : %lld , signal : %d", exitStatus, termSignal);
    })
    .spawn();

    // check the result of spawn()
    if (r != 0) {
        LOGI("%s %s", uv_err_name(r), uv_strerror(r));
    }

    uv_run(uv_loop, UV_RUN_DEFAULT);

    return env->NewStringUTF(hello.c_str());
}
