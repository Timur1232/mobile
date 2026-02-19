#!/bin/fish

set -l ANDROID_BT_DIR $HOME/Android/Sdk/build-tools/36.1.0
set -l ANDROID_P_DIR $HOME/Android/Sdk/platforms/android-36.1

rm app.apk

$ANDROID_BT_DIR/aapt2 compile -o res.flata \
    ./res/values/strings.xml

$ANDROID_BT_DIR/aapt2 link -o app.apk.unaligned \
    -I $ANDROID_P_DIR/android.jar \
    --manifest ./AndroidManifest.xml \
    --java . \
    --auto-add-overlay \
    res.flata

javac -d classes \
    -cp $ANDROID_P_DIR/android.jar \
    ./MainActivity.java

$ANDROID_BT_DIR/d8 --lib $ANDROID_P_DIR/android.jar \
    --output . \
    ./classes/com/example/myapp/*.class

$ANDROID_BT_DIR/aapt add app.apk.unaligned classes.dex

$ANDROID_BT_DIR/zipalign -v -p 4 app.apk.unaligned app.apk

$ANDROID_BT_DIR/apksigner sign --ks my-keystore.jks app.apk
