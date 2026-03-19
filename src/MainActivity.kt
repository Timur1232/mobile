package com.example.myapp;

import android.app.Activity
import android.os.Bundle
import android.widget.Button
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.TextView
import android.widget.CheckBox
import android.content.Intent
import android.content.SharedPreferences
import android.content.Context
import android.view.ViewGroup.LayoutParams
import android.view.View
import android.text.TextWatcher
import android.text.Editable

// TODO: Add session manager class

class MainActivity : Activity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val prefs = getSharedPreferences("user_session", Context.MODE_PRIVATE)
        val isAuthorized = prefs.getBoolean("is_authorized", false)
        if (isAuthorized) {
            val email = prefs.getString("email", null)
            if (email.isNullOrEmpty()) {
                prefs.edit().putBoolean("is_authorized", false).apply()
                runLogin()
            } else {
                val intent = Intent(this, SecondActivity::class.java)
                intent.putExtra("user_email", email)
                startActivity(intent)
            }
        } else {
            runLogin()
        }
        finish()
    }

    fun runLogin() {
        val intent = Intent(this, LoginActivity::class.java)
        startActivity(intent)
    }
}

class LoginActivity : Activity() {
    private lateinit var emailWidget: EditText
    private lateinit var passwordWidget: EditText
    // private lateinit var acceptCheckBox: CheckBox
    // private lateinit var buttonLogin: Button
    private lateinit var buttonCancel: Button
    private lateinit var errorLabel: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val layout = LinearLayout(this)
        layout.orientation = LinearLayout.VERTICAL
        layout.layoutParams = LayoutParams(
            LayoutParams.MATCH_PARENT,
            LayoutParams.MATCH_PARENT
        )

        val label = TextView(this).apply {
            text = "Электронная почта:"
            textSize = 18f
        }
        layout.addView(label)

        emailWidget = EditText(this).apply {
            hint = "example@aboba.com"
        }
        layout.addView(emailWidget)

        passwordWidget = EditText(this).apply {
            hint = "urmom"
        }
        layout.addView(passwordWidget)

        errorLabel = TextView(this).apply {
            text = null
        }
        layout.addView(errorLabel)

        val acceptCheckBox = CheckBox(this).apply {
            text = "Я согласен продать душу дьяволу"
        }
        val buttonLogin = Button(this).apply {
            text = "Войти"
            // isEnabled = false
        }
        // acceptCheckBox.setOnCheckedChangeListener { _, isChecked ->
        //     buttonLogin.isEnabled = isChecked
        // }

        layout.addView(acceptCheckBox)
        layout.addView(buttonLogin)

        val prefs = getSharedPreferences("user_session", Context.MODE_PRIVATE)
        buttonLogin.setOnClickListener {
            if (!acceptCheckBox.isChecked) {
                errorLabel.text = "Нужно подвердить согласие, чтобы продолжить"
            } else {
                val email = emailWidget.text.toString()
                val password = passwordWidget.text.toString()

                if (!email.equals(password)) {
                    errorLabel.text = "Неправильная почта или пароль"
                } else {
                    errorLabel.text = null
                    val editor = prefs.edit()
                    editor.putString("email", email).apply()
                    editor.putBoolean("is_authorized", true).apply()
                    val intent = Intent(this@LoginActivity, SecondActivity::class.java)
                    intent.putExtra("user_email", email)
                    startActivity(intent)
                    finish()
                }
            }
        }

        buttonCancel = Button(this).apply {
            text = "Очистить"
        }
        buttonCancel.setOnClickListener {
            emailWidget.text.clear()
            passwordWidget.text.clear()
            acceptCheckBox.isChecked = false
        }
        layout.addView(buttonCancel)

        setContentView(layout)
    }

    fun checkAccess(email: String, password: String): Boolean {
        return email.equals(password)
    }
}

class SecondActivity : Activity() {

    init {
        System.loadLibrary("math")
    }
    external fun add(a: Int, b: Int): Int

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

        val email = intent.getStringExtra("user_email") ?: "Гость"

        val textView = TextView(this)
        textView.text = "Привет, $email!"
        textView.textSize = 20f
        layout.addView(textView)

        val prefs = getSharedPreferences("user_session", Context.MODE_PRIVATE)
        // val emailPrefs = prefs.getString("email", null)
        // val isAuthorized = prefs.getBoolean("is_authorized", false)
        // val fromPrefs = TextView(this)
        // fromPrefs.text = "Почта из хранилища: $emailPrefs. Авторизация из хранилища: $isAuthorized"
        // fromPrefs.textSize = 20f
        // layout.addView(fromPrefs)

        val buttonBack = Button(this)
        buttonBack.text = "Выйти"
        buttonBack.setOnClickListener {
            prefs.edit().putBoolean("is_authorized", false).apply()
            val intent = Intent(this@SecondActivity, MainActivity::class.java)
            startActivity(intent)
            finish()
        }
        layout.addView(buttonBack)

        setContentView(layout)
    }
}
