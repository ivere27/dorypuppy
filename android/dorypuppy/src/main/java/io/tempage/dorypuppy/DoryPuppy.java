package io.tempage.dorypuppy;

import android.util.Log;

import java.io.File;
import java.io.IOException;
import java.util.HashMap;
import java.util.Hashtable;
import java.util.Map;

public class DoryPuppy {
    private Map<Integer, DoryProcess> processList;

    static {
        System.loadLibrary("uv");
        System.loadLibrary("dorypuppy");
    }

    public interface StdListener {
        void listener(byte[] array);
    }
    public interface ExitListener {
        void listener(long code, int signal);
    }

    private DoryPuppy() {
        this.processList = new Hashtable<Integer, DoryProcess>();
    }
    private static class Singleton {
        private static final DoryPuppy instance = new DoryPuppy();
    }
    public static DoryPuppy getInstance () {
        return Singleton.instance;
    }

    // jni from c
    private void jniStdout(int pid, byte[] array) {
        //Log.i("dorypuppy", pid + " : " + new String(array));
        if (processList.containsKey(pid) && processList.get(pid).stdoutListener != null) {
            processList.get(pid).stdoutListener.listener(array);
        }
    }
    private void jniStderr(int pid, byte[] array) {
        //Log.i("dorypuppy", pid + " : " + new String(array));
        if (processList.containsKey(pid) && processList.get(pid).stderrListener != null) {
            processList.get(pid).stderrListener.listener(array);
        }
    }
    private void jniExit(int pid, long code, int signal) {
        //Log.i("dorypuppy", pid + " : " + code + " / " + signal);

        if (processList.containsKey(pid)) {
            processList.get(pid).code = code;
            processList.get(pid).signal = signal;

            if (processList.get(pid).exitListener != null)
                processList.get(pid).exitListener.listener(code, signal);
        }

        processList.remove(pid);
    }


    // public
    public void puppyKill(DoryProcess puppy, int signal) {
        kill(puppy.pid, signal);
    }

    public int puppySpawn(DoryProcess puppy) throws IOException {
        String[] cmdArray = puppy.command.toArray(new String[puppy.command.size()]);
        String[] envArray = new String[puppy.environment.size()];
        int i = 0;
        for (Map.Entry<String, String> entry : puppy.environment.entrySet()) {
            envArray[i++] = entry.getKey() + "=" + entry.getValue();
        }

        int pid = spawn(
                cmdArray,
                puppy.timeout,
                (puppy.directory != null) ? puppy.directory.toString() : null,
                (envArray.length > 0) ? envArray : null
        );

        if (pid <= 0)
            throw new IOException(uverror(pid));

        processList.put(pid, puppy);
        return pid;
    }


    // native functions
    public native String uverror(int r);
    private native int spawn(String[] cmdArray, long timeout, String directory, String[] envArray);
    private native void kill(int pid, int signal);
}
