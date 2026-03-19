package com.example.myapp;

import android.app.Activity
import android.os.Bundle
import android.widget.Button
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.TextView
import android.content.Intent
import android.view.ViewGroup.LayoutParams
import android.view.View

class MainActivity : Activity() {

    init {
        System.loadLibrary("math")
    }

    external fun add(a: Int, b: Int): Int

    private lateinit var editTextName: EditText

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val layout = LinearLayout(this)
        layout.orientation = LinearLayout.VERTICAL
        layout.layoutParams = LayoutParams(
            LayoutParams.MATCH_PARENT,
            LayoutParams.MATCH_PARENT
        )

        val cAnswer = TextView(this)
        val res = add(34, 35)
        cAnswer.text = "Результат сложения из Си: $res"
        cAnswer.textSize = 18f
        layout.addView(cAnswer)

        val label = TextView(this)
        label.text = "Введите ваше имя:"
        label.textSize = 18f
        layout.addView(label)

        editTextName = EditText(this)
        editTextName.hint = "Имя"
        layout.addView(editTextName)

        val buttonOk = Button(this)
        buttonOk.text = "ОК"
        buttonOk.setOnClickListener {
            val name = editTextName.text.toString()
            val intent = Intent(this, SecondActivity::class.java)
            intent.putExtra("USER_NAME", name)
            startActivity(intent)
        }
        layout.addView(buttonOk)

        val buttonCancel = Button(this)
        buttonCancel.text = "Очистить"
        buttonCancel.setOnClickListener {
            editTextName.text.clear()
        }
        layout.addView(buttonCancel)

        setContentView(layout)
    }
}

class SecondActivity : Activity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val layout = LinearLayout(this)
        layout.orientation = LinearLayout.VERTICAL
        layout.layoutParams = LayoutParams(
            LayoutParams.MATCH_PARENT,
            LayoutParams.MATCH_PARENT
        )

        val name = intent.getStringExtra("USER_NAME") ?: "Гость"

        val textView = TextView(this)
        textView.text = "Привет, $name!"
        textView.textSize = 20f
        layout.addView(textView)

        val buttonBack = Button(this)
        buttonBack.text = "Назад"
        buttonBack.setOnClickListener {
            finish()
        }
        layout.addView(buttonBack)

        setContentView(layout)
    }
}
