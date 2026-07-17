package com.ilhan.capp

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import com.google.android.material.button.MaterialButton
import com.google.android.material.textfield.TextInputEditText
import com.google.android.material.progressindicator.LinearProgressIndicator
import android.widget.TextView
import android.widget.LinearLayout
import java.io.File
import java.io.FileOutputStream
import java.util.zip.ZipEntry
import java.util.zip.ZipOutputStream

class MainActivity : AppCompatActivity() {

  private lateinit var appNameInput: TextInputEditText
  private lateinit var packageNameInput: TextInputEditText
  private lateinit var minSdkInput: TextInputEditText
  private lateinit var targetSdkInput: TextInputEditText
  private lateinit var generateBtn: MaterialButton
  private lateinit var progressLayout: LinearLayout
  private lateinit var statusText: TextView

  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    setContentView(R.layout.activity_main)
    WindowCompat.setDecorFitsSystemWindows(window, false)

    appNameInput = findViewById(R.id.appNameInput)
    packageNameInput = findViewById(R.id.packageNameInput)
    minSdkInput = findViewById(R.id.minSdkInput)
    targetSdkInput = findViewById(R.id.targetSdkInput)
    generateBtn = findViewById(R.id.generateBtn)
    progressLayout = findViewById(R.id.progressLayout)
    statusText = findViewById(R.id.statusText)

    ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.root)) { view, insets ->
      val statusBar = insets.getInsets(WindowInsetsCompat.Type.statusBars())
      val navBar = insets.getInsets(WindowInsetsCompat.Type.navigationBars())
      view.setPadding(statusBar.left, statusBar.top, statusBar.right, navBar.bottom)
      WindowInsetsCompat.CONSUMED
    }

    generateBtn.setOnClickListener { onGenerateClick() }
  }

  private fun onGenerateClick() {
    val appName = appNameInput.text?.toString()?.trim() ?: ""
    val packageName = packageNameInput.text?.toString()?.trim() ?: ""
    val minSdk = minSdkInput.text?.toString()?.trim() ?: ""
    val targetSdk = targetSdkInput.text?.toString()?.trim() ?: ""

    if (appName.isEmpty()) {
      appNameInput.error = "Enter app name"
      return
    }
    if (appName.length > 50) {
      appNameInput.error = "Max 50 characters"
      return
    }
    if (!appName.matches(Regex("^[a-zA-Z][a-zA-Z0-9_ ]*$"))) {
      appNameInput.error = "Letters, numbers, spaces and underscores only"
      return
    }
    if (packageName.isEmpty()) {
      packageNameInput.error = "Enter package name"
      return
    }
    if (packageName.length > 100) {
      packageNameInput.error = "Max 100 characters"
      return
    }
    if (!packageName.matches(Regex("^[a-z][a-z0-9]*(\\.[a-z][a-z0-9]*)*$"))) {
      packageNameInput.error = "e.g. com.example.myapp"
      return
    }
    if (packageName.startsWith(".") || packageName.endsWith(".")) {
      packageNameInput.error = "Cannot start/end with dot"
      return
    }
    if (minSdk.isEmpty() || minSdk.toIntOrNull() == null) {
      minSdkInput.error = "Enter a valid number"
      return
    }
    if (targetSdk.isEmpty() || targetSdk.toIntOrNull() == null) {
      targetSdkInput.error = "Enter a valid number"
      return
    }

    checkPermissionAndExport(appName, packageName)
  }

  private fun checkPermissionAndExport(appName: String, packageName: String) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
      exportProject(appName, packageName)
      return
    }

    val perm = ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE)
    if (perm != PackageManager.PERMISSION_GRANTED) {
      ActivityCompat.requestPermissions(
        this,
        arrayOf(Manifest.permission.WRITE_EXTERNAL_STORAGE),
        PERMISSION_REQUEST_CODE
      )
      return
    }
    exportProject(appName, packageName)
  }

  override fun onRequestPermissionsResult(
    requestCode: Int, permissions: Array<String>, grantResults: IntArray
  ) {
    super.onRequestPermissionsResult(requestCode, permissions, grantResults)
    if (requestCode == PERMISSION_REQUEST_CODE) {
      if (grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
        val appName = appNameInput.text?.toString()?.trim() ?: ""
        val packageName = packageNameInput.text?.toString()?.trim() ?: ""
        exportProject(appName, packageName)
      } else {
        Toast.makeText(this, "Permission denied", Toast.LENGTH_SHORT).show()
      }
    }
  }

  private fun exportProject(appName: String, packageName: String) {
    val minSdk = minSdkInput.text?.toString()?.trim() ?: "26"
    val targetSdk = targetSdkInput.text?.toString()?.trim() ?: "36"

    progressLayout.visibility = LinearLayout.VISIBLE
    statusText.text = getString(R.string.generating)
    generateBtn.isEnabled = false

    val docsDir = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
      Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOCUMENTS)
    } else {
      Environment.getExternalStorageDirectory()
    }
    if (!docsDir.exists()) docsDir.mkdirs()

    Thread {
      val result = generateProjectZip(appName, packageName, minSdk, targetSdk, docsDir.absolutePath)

      runOnUiThread {
        progressLayout.visibility = LinearLayout.GONE
        generateBtn.isEnabled = true

        if (result) {
          val appNameSnake = appName.lowercase().replace(" ", "_")
          val zipPath = File(docsDir, "${appNameSnake}_project.zip").absolutePath
          Toast.makeText(
            this,
            "${getString(R.string.export_success)}\n$zipPath",
            Toast.LENGTH_LONG
          ).show()
        } else {
          Toast.makeText(this, R.string.export_failed, Toast.LENGTH_SHORT).show()
        }
      }
    }.start()
  }

  private fun generateProjectZip(appName: String, packageName: String, minSdk: String, targetSdk: String, outputDir: String): Boolean {
    return try {
      val appNameSnake = appName.lowercase().replace(" ", "_")
      val zipFile = File(outputDir, "${appNameSnake}_project.zip")
      val files = generateProjectFiles(appName, packageName, minSdk, targetSdk)
      val baseDir = appNameSnake

      ZipOutputStream(FileOutputStream(zipFile)).use { zos ->
        for ((path, content) in files) {
          val entryName = "$baseDir/$path"
          zos.putNextEntry(ZipEntry(entryName))
          zos.write(content.toByteArray(Charsets.UTF_8))
          zos.closeEntry()
        }
      }
      true
    } catch (e: Exception) {
      e.printStackTrace()
      false
    }
  }

  private fun generateProjectFiles(appName: String, packageName: String, minSdk: String = "26", targetSdk: String = "35"): List<Pair<String, String>> {
    val files = mutableListOf<Pair<String, String>>()
    val appNameSnake = appName.lowercase().replace(" ", "_")
    val pkgPath = packageName.replace('.', '/')
    val appClass = appName.replace(" ", "").replaceFirstChar { it.uppercase() } + "Activity"
    val activityName = if (appClass == "Activity") "MainActivity" else appClass
    val themeName = "Theme" + appName.replace(" ", "").replaceFirstChar { it.uppercase() }

    files.add("settings.gradle.kts" to """
pluginManagement {
  repositories {
    google()
    mavenCentral()
  }
}
dependencyResolutionManagement {
  repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
  repositories {
    google()
    mavenCentral()
  }
}
rootProject.name = "$appNameSnake"
include(":app")
""".trimIndent())

    files.add("build.gradle.kts" to """
plugins {
  alias(libs.plugins.android.application) apply false
  alias(libs.plugins.kotlin.android) apply false
}
tasks.register<Delete>("clean") {
  delete(rootProject.buildDir)
}
""".trimIndent())

    files.add("gradle.properties" to """
android.nonTransitiveRClass=true
kotlin.code.style=official
android.useAndroidX=true
org.gradle.jvmargs=-Xmx2048m -Dfile.encoding\=UTF-8
""".trimIndent())

    files.add("gradle/libs.versions.toml" to """
[versions]
agp = "8.13.2"
kotlin = "2.3.0"

[plugins]
android-application = { id = "com.android.application", version.ref = "agp" }
android-library = { id = "com.android.library", version.ref = "agp" }
kotlin-android = { id = "org.jetbrains.kotlin.android", version.ref = "kotlin" }
""".trimIndent())

    files.add("app/build.gradle.kts" to """
plugins {
  alias(libs.plugins.android.application)
  alias(libs.plugins.kotlin.android)
}
android {
  namespace = "$packageName"
  compileSdk = 36
  defaultConfig {
    applicationId = "$packageName"
    minSdk = $minSdk
    targetSdk = $targetSdk
    versionCode = 1
    versionName = "1.0"
    ndk {
      abiFilters += listOf("arm64-v8a", "armeabi-v7a", "x86_64")
    }
  }
  buildFeatures { viewBinding = true }
  compileOptions {
    sourceCompatibility = JavaVersion.VERSION_17
    targetCompatibility = JavaVersion.VERSION_17
  }
  kotlin {
    compilerOptions { jvmTarget.set(org.jetbrains.kotlin.gradle.dsl.JvmTarget.fromTarget("17")) }
  }
  externalNativeBuild {
    cmake {
      path = file("src/main/cpp/CMakeLists.txt")
      version = "3.22.1"
    }
  }
}
dependencies {
  implementation("androidx.core:core-ktx:+")
  implementation("androidx.appcompat:appcompat:+")
  implementation("com.google.android.material:material:+")
  implementation("androidx.constraintlayout:constraintlayout:+")
}
""".trimIndent())

    files.add("app/src/main/cpp/CMakeLists.txt" to """
cmake_minimum_required(VERSION 3.22.1)
project("$appNameSnake")

add_library(
  native-lib
  SHARED
  native-lib.cpp
)

find_library(log-lib log)

target_link_libraries(
  native-lib
  ${'$'}{log-lib}
)
""".trimIndent())

    val jniFuncName = "Java_${packageName.replace('.', '_')}_${activityName}_stringFromJNI"

    files.add("app/src/main/cpp/native-lib.cpp" to """
#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring JNICALL
$jniFuncName(JNIEnv* env, jobject /*thiz*/) {
  std::string hello = "Hello from C++ NDK 27.0.12077973!";
  return env->NewStringUTF(hello.c_str());
}
""".trimIndent())

    files.add("app/src/main/AndroidManifest.xml" to """<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android">
  <application
    android:allowBackup="true"
    android:label="@string/app_name"
    android:supportsRtl="true"
    android:theme="@style/$themeName"
    android:icon="@mipmap/ic_launcher"
    android:roundIcon="@mipmap/ic_launcher_round">
    <activity android:name=".$activityName" android:exported="true">
      <intent-filter>
        <action android:name="android.intent.action.MAIN" />
        <category android:name="android.intent.category.LAUNCHER" />
      </intent-filter>
    </activity>
  </application>
</manifest>
""".trimIndent())

    files.add("app/src/main/java/$pkgPath/$activityName.kt" to """
package $packageName
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.TextView
class $activityName : AppCompatActivity() {
  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    setContentView(R.layout.activity_main)
    findViewById<TextView>(R.id.helloText).text = stringFromJNI()
  }
  external fun stringFromJNI(): String
  companion object {
    init { System.loadLibrary("native-lib") }
  }
}
""".trimIndent())

    files.add("app/src/main/res/layout/activity_main.xml" to """<?xml version="1.0" encoding="utf-8"?>
<androidx.constraintlayout.widget.ConstraintLayout
  xmlns:android="http://schemas.android.com/apk/res/android"
  android:layout_width="match_parent"
  android:layout_height="match_parent">
  <TextView
    android:id="@+id/helloText"
    android:layout_width="wrap_content"
    android:layout_height="wrap_content"
    android:text="Loading..."
    android:textSize="18sp"
    app:layout_constraintBottom_toBottomOf="parent"
    app:layout_constraintEnd_toEndOf="parent"
    app:layout_constraintStart_toStartOf="parent"
    app:layout_constraintTop_toTopOf="parent" />
</androidx.constraintlayout.widget.ConstraintLayout>
""".trimIndent())

    files.add("app/src/main/res/values/colors.xml" to """<?xml version="1.0" encoding="utf-8"?>
<resources>
  <color name="primary">#006D3B</color>
  <color name="on_primary">#FFFFFF</color>
  <color name="primary_container">#3DDC84</color>
  <color name="on_primary_container">#005C31</color>
  <color name="background">#F8F9FA</color>
  <color name="surface">#F8F9FA</color>
  <color name="on_surface">#191C1D</color>
  <color name="surface_variant">#E1E3E4</color>
  <color name="on_surface_variant">#3C4A3F</color>
  <color name="outline">#6C7B6E</color>
  <color name="outline_variant">#BBCBBC</color>
  <color name="white">#FFFFFF</color>
  <color name="black">#000000</color>
</resources>
""".trimIndent())

    files.add("app/src/main/res/values/themes.xml" to """<?xml version="1.0" encoding="utf-8"?>
<resources xmlns:tools="http://schemas.android.com/tools">
  <style name="$themeName" parent="Theme.Material3.Light.NoActionBar">
    <item name="colorPrimary">@color/primary</item>
    <item name="colorOnPrimary">@color/on_primary</item>
    <item name="colorPrimaryContainer">@color/primary_container</item>
    <item name="colorOnPrimaryContainer">@color/on_primary_container</item>
    <item name="android:colorBackground">@color/background</item>
    <item name="colorOnBackground">@color/on_surface</item>
    <item name="colorSurface">@color/surface</item>
    <item name="colorOnSurface">@color/on_surface</item>
    <item name="colorOutline">@color/outline</item>
    <item name="colorOutlineVariant">@color/outline_variant</item>
  </style>
</resources>
""".trimIndent())

    files.add("app/src/main/res/values/strings.xml" to """<?xml version="1.0" encoding="utf-8"?>
<resources>
  <string name="app_name">$appName</string>
</resources>
""".trimIndent())

    files.add("app/proguard-rules.pro" to "# ProGuard rules for $appName\n")
    files.add("gradlew" to "#!/bin/sh\necho \"Please download Gradle wrapper to use this project.\"\n")
    files.add("gradlew.bat" to "@echo off\nREM Gradle wrapper placeholder\necho Please download Gradle wrapper to use this project.\n")

    files.add("gradle/wrapper/gradle-wrapper.properties" to """
distributionBase=GRADLE_USER_HOME
distributionPath=wrapper/dists
distributionUrl=https\://services.gradle.org/distributions/gradle-9.1.0-bin.zip
networkTimeout=10000
validateDistributionUrl=true
zipStoreBase=GRADLE_USER_HOME
zipStorePath=wrapper/dists
""".trimIndent())

    files.add(".gitignore" to """
*.iml
.gradle
/local.properties
/.idea
.DS_Store
/build
/captures
.externalNativeBuild
.cxx
local.properties
""".trimIndent())

    files.add("app/src/main/res/drawable/ic_launcher_background.xml" to """<?xml version="1.0" encoding="utf-8"?>
<shape xmlns:android="http://schemas.android.com/apk/res/android">
  <solid android:color="#3DDC84" />
</shape>
""".trimIndent())

    files.add("app/src/main/res/drawable/ic_launcher_foreground.xml" to """<?xml version="1.0" encoding="utf-8"?>
<vector xmlns:android="http://schemas.android.com/apk/res/android"
  android:width="108dp"
  android:height="108dp"
  android:viewportWidth="108"
  android:viewportHeight="108">
  <path
    android:fillColor="#006D3B"
    android:pathData="M54,28c-3.5,0 -6.5,2 -8,5c-0.5,1 -0.5,2 0,3c0.5,1 1.5,1.5 2.5,2c-0.5,2 -0.5,4 0.5,5.5l5,-3l5,3c1,-1.5 1,-3.5 0.5,-5.5c1,-0.5 2,-1 2.5,-2c0.5,-1 0.5,-2 0,-3c-1.5,-3 -4.5,-5 -8,-5z" />
</vector>
""".trimIndent())

    files.add("app/src/main/res/mipmap-anydpi-v26/ic_launcher.xml" to """<?xml version="1.0" encoding="utf-8"?>
<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">
  <background android:drawable="@drawable/ic_launcher_background" />
  <foreground android:drawable="@drawable/ic_launcher_foreground" />
</adaptive-icon>
""".trimIndent())

    files.add("app/src/main/res/mipmap-anydpi-v26/ic_launcher_round.xml" to """<?xml version="1.0" encoding="utf-8"?>
<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">
  <background android:drawable="@drawable/ic_launcher_background" />
  <foreground android:drawable="@drawable/ic_launcher_foreground" />
</adaptive-icon>
""".trimIndent())

    return files
  }

  private fun getProjectStructure(): String {
    return """├── . (NeonEngine)
│   ├── app/
│   │   ├── cpp/
│   │   │   ├── CMakeLists.txt
│   │   │   └── native-lib.cpp
│   │   ├── java/[package]/
│   │   │   └── [App]Activity.kt
│   │   └── res/
│   ├── gradle/
│   ├── build.gradle.kts
│   ├── settings.gradle.kts
│   └── gradle.properties

Hierarchy validated: 24 source files identified."""
  }

  companion object {
    private const val PERMISSION_REQUEST_CODE = 100
  }
}
