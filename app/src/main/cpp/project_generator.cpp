#include "project_generator.h"
#include "zip_writer.h"
#include <android/log.h>
#include <sstream>
#include <algorithm>
#include <cctype>

#define LOG_TAG "ProjectGen"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static std::string toCamelCase(const std::string& s) {
  std::string result;
  bool cap = true;
  for (char c : s) {
    if (c == ' ' || c == '_' || c == '-') {
      cap = true;
    } else if (cap) {
      result += toupper(c);
      cap = false;
    } else {
      result += c;
    }
  }
  return result;
}

static std::string toUpperCamelCase(const std::string& s) {
  std::string r = toCamelCase(s);
  if (!r.empty()) r[0] = toupper(r[0]);
  return r;
}

static std::string toLowerCamelCase(const std::string& s) {
  std::string r = toCamelCase(s);
  if (!r.empty()) r[0] = tolower(r[0]);
  return r;
}

static std::string packageToPath(const std::string& pkg) {
  std::string path = pkg;
  std::replace(path.begin(), path.end(), '.', '/');
  return path;
}

std::vector<GeneratedFile> ProjectGenerator::generateProject(
    const std::string& appName,
    const std::string& packageName) {

  std::vector<GeneratedFile> files;
  std::string pkgPath = packageToPath(packageName);
  std::string activityName = toUpperCamelCase(appName) + "Activity";
  if (activityName == "Activity") activityName = "MainActivity";

  std::string appNameSnake = appName;
  std::replace(appNameSnake.begin(), appNameSnake.end(), ' ', '_');
  std::transform(appNameSnake.begin(), appNameSnake.end(), appNameSnake.begin(), ::tolower);

  // settings.gradle.kts
  {
    GeneratedFile f;
    f.path = "settings.gradle.kts";
    f.content = R"(pluginManagement {
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

rootProject.name = ")";
    f.content += appNameSnake + "\"\n\ninclude(\":app\")\n";
    files.push_back(f);
  }

  // build.gradle.kts (root)
  {
    GeneratedFile f;
    f.path = "build.gradle.kts";
    f.content = R"(plugins {
  alias(libs.plugins.android.application) apply false
  alias(libs.plugins.kotlin.android) apply false
}

tasks.register<Delete>("clean") {
  delete(rootProject.buildDir)
}
)";
    files.push_back(f);
  }

  // gradle.properties
  {
    GeneratedFile f;
    f.path = "gradle.properties";
    f.content = R"(android.nonTransitiveRClass=true
kotlin.code.style=official
android.useAndroidX=true
org.gradle.jvmargs=-Xmx2048m -Dfile.encoding\=UTF-8
)";
    files.push_back(f);
  }

  // gradle/libs.versions.toml
  {
    GeneratedFile f;
    f.path = "gradle/libs.versions.toml";
    f.content = R"([versions]
agp = "8.13.2"
kotlin = "2.3.0"
coreKtx = "1.17.0"
appcompat = "1.7.1"
material = "1.13.0"
constraintlayout = "2.2.1"

[libraries]
androidx-core-ktx = { module = "androidx.core:core-ktx", version.ref = "coreKtx" }
androidx-appcompat = { module = "androidx.appcompat:appcompat", version.ref = "appcompat" }
material = { module = "com.google.android.material:material", version.ref = "material" }
androidx-constraintlayout = { module = "androidx.constraintlayout:constraintlayout", version.ref = "constraintlayout" }

[plugins]
android-application = { id = "com.android.application", version.ref = "agp" }
android-library = { id = "com.android.library", version.ref = "agp" }
kotlin-android = { id = "org.jetbrains.kotlin.android", version.ref = "kotlin" }
)";
    files.push_back(f);
  }

  // app/build.gradle.kts
  {
    GeneratedFile f;
    f.path = "app/build.gradle.kts";
    f.content = R"(plugins {
  alias(libs.plugins.android.application)
  alias(libs.plugins.kotlin.android)
}

android {
  namespace = ")";
    f.content += packageName + "\n";
    f.content += R"(  compileSdk = 36

  defaultConfig {
    applicationId = ")";
    f.content += packageName + "\"\n";
    f.content += R"(    minSdk = 26
    targetSdk = 35
    versionCode = 1
    versionName = "1.0"
  }

  buildFeatures {
    viewBinding = true
  }

  compileOptions {
    sourceCompatibility = JavaVersion.VERSION_17
    targetCompatibility = JavaVersion.VERSION_17
  }
  kotlin {
    compilerOptions {
      jvmTarget.set(org.jetbrains.kotlin.gradle.dsl.JvmTarget.fromTarget("17"))
    }
  }
}

dependencies {
  implementation(libs.androidx.core.ktx)
  implementation(libs.androidx.appcompat)
  implementation(libs.material)
  implementation(libs.androidx.constraintlayout)
}
)";
    files.push_back(f);
  }

  // AndroidManifest.xml
  {
    GeneratedFile f;
    f.path = "app/src/main/AndroidManifest.xml";
    f.content = R"(<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android">

  <application
    android:allowBackup="true"
    android:label="@string/app_name"
    android:supportsRtl="true"
    android:theme="@style/Theme.)";
    f.content += toUpperCamelCase(appName) + "\"\n";
    f.content += R"(    android:icon="@mipmap/ic_launcher"
    android:roundIcon="@mipmap/ic_launcher_round">

    <activity
      android:name=".)";
    f.content += activityName + "\"\n";
    f.content += R"(      android:exported="true">
      <intent-filter>
        <action android:name="android.intent.action.MAIN" />
        <category android:name="android.intent.category.LAUNCHER" />
      </intent-filter>
    </activity>

  </application>

</manifest>
)";
    files.push_back(f);
  }

  // MainActivity.kt
  {
    GeneratedFile f;
    f.path = "app/src/main/java/" + pkgPath + "/" + activityName + ".kt";
    f.content = "package " + packageName + "\n\n";
    f.content += R"(import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle

class )";
    f.content += activityName + R"( : AppCompatActivity() {
  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    setContentView(R.layout.activity_main)
  }
}
)";
    files.push_back(f);
  }

  // activity_main.xml
  {
    GeneratedFile f;
    f.path = "app/src/main/res/layout/activity_main.xml";
    f.content = R"(<?xml version="1.0" encoding="utf-8"?>
<androidx.constraintlayout.widget.ConstraintLayout
  xmlns:android="http://schemas.android.com/apk/res/android"
  android:layout_width="match_parent"
  android:layout_height="match_parent">

  <TextView
    android:layout_width="wrap_content"
    android:layout_height="wrap_content"
    android:text="Hello from )";
    f.content += appName + "!\"\n";
    f.content += R"(    android:layout_centerInParent="true"
    app:layout_constraintBottom_toBottomOf="parent"
    app:layout_constraintEnd_toEndOf="parent"
    app:layout_constraintStart_toStartOf="parent"
    app:layout_constraintTop_toTopOf="parent" />

</androidx.constraintlayout.widget.ConstraintLayout>
)";
    files.push_back(f);
  }

  // colors.xml
  {
    GeneratedFile f;
    f.path = "app/src/main/res/values/colors.xml";
    f.content = R"(<?xml version="1.0" encoding="utf-8"?>
<resources>
  <color name="purple_primary">#7C3AED</color>
  <color name="purple_dark">#5B21B6</color>
  <color name="purple_light">#A78BFA</color>
  <color name="black">#FF000000</color>
  <color name="white">#FFFFFFFF</color>
  <color name="background">#121212</color>
  <color name="surface">#1E1E2E</color>
  <color name="on_surface">#E0E0E0</color>
</resources>
)";
    files.push_back(f);
  }

  // themes.xml
  {
    GeneratedFile f;
    f.path = "app/src/main/res/values/themes.xml";
    f.content = R"(<?xml version="1.0" encoding="utf-8"?>
<resources xmlns:tools="http://schemas.android.com/tools">
  <style name="Theme.)";
    f.content += toUpperCamelCase(appName) + "\" parent=\"Theme.Material3.Dark.NoActionBar\">\n";
    f.content += R"(    <item name="colorPrimary">@color/purple_primary</item>
    <item name="colorPrimaryDark">@color/purple_dark</item>
    <item name="colorAccent">@color/purple_light</item>
    <item name="android:colorBackground">@color/background</item>
    <item name="colorSurface">@color/surface</item>
    <item name="colorOnSurface">@color/on_surface</item>
  </style>
</resources>
)";
    files.push_back(f);
  }

  // strings.xml
  {
    GeneratedFile f;
    f.path = "app/src/main/res/values/strings.xml";
    f.content = R"(<?xml version="1.0" encoding="utf-8"?>
<resources>
  <string name="app_name">)" + appName + R"(</string>
</resources>
)";
    files.push_back(f);
  }

  // proguard-rules.pro
  {
    GeneratedFile f;
    f.path = "app/proguard-rules.pro";
    f.content = "# ProGuard rules for " + appName + "\n";
    files.push_back(f);
  }

  // gradlew (placeholder script)
  {
    GeneratedFile f;
    f.path = "gradlew";
    f.content = R"(#!/bin/sh
# Gradle wrapper script placeholder
echo "Please download Gradle wrapper to use this project."
)";
    files.push_back(f);
  }

  // gradlew.bat
  {
    GeneratedFile f;
    f.path = "gradlew.bat";
    f.content = R"(@echo off
REM Gradle wrapper placeholder
echo Please download Gradle wrapper to use this project.
)";
    files.push_back(f);
  }

  // gradle/wrapper/gradle-wrapper.properties
  {
    GeneratedFile f;
    f.path = "gradle/wrapper/gradle-wrapper.properties";
    f.content = R"(distributionBase=GRADLE_USER_HOME
distributionPath=wrapper/dists
distributionUrl=https\://services.gradle.org/distributions/gradle-8.10.2-bin.zip
networkTimeout=10000
validateDistributionUrl=true
zipStoreBase=GRADLE_USER_HOME
zipStorePath=wrapper/dists
)";
    files.push_back(f);
  }

  // .gitignore
  {
    GeneratedFile f;
    f.path = ".gitignore";
    f.content = R"(*.iml
.gradle
/local.properties
/.idea
.DS_Store
/build
/captures
.externalNativeBuild
.cxx
local.properties
)";
    files.push_back(f);
  }

  return files;
}

std::string ProjectGenerator::getVersion() {
  return "1.0";
}

std::string ProjectGenerator::getProjectStructure() {
  return R"(Project Structure:
├── build.gradle.kts
├── settings.gradle.kts
├── gradle.properties
├── gradle/
│   ├── libs.versions.toml
│   └── wrapper/
│       └── gradle-wrapper.properties
├── gradlew
├── gradlew.bat
├── .gitignore
└── app/
    ├── build.gradle.kts
    ├── proguard-rules.pro
    └── src/
        └── main/
            ├── AndroidManifest.xml
            ├── java/[package]/
            │   └── [AppName]Activity.kt
            └── res/
                ├── layout/
                │   └── activity_main.xml
                └── values/
                    ├── colors.xml
                    ├── strings.xml
                    └── themes.xml)";
}

// JNI functions
extern "C" {

JNIEXPORT jstring JNICALL
Java_my_company_cgame_MainActivity_nativeGetProjectStructure(JNIEnv* env, jobject /*thiz*/) {
  std::string s = ProjectGenerator::getProjectStructure();
  return env->NewStringUTF(s.c_str());
}

JNIEXPORT jstring JNICALL
Java_my_company_cgame_MainActivity_nativeGetVersion(JNIEnv* env, jobject /*thiz*/) {
  std::string v = ProjectGenerator::getVersion();
  return env->NewStringUTF(v.c_str());
}

JNIEXPORT jboolean JNICALL
Java_my_company_cgame_MainActivity_nativeGenerateProject(JNIEnv* env, jobject /*thiz*/,
                                                          jstring appName,
                                                          jstring packageName,
                                                          jstring outputPath) {
  const char* appNameStr = env->GetStringUTFChars(appName, nullptr);
  const char* pkgStr = env->GetStringUTFChars(packageName, nullptr);
  const char* outPath = env->GetStringUTFChars(outputPath, nullptr);

  if (!appNameStr || !pkgStr || !outPath) {
    if (appNameStr) env->ReleaseStringUTFChars(appName, appNameStr);
    if (pkgStr) env->ReleaseStringUTFChars(packageName, pkgStr);
    if (outPath) env->ReleaseStringUTFChars(outputPath, outPath);
    return JNI_FALSE;
  }

  std::string appNameCpp(appNameStr);
  std::string pkgCpp(pkgStr);
  std::string outCpp(outPath);

  env->ReleaseStringUTFChars(appName, appNameStr);
  env->ReleaseStringUTFChars(packageName, pkgStr);
  env->ReleaseStringUTFChars(outputPath, outPath);

  auto files = ProjectGenerator::generateProject(appNameCpp, pkgCpp);

  std::string appNameSnakeLC = appNameCpp;
  std::replace(appNameSnakeLC.begin(), appNameSnakeLC.end(), ' ', '_');
  std::transform(appNameSnakeLC.begin(), appNameSnakeLC.end(), appNameSnakeLC.begin(), ::tolower);

  std::vector<std::pair<std::string, std::string>> filePairs;
  for (const auto& f : files) {
    filePairs.emplace_back(f.path, f.content);
  }

  std::string zipPath = outCpp + "/" + appNameSnakeLC + "_project.zip";
  bool ok = createZip(zipPath, filePairs, appNameSnakeLC);
  return ok ? JNI_TRUE : JNI_FALSE;
}

} // extern "C"
