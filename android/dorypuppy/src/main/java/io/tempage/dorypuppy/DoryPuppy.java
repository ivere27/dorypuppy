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

    public void test(byte[] array) {
        final String s = new String(array);
        Log.i("____________",s);

        // we are in different thread.
        // android.view.ViewRootImpl$CalledFromWrongThreadException: Only the original thread that created a view hierarchy can touch its views.
//        Thread thread= new Thread(){
//            @Override
//            public void run() {
//                runOnUiThread(new Runnable() {
//                    public void run() {
//                        TextView tv = (TextView) findViewById(R.id.sample_text);
//                        tv.setText(s);
//                    }
//                });
//            }
//        };
//        thread.start();
    }


    // jni from c
    private void jniStdout(int pid, byte[] array) {
        final String s = new String(array);
        Log.i("stdout_from_c", pid + " : " +s);

        if (processList.containsKey(pid) && processList.get(pid).stdoutListener != null) {
            processList.get(pid).stdoutListener.listener(array);
        }
    }
    private void jniStderr(int pid, byte[] array) {
        final String s = new String(array);
        Log.i("stderr_from_c", pid + " : " +s);

        if (processList.containsKey(pid) && processList.get(pid).stderrListener != null) {
            processList.get(pid).stderrListener.listener(array);
        }
    }
    private void jniExit(int pid, long code, int signal) {
        Log.i("exit_from_c", pid + " : " + code + " / " + signal);
        processList.remove(pid);

        if (processList.containsKey(pid) && processList.get(pid).exitListener != null) {
            processList.get(pid).exitListener.listener(code, signal);
        }
    }


    // public
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

        if (pid <= 0) {
            throw new IOException(uverror(pid));
        }
        processList.put(pid, puppy);

        return pid;
    }


    /**
     * A native method that is implemented by the 'dorypuppy' native library,
     * which is packaged with this application.
     */
    public native String uverror(int r);
    private native int spawn(String[] cmdArray, long timeout, String directory, String[] envArray);
}
