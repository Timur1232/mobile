package com.example.myapp;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

public class MainActivity extends Activity {
    override protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        var hello = TextView(this);
        hello.setText("Hello from wsl");
        setContentView(hello);
    }
}
