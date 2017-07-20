package io.tempage.dorypuppy;

import android.util.Log;

public class DoryPuppy {
    static {
        System.loadLibrary("uv");
        System.loadLibrary("dorypuppy");
    }

    private DoryPuppy() {

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

    public void test(String s) {
        Log.i("____________",s);
    }

    /**
     * A native method that is implemented by the 'dorypuppy' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
    public native int doryTest();
}
