#!/bin/bash

set -euxo pipefail

ANDROID_BT_DIR=$ANDROID_HOME/build-tools/34.0.0
ANDROID_P_DIR=$ANDROID_HOME/platforms/android-36

JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64

if [ -f app.apk ]; then
    rm app.apk
fi

$ANDROID_BT_DIR/aapt2 compile -o res.flata \
    ./res/values/strings.xml

$ANDROID_BT_DIR/aapt2 link -o app.apk.unaligned \
    -I $ANDROID_P_DIR/android.jar \
    --manifest ./AndroidManifest.xml \
    --java . \
    --auto-add-overlay \
    res.flata

$JAVA_HOME/bin/javac -d classes \
    -cp $ANDROID_P_DIR/android.jar \
    ./MainActivity.java

$ANDROID_BT_DIR/d8 --lib $ANDROID_P_DIR/android.jar \
    --output . \
    ./classes/com/example/myapp/*.class

$ANDROID_BT_DIR/aapt add app.apk.unaligned classes.dex

$ANDROID_BT_DIR/zipalign -v -p 4 app.apk.unaligned app.apk

$ANDROID_BT_DIR/apksigner sign --ks my-keystore.jks app.apk

