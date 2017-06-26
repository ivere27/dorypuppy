#include <cinttypes>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <thread>
#include <time.h>

#include <android/log.h>
#include <jni.h>

#include "uv.h"
#include "DoryProcessSpawn.hpp"

using namespace std;
using namespace spawn;

#ifdef NDEBUG
#define LOGI(...)
#else
#define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, "dorypuppy::", __VA_ARGS__))
#endif

std::thread *n = NULL;
uv_loop_t* uv_loop = uv_default_loop();
map<int, shared_ptr<DoryProcessSpawn>> processList;

JavaVM *jvm;
JNIEnv *g_env;
unsigned long long processIndex = 0;

void loop(uv_loop_t* uv_loop) {
    uv_timer_t timer;
    int r;

    jvm->AttachCurrentThread(&g_env, NULL);

    r = uv_timer_init(uv_loop, &timer);
    ASSERT(r==0);

    // repeat every 1000 millisec to make the loop lives forever
    // later, it will be used as watchdog.
    r = uv_timer_start(&timer, [](uv_timer_t* timer){
        LOGI("in timer. process cout : %d", processList.size());
    }, 0, 1000);
    ASSERT(r == 0);
    uv_run(uv_loop, UV_RUN_DEFAULT);
    jvm->DetachCurrentThread();
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;
    jvm = vm;

    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    //init
    srand(time(NULL));
    n = new std::thread(loop, uv_loop);
    n->detach();

    return JNI_VERSION_1_6;
}


extern "C"
JNIEXPORT int JNICALL
Java_io_tempage_dorypuppy_DoryPuppy_doryTest(
        JNIEnv *env,
        jobject obj) {

    int err;
    double uptime;
    err = uv_uptime(&uptime);
    LOGI("uv_uptime: %" PRIu64, uptime);

    char *args[2];

    // cal or vmstat : possibly StrictMode error
    args[0] = (char *) "/system/bin/top";
    args[1] = NULL;

    DoryProcessSpawn *process = new DoryProcessSpawn(uv_loop, args);

    process->obj = env->NewGlobalRef(obj);
    process->clazz = env->FindClass("io/tempage/dorypuppy/DoryPuppy");
    process->testLog = env->GetMethodID(process->clazz, "test", "([B)V");

    process->timeout = rand()%(10*1000*10);
    int r = process->on("timeout", []() {
        LOGI("timeout fired");
    })
    .on("stdout", [process](char* buf, ssize_t nread) {
        //std::string str(buf, nread);
        //LOGI("%s", str.c_str());

        jbyteArray array = g_env->NewByteArray(nread);
        g_env->SetByteArrayRegion(array,0,nread,(jbyte*)buf);
        g_env->CallVoidMethod(process->obj, process->testLog, array);
        g_env->DeleteLocalRef(array);
    })
    .on("stderr", [process](char* buf, ssize_t nread) {
        // std::string str(buf, nread);
        // LOGI("%s", str.c_str());

        jbyteArray array = g_env->NewByteArray(nread);
        g_env->SetByteArrayRegion(array,0,nread,(jbyte*)buf);
        g_env->CallVoidMethod(process->obj, process->testLog, array);
        g_env->DeleteLocalRef(array);
    })
    .on("exit", [process](int64_t exitStatus, int termSignal) {
        LOGI("pid : %d, exit code : %lld , signal : %d", process->getPid(), exitStatus, termSignal);
        processList.erase(process->getPid());
    })
    .spawn();
    processIndex++;
    LOGI("************** process index : %lld", processIndex);

    // check the result of spawn()
    if (r != 0) {
        LOGI("%s %s", uv_err_name(r), uv_strerror(r));
        return r;
    }

    processList[process->getPid()] = std::make_shared<DoryProcessSpawn>(*process);
    LOGI("child pid : %d", process->getPid());

    return process->getPid();
}

extern "C"
JNIEXPORT jstring JNICALL
Java_io_tempage_dorypuppy_DoryPuppy_stringFromJNI(
        JNIEnv *env,
        jobject obj) {
    std::string hello = "Hello from C++";

    JNIEnv *_env;
    jint res = jvm->GetEnv((void**)&_env, JNI_VERSION_1_6);
    jstring jstr = _env->NewStringUTF("JNI String Test");
    jclass clazz = _env->FindClass("io/tempage/dorypuppy/DoryPuppy");
    jmethodID testLog = _env->GetMethodID(clazz, "test", "(Ljava/lang/String;)V");
    _env->CallVoidMethod(obj, testLog, jstr);

    return env->NewStringUTF(hello.c_str());
}
