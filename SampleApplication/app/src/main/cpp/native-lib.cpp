#include <jni.h>
#include <string>
#include <cinttypes>
#include <iostream>

#include <android/log.h>
#include "uv.h"

#include "SimpleProcessSpawn.hpp"

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

    {
        uv_loop_t *uv_loop = uv_default_loop();
        char *args[3];
        args[0] = (char *) "/system/bin/ls";
        args[1] = (char *) "/";
        args[2] = NULL;

        SimpleProcessSpawn process(uv_loop, args);
        process.timeout = 1000;
        process.on("error", [](Error &&error) {
            cout << error.name << endl;
            cout << error.message << endl;
        })
        .on("response", [&](Response &&response) {
            cout << "exit code : " << response.exitStatus << endl;
            cout << "signal : " << response.termSignal << endl;
            cout << response.out.str();
            cout << response.err.str();

            hello = response.out.str();
            LOGI("%s", hello.c_str());
        })
        .spawn();

        uv_run(uv_loop, UV_RUN_DEFAULT);
    }

    return env->NewStringUTF(hello.c_str());
}
