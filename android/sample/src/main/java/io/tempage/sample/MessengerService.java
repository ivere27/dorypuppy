package io.tempage.sample;

import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.StrictMode;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Random;

import io.tempage.dorypuppy.DoryProcess;
import io.tempage.dorypuppy.DoryPuppy;

public class MessengerService extends Service {
    private static final String TAG = "DORY";

    ArrayList<Messenger> mClients = new ArrayList<Messenger>();

    static final int MSG_REGISTER_CLIENT = 1;
    static final int MSG_UNREGISTER_CLIENT = 2;

    static final int MSG_SET_VALUE = 3;
    static final int MSG_INIT = 4;
    static final int MSG_STDOUT = 5;

    private static final DoryPuppy doryPuppy =  DoryPuppy.getInstance();
    Thread thread = null;

    class IncomingHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_REGISTER_CLIENT:
                    mClients.add(msg.replyTo);
                    broadcastServiceStatus();
                    break;
                case MSG_UNREGISTER_CLIENT:
                    mClients.remove(msg.replyTo);
                    broadcastServiceStatus();
                    break;
                default:
                    super.handleMessage(msg);
            }
        }
    }

    private void broadcastServiceStatus() {
        Intent brocastIntent = new Intent("responseServiceStatus");
        brocastIntent.putExtra("status", true);
        LocalBroadcastManager.getInstance(getApplicationContext()).sendBroadcast(brocastIntent);
    }


    final Messenger mMessenger = new Messenger(new IncomingHandler());

    private BroadcastReceiver mMessageReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals("start")) {
                return;
            }
            if (intent.getAction().equals("stop")) {
                thread.interrupt();;

                stopSelf();
                return;
            }
        }
    };

    public void doTest() {

        Intent notificationIntent = new Intent(this, MainActivity.class);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, notificationIntent, 0);

        Notification notification = new Notification.Builder(this)
                .setSmallIcon(R.mipmap.ic_launcher)
                .setContentTitle("dorypuppy")
                .setContentText("puppy is running")
                .setContentIntent(pendingIntent)
                .build();
        startForeground(1, notification);

        thread = new Thread() {
            @Override
            public void run() {
                try {
                    while(true) {
                        for (int i = 0; i<20; i++) {
                            DoryProcess p = new DoryProcess("/system/bin/top");
                            try {
                                Random random = new Random();
                                final int pid = p.start(random.nextInt(10) * 1000 * 10);
                                p.setOnExitListener(new DoryPuppy.ExitListener() {
                                    @Override
                                    public void listener(long code, int signal) {
                                        Log.d("main activity", "pid : " + pid + " / " + code + " / " + signal);

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
                                Log.d("main activity", "pid : " + pid);
                            } catch (IOException e) {
                                e.printStackTrace();
                            }
                        }


                        sleep(10*1000*10);
                    }
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        };
        thread.start();
    }


    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (null == intent) {
            Log.e (TAG, "intent was null, flags=" + flags + " bits=" + Integer.toBinaryString (flags));
            return START_STICKY;
        }
        return START_NOT_STICKY; //START_STICKY
    }

    @Override
    public void onCreate() {
        if (BuildConfig.DEBUG) {
            StrictMode.setThreadPolicy(new StrictMode.ThreadPolicy.Builder()
                    .detectAll()
                    .penaltyLog()
                    .penaltyDropBox()
                    .penaltyDialog()
                    .build());
            StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder()
                    .detectAll()
                    .penaltyLog()
                    .penaltyDropBox()
                    //.penaltyDeath()
                    .build());
        }

        IntentFilter filter = new IntentFilter();
        filter.addAction("start");
        filter.addAction("stop");
        LocalBroadcastManager.getInstance(this).registerReceiver(mMessageReceiver, filter);

        doTest();

    }

    @Override
    public void onDestroy() {
        LocalBroadcastManager.getInstance(this).unregisterReceiver(mMessageReceiver);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mMessenger.getBinder();
    }

    @Override
    public boolean onUnbind(Intent intent) {
        return super.onUnbind(intent);
    }
}