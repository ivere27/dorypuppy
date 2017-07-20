package io.tempage.dorypuppy;

import android.util.Log;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Hashtable;
import java.util.List;
import java.util.Map;

public class DoryProcess {
    private static final DoryPuppy doryPuppy =  DoryPuppy.getInstance();
    protected List<String> command;
    protected File directory;
    protected Map<String, String> environment;
    protected int pid = 0;
    protected long timeout = 0;

    // listener. instead of InputOutputStream;
    protected DoryPuppy.StdListener stdoutListener, stderrListener;
    protected DoryPuppy.ExitListener exitListener;

    public DoryProcess(String... command) {
        this(new ArrayList<String>(Arrays.asList(command)));
    }

    public DoryProcess(List<String> command) {
        if (command == null) {
            throw new NullPointerException("command == null");
        }
        this.command = command;
        this.environment = new Hashtable<String, String>(System.getenv());
    }

    public DoryProcess directory(File directory) {
        this.directory = directory;
        return this;
    }

    public Map<String, String> environment() {
        return environment;
    }

    public DoryProcess setOnStdoutListener(DoryPuppy.StdListener listener) {
        this.stdoutListener = listener;
        return this;
    }
    public DoryProcess setOnStderrListener(DoryPuppy.StdListener listener) {
        this.stderrListener = listener;
        return this;
    }
    public DoryProcess setOnExitListener(DoryPuppy.ExitListener listener) {
        this.exitListener = listener;
        return this;
    }

    public int start(long timeout) throws  IOException {
        this.timeout = timeout;
        this.pid = doryPuppy.puppySpawn(this);
        return pid;
    }
    public int start() throws IOException {
        return start(0);
    }
}
