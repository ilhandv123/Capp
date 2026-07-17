<p align="center">
  <img src="https://github.com/ilhandv123/Capp/raw/main/app/src/main/res/mipmap-xxxhdpi/ic_launcher.webp" width="96" alt="Capp logo">
</p>

<h1 align="center">Capp — C++ Android Project Generator</h1>

<p align="center">
  <a href="https://github.com/ilhandv123/Capp/releases/latest">
    <img src="https://img.shields.io/github/v/release/ilhandv123/Capp?label=Release&color=3DDC84&style=flat-square" alt="Release">
  </a>
  <a href="https://github.com/ilhandv123/Capp/releases/download/Capp/Capp.1.0.apk">
    <img src="https://img.shields.io/badge/Download-APK-006D3B?style=flat-square&logo=android" alt="Download APK">
  </a>
</p>

A Material 3 Android app that generates ready-to-build Android Studio projects with **NDK 27** and **CMake** support.

## Features

- Generates a complete Android project ZIP with JNI boilerplate
- NDK 27.0 BETA + CMake 3.22.1 integration (arm64-v8a, armeabi-v7a, x86_64)
- Customizable app name, package name, minSdk, and targetSdk
- Adaptive launcher icon included
- Material 3 design with Android Green theme (light & dark)
- Exports directly to device storage — no PC required

## Usage

1. Enter your app name and package name
2. Set minSdk / targetSdk (defaults: 26 / 36)
3. Tap **Generate & Export**
4. Transfer the ZIP to a computer and open in Android Studio

## Generated Project

```
myapp/
├── app/
│   ├── src/main/
│   │   ├── java/[package]/
│   │   │   └── [App]Activity.kt
│   │   ├── cpp/
│   │   │   ├── CMakeLists.txt
│   │   │   └── native-lib.cpp
│   │   ├── res/
│   │   └── AndroidManifest.xml
│   └── build.gradle.kts
├── gradle/
├── build.gradle.kts
├── settings.gradle.kts
└── gradle.properties
```

## Download

[<img src="https://img.shields.io/badge/Download_Latest_APK-006D3B?style=for-the-badge&logo=android&logoColor=white">](https://github.com/ilhandv123/Capp/releases/download/Capp/Capp.1.0.apk)

## Requirements

- Android 7.0+ (API 24)
- Storage permission (Android < 11) for ZIP export

## Tech Stack

- **Kotlin** — app logic
- **Material 3** — UI components & theming
- **NDK 27** — C++ native library in generated projects
- **Gradle 9.1 / AGP 8.13** — generated project build
