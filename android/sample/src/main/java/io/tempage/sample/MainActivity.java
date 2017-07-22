package io.tempage.sample;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.os.StrictMode;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;
import android.widget.TextView;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;

import java.io.File;
import java.io.IOException;

import io.tempage.dorypuppy.DoryProcess;
import io.tempage.dorypuppy.DoryPuppy;

public class MainActivity extends AppCompatActivity {
//    private static final DoryPuppy doryPuppy =  DoryPuppy.getInstance();

    Messenger mService = null;
    boolean mIsBound;


    class IncomingHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MessengerService.MSG_INIT:
                    break;
                case MessengerService.MSG_STDOUT:
                    Bundle bundle = msg.getData();
                    String line = bundle.getString("stdout");
                    break;
                default:
                    super.handleMessage(msg);
            }
        }
    }

    final Messenger mMessenger = new Messenger(new IncomingHandler());

        private ServiceConnection mConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className,
                                       IBinder service) {
            mService = new Messenger(service);

            try {
                Message msg = Message.obtain(null,
                        MessengerService.MSG_REGISTER_CLIENT);
                msg.replyTo = mMessenger;
                mService.send(msg);

                // Give it some value as an example.
                msg = Message.obtain(null,
                        MessengerService.MSG_SET_VALUE, this.hashCode(), 0);
                mService.send(msg);

            } catch (RemoteException e) {
            }

        }

        public void onServiceDisconnected(ComponentName className) {
            mService = null;
        }
    };


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

//        if (BuildConfig.DEBUG) {
//            StrictMode.setThreadPolicy(new StrictMode.ThreadPolicy.Builder()
//                    .detectAll()
//                    .penaltyLog()
//                    .penaltyDropBox()
//                    .penaltyDialog()
//                    .build());
//            StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder()
//                    .detectAll()
//                    .penaltyLog()
//                    .penaltyDropBox()
//                    //.penaltyDeath()
//                    .build());
//        }


        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                // start test
                Intent brocastIntent = new Intent("start");
                LocalBroadcastManager.getInstance(getApplicationContext()).sendBroadcast(brocastIntent);

                Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();

                // cal or vmstat : possibly StrictMode error // args[0] = (char *) "/system/bin/top";
                // to test stderr :  /system/bin/cat / -> /system/bin/sh: cat: /: Is a directory
                // to test timeout : /system/bin/top  with p.start(1000*5);
                final DoryProcess p = new DoryProcess("top");
                p.directory(new File("/"));
                p.environment().put("TEST","ABCD");
                p.setOnExitListener(new DoryPuppy.ExitListener() {
                    @Override
                    public void listener(long code, int signal) {
                        Log.d("main activity", "pid : " + p.pid() + ", code : " + code + " , signal : " + signal);
                    }
                });
                p.setOnStdoutListener(new DoryPuppy.StdListener() {
                    @Override
                    public void listener(byte[] array) {
                        Log.d("main activity stdout", new String(array));
                    }
                });
                p.setOnStderrListener(new DoryPuppy.StdListener() {
                    @Override
                    public void listener(byte[] array) {
                        Log.d("main activity stderr", new String(array));
                    }
                });
                try {
                    final int pid = p.start(1000*5);
                    Log.d("main activity", "pid : " + pid);
                    //p.kill();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        });

        Intent intent  = new Intent(this, MessengerService.class);
        startService(intent);
        bindService(intent, mConnection, Context.BIND_AUTO_CREATE);
        mIsBound = true;

        // Example of a call to a native method
        TextView tv = (TextView) findViewById(R.id.sample_text);
        tv.setText("...");
    }

    @Override
    protected void onDestroy() {
        if (mIsBound) {
            if (mService != null) {
                try {
                    Message msg = Message.obtain(null,
                            MessengerService.MSG_UNREGISTER_CLIENT);
                    msg.replyTo = mMessenger;
                    mService.send(msg);
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
            }

            try {
                unbindService(mConnection);
            } catch (Exception e){
                e.printStackTrace();
            }
            mIsBound = false;
        }

        super.onDestroy();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }


}
