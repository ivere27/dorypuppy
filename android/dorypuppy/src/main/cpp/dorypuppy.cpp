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
static uv_mutex_t mutex;

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
    int r = uv_mutex_init(&mutex);
    ASSERT(r == 0);

    srand(time(NULL));
    n = new std::thread(loop, uv_loop);
    n->detach();

    return JNI_VERSION_1_6;
}


extern "C"
JNIEXPORT int JNICALL
Java_io_tempage_dorypuppy_DoryPuppy_spawn(
        JNIEnv *env,
        jobject obj,
        jobjectArray cmdArray,
        jlong timeout = 0,
        jstring cwd = nullptr,
        jobjectArray envArray = nullptr) {

    int err;
    double uptime;
    err = uv_uptime(&uptime);
    LOGI("uv_uptime: %" PRIu64, uptime);

    jint cmdSize = env->GetArrayLength(cmdArray);
    char *args[cmdSize+1];
    LOGI("cmdSize: %d", cmdSize);

    for (int i=0; i<cmdSize; i++) {
        jstring string = (jstring) (env->GetObjectArrayElement(cmdArray, i));
        args[i] = (char *) env->GetStringUTFChars(string, 0);
        LOGI("args[%d]: %s", i, args[i]);
    }
    args[cmdSize] = NULL;


    jint envSize = (envArray == nullptr) ? 0 : env->GetArrayLength(envArray);
    char *environment[envSize+1];
    LOGI("envSize: %d", envSize);

    for (int i=0; i<envSize; i++) {
        jstring string = (jstring) (env->GetObjectArrayElement(envArray, i));
        environment[i] = (char *) env->GetStringUTFChars(string, 0);
        LOGI("env[%d]: %s", i, environment[i]);
    }
    environment[envSize] = NULL;

    DoryProcessSpawn *process = new DoryProcessSpawn(
            uv_loop,
            args,
            (cwd == nullptr) ? NULL : (char *)env->GetStringUTFChars(cwd, 0),
            (envArray == nullptr) ? NULL : environment);

    process->obj = env->NewGlobalRef(obj);
    process->clazz = env->FindClass("io/tempage/dorypuppy/DoryPuppy");
    process->jniStdoutCallback = env->GetMethodID(process->clazz, "jniStdout", "(I[B)V");
    process->jniStderrCallback = env->GetMethodID(process->clazz, "jniStderr", "(I[B)V");
    process->jniExitCallback = env->GetMethodID(process->clazz, "jniExit", "(IJI)V");
    process->jniTimeoutCallback = env->GetMethodID(process->clazz, "jniTimeout", "(I)V");


    process->timeout = timeout;
    int r = process->on("timeout", [process]() {
        LOGI("timeout fired");
        int pid = process->getPid();
        g_env->CallVoidMethod(process->obj, process->jniTimeoutCallback, pid);
    })
    .on("stdout", [process](char* buf, ssize_t nread) {
        //std::string str(buf, nread);
        //LOGI("%s", str.c_str());

        jbyteArray array = g_env->NewByteArray(nread);
        g_env->SetByteArrayRegion(array,0,nread,(jbyte*)buf);
        g_env->CallVoidMethod(process->obj, process->jniStdoutCallback, process->getPid(), array);
        g_env->DeleteLocalRef(array);
    })
    .on("stderr", [process](char* buf, ssize_t nread) {
        // std::string str(buf, nread);
        // LOGI("%s", str.c_str());

        jbyteArray array = g_env->NewByteArray(nread);
        g_env->SetByteArrayRegion(array,0,nread,(jbyte*)buf);
        g_env->CallVoidMethod(process->obj, process->jniStderrCallback, process->getPid(), array);
        g_env->DeleteLocalRef(array);
    })
    .on("exit", [process](int64_t exitStatus, int termSignal) {
        int pid = process->getPid();
        LOGI("pid : %d, exit code : %lld , signal : %d", pid, exitStatus, termSignal);

        g_env->CallVoidMethod(process->obj, process->jniExitCallback, pid, exitStatus, termSignal);

        uv_mutex_lock(&mutex);
        processList.erase(pid);
        uv_mutex_unlock(&mutex);
    })
    .spawn();
    processIndex++;
    LOGI("************** process index : %lld", processIndex);

    // check the result of spawn()
    if (r != 0) {
        LOGI("%s %s", uv_err_name(r), uv_strerror(r));
        return r;
    }

    int pid = process->getPid();
    LOGI("child pid : %d", pid);

    uv_mutex_lock(&mutex);
    processList[pid] = std::make_shared<DoryProcessSpawn>(*process);
    uv_mutex_unlock(&mutex);

    return pid;
}

extern "C"
JNIEXPORT void JNICALL
Java_io_tempage_dorypuppy_DoryPuppy_kill(
        JNIEnv *env,
        jobject obj,
        int pid,
        int signal) {

    if (processList.count(pid))
        processList[pid]->kill(signal);
}

extern "C"
JNIEXPORT jstring JNICALL
Java_io_tempage_dorypuppy_DoryPuppy_uverror(
        JNIEnv *env,
        jobject obj,
        int r) {
    std::string s = uv_err_name(r);
    s += ": ";
    s += uv_strerror(r);

    return env->NewStringUTF(s.c_str());
}