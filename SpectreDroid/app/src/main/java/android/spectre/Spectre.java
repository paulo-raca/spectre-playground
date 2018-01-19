package android.spectre;

import android.util.Log;

/**
 * Created by paulo on 18/01/18.
 */

public class Spectre {
    public static final String TAG = Spectre.class.getSimpleName();

    static {
        System.loadLibrary("spectre");
        Log.i(TAG, "Native library loaded");
    }


    public static int calibrateTiming() {
        return calibrateTiming(64);
    }
    public static int calibrateTiming(int count) {
        return calibrateTiming(count, null, null);
    }
    public static native int calibrateTiming(int count, int[] hit_times, int[] miss_times);


    public static native void read(byte[] data, Callback callback);

    public interface Callback {
        public void onByte(int offset, long pointer, byte bestGuess, int bestScore, byte secondGuess, int secondScore);
    }
}
