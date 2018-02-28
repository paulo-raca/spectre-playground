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

    public enum Variant {
        BoundsCheckBypass("v1 -- Spectre" ),
        FunctionsBoundsCheckBypass("v1b -- Function Array"),
        BranchTargetInjection("v2 -- Branch Target Injection"),
        RogueDataCacheLoad("v3 -- Meltdown"),
        BoundsCheckBypassKernel("Kernel v1 -- Spectre" ),
        FunctionsBoundsCheckBypassKernel("Kernel v1b -- Function Array" ),
        DirectAccess("Direct Memory Access");

        public final String description;

        Variant(String description) {
            this.description = description;
        }

        public int value() {
            return 1<<this.ordinal();
        }
        public static int value(Variant... variants) {
            int variantFlags = 0;
            for (Variant v : variants)
                variantFlags |= 1 << v.ordinal();
            return variantFlags;
        }
    };

    public static int calibrateTiming() {
        return calibrateTiming(64);
    }
    public static int calibrateTiming(int count) {
        return calibrateTiming(count, null, null);
    }
    public static native int calibrateTiming(int count, int[] hit_times, int[] miss_times);


    public static native void readPtr(long address, int length, Callback callback, int variant);
    public static native void readBuf(byte[] data, Callback callback, int variant);

    public static void read(long ptr, int len, Callback callback, Variant... variants) {
        readPtr(ptr, len, callback, Variant.value(variants));
    }
    public static void read(byte[] data, Callback callback, Variant... variants) {
        readBuf(data, callback, Variant.value(variants));
    }

    public interface Callback {
        public void onByte(int offset, long pointer, byte bestGuess, int bestScore, byte secondGuess, int secondScore);
    }
}
