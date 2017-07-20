# dorypuppy
event-driven process manager on Android by JNI(libuv)

## how to use
1. 'Import Moudle' in Android Studio (shared objects are included for arm/x86)
2. Source directory : (git root)/android/dorypuppy
3. add "compile project(':dorypuppy')" to dependencies{} in your build.gradle

```java
DoryProcess p = new DoryProcess("/system/bin/ls", "/system/bin"); // cmd, args...
p.directory(new File("/system"));                   // cwd, optional
p.environment().put("KEY","VALUE");                 // env, optional

p.setOnExitListener(new DoryPuppy.ExitListener() {  // optional
    @Override
    public void listener(long code, int signal) {
        Log.d("exit", code + " / " + signal);
    }
});
p.setOnStdoutListener(new DoryPuppy.StdListener() {  // optional
    @Override
    public void listener(byte[] array) {
        Log.d("stdout", new String(array));
    }
});
p.setOnStderrListener(new DoryPuppy.StdListener() {  // optional
    @Override
    public void listener(byte[] array) {
        Log.d("stderr", new String(array));
    }
});

try {
    int pid = p.start();                             // p.start(long timeout)
    Log.d("app", "pid : " + pid);
} catch (IOException e) {
    e.printStackTrace();
}
```
