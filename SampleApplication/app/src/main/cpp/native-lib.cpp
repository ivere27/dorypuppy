#include <jni.h>
#include <string>
#include <cinttypes>

#include <android/log.h>
#include "uv.h"

#define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, "dorypuppy::", __VA_ARGS__))

extern "C"
JNIEXPORT jstring JNICALL
Java_io_tempage_dorypuppy_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {

    int err;
    double uptime;
    err = uv_uptime(&uptime);
    LOGI("uv_uptime: %" PRIu64, uptime);

    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
