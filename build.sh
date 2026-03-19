#!/bin/bash

set -euxo pipefail

ANDROID_BT_DIR=$ANDROID_HOME/build-tools/36.1.0
ANDROID_P_DIR=$ANDROID_HOME/platforms/android-36.1

AAPT=$ANDROID_BT_DIR/aapt
AAPT2=$ANDROID_BT_DIR/aapt2
D8=$ANDROID_BT_DIR/d8
ZIPALIGN=$ANDROID_BT_DIR/zipalign
APKSIGNER=$ANDROID_BT_DIR/apksigner

NDK_HOME=$ANDROID_HOME/ndk/30.0.14904198
NDK_TOOLCHAIN=$NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin
X86_64_CLANG=$NDK_TOOLCHAIN/x86_64-linux-android21-clang
ARMV7A_CLANG=$NDK_TOOLCHAIN/armv7a-linux-androideabi21-clang
AARCH64_CLANG=$NDK_TOOLCHAIN/aarch64-linux-android21-clang
# ARM_V8A_CLANG= $NDK_HOME/armv8a-linux-androideabi21-clang

# JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64

if [[ -f app.apk ]]; then
    rm app.apk
fi

BUILD_DIR=build
if [[ -d ./$BUILD_DIR ]]; then
    rm -r ./$BUILD_DIR
fi

mkdir ./$BUILD_DIR
cd ./$BUILD_DIR

SRC_DIR=../src

mkdir -p lib/x86_64/
mkdir -p lib/armeabi-v7a
mkdir -p lib/aarch64
$X86_64_CLANG -shared -o lib/x86_64/libmath.so $SRC_DIR/main.c
$ARMV7A_CLANG -shared -o lib/armeabi-v7a/libmath.so $SRC_DIR/main.c
$AARCH64_CLANG -shared -o lib/aarch64/libmath.so $SRC_DIR/main.c
# $ARMV8A_CLANG -shared -o lib/arm64-v8a/libmath.so $SRC_DIR/main.c


$AAPT2 compile -o res.flata \
    ../res/mipmap-hdpi/ic_launcher.png

$AAPT2 link -o app.apk.unaligned \
    -I $ANDROID_P_DIR/android.jar \
    --manifest ../AndroidManifest.xml \
    --java ./java \
    --auto-add-overlay \
    ./res.flata

kotlinc -d classes \
    -cp $ANDROID_P_DIR/android.jar \
    -Xmetadata-version=2.2.0 \
    $SRC_DIR/MainActivity.kt

$D8 --lib $ANDROID_P_DIR/android.jar \
    --output . \
    ./classes/com/example/myapp/*.class

$AAPT add app.apk.unaligned \
    classes.dex \
    lib/armeabi-v7a/libmath.so \
    lib/x86_64/libmath.so \
    lib/aarch64/libmath.so
    # lib/arm64-v8a/libmath.so

$ZIPALIGN -v -p 4 app.apk.unaligned app.apk

$APKSIGNER sign --ks ../my-keystore.jks app.apk

