#define NOB_IMPLEMENTATION
#include "nob.h"

// SDK paths
const char* android_home;
const char* android_bt_dir;
const char* android_p_dir;

// SDK build tools
const char* aapt;
const char* aapt2;
const char* d8;
const char* zipalign;
const char* apksigner;

// NDK paths
const char* ndk_home;
const char* ndk_toolchain;

// NDK compilers
const char* x86_64_clang;
const char* armv7a_clang;
const char* aarch64_clang;

// Working directories
#define BUILD_DIR "build"
const char* src_dir;
const char* csrc_dir;
const char* prj_dir;

void get_paths()
{
    android_home = getenv("ANDROID_HOME");
    if (android_home == NULL) android_home = "/home/timur/dev/thirdparty/android/sdk";

    android_bt_dir = temp_sprintf("%s/build-tools/36.1.0", android_home);
    android_p_dir  = temp_sprintf("%s/platforms/android-36.1", android_home);

    aapt      = temp_sprintf("%s/aapt", android_bt_dir);
    aapt2     = temp_sprintf("%s/aapt2", android_bt_dir);
    d8        = temp_sprintf("%s/d8", android_bt_dir);
    zipalign  = temp_sprintf("%s/zipalign", android_bt_dir);
    apksigner = temp_sprintf("%s/apksigner", android_bt_dir);

    ndk_home      = temp_sprintf("%s/ndk/30.0.14904198", android_home);
    ndk_toolchain = temp_sprintf("%s/toolchains/llvm/prebuilt/linux-x86_64/bin", ndk_home);
    x86_64_clang  = temp_sprintf("%s/x86_64-linux-android21-clang", ndk_toolchain);
    armv7a_clang  = temp_sprintf("%s/armv7a-linux-androideabi21-clang", ndk_toolchain);
    aarch64_clang = temp_sprintf("%s/aarch64-linux-android21-clang", ndk_toolchain);

    prj_dir = get_current_dir_temp();
    src_dir = temp_sprintf("%s/src", prj_dir);
    csrc_dir = temp_sprintf("%s/csrc", prj_dir);
}

// TODO: make incremental compilation with `nob_needs_rebuild` instead of full recompiling
bool clear_build_files(Cmd* cmd)
{
    Dir_Entry dir = {0};
    if (dir_entry_open(BUILD_DIR, &dir)) {
        dir_entry_close(dir);
        cmd_append(cmd, "rm", "-r", BUILD_DIR);
        if (!cmd_run(cmd)) {
            nob_log(NOB_ERROR, "Unable to delete build directory");
            return false;
        }
        cmd_append(cmd, "mkdir", BUILD_DIR);
        if (!cmd_run(cmd)) {
            nob_log(NOB_ERROR, "Unable to create build directory");
            return false;
        }
        nob_log(NOB_INFO, "Build files cleared");
    }
    return true;
}

bool compile_c_library(Cmd* cmd, const char* target, const char* compiler, const char* so_name, const char** source_files, size_t source_files_count)
{
    // Create directory for specified target
    cmd_append(cmd, "mkdir", "-p");
    cmd_append(cmd, temp_sprintf("lib/%s", target));
    if (!cmd_run(cmd)) {
        nob_log(NOB_ERROR, "Unable to create directory for target %s", target);
        return false;
    }

    // Compile C code
    cmd_append(cmd, compiler);
    cmd_append(cmd, "-shared");
    cmd_append(cmd, "-o", temp_sprintf("lib/%s/lib%s.so", target, so_name));
    for (size_t i = 0; i < source_files_count; i++) {
        cmd_append(cmd, temp_sprintf("%s/%s", csrc_dir, source_files[i]));
    }

    if (!cmd_run(cmd)) {
        nob_log(NOB_ERROR, "Unable to compile files for target %s", target);
        return false;
    }

    return true;
}

typedef struct {
    Cmd* cmd;
    const char* ext;
} OnFileData;

bool on_file(Walk_Entry entry)
{
    OnFileData* data = entry.data;
    if (entry.type == NOB_FILE_REGULAR) {
        String_View name = sv_from_cstr(entry.path);
        if (sv_ends_with_cstr(name, data->ext)) {
            cmd_append(data->cmd, temp_strdup(entry.path));
        }
    }
    return true;
}

bool cmd_append_by_extention(Cmd* cmd, const char* dir, const char* ext)
{
    OnFileData data = {
        .cmd = cmd,
        .ext = ext,
    };
    return walk_dir(dir, on_file, .post_order = false, .data = &data);
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    get_paths();

    // TODO: extract each step in its own function

    Cmd cmd = {0};

    // Generating keystore for signing
    const char* keystore = "keystore.jks";
    if (!file_exists(keystore)) {
        cmd_append(&cmd, "keytool");
        cmd_append(&cmd, "-genkey", "-v");
        cmd_append(&cmd, "-keystore", keystore);
        cmd_append(&cmd, "-alias", "my-alias");
        cmd_append(&cmd, "-keyalg", "RSA");
        cmd_append(&cmd, "-keysize", "2048");
        cmd_append(&cmd, "-validity", "10000");
        if (!cmd_run(&cmd)) return 1;
    }

    clear_build_files(&cmd);

    if (!set_current_dir(BUILD_DIR)) return 1;

    const char* c_src[] = {
        "math.c",
    };
    const char* targets[] = {
        "x86_64",
        "armeabi-v7a",
        "aarch64",
    };
    const char* compilers[] = {
        x86_64_clang,
        armv7a_clang,
        aarch64_clang,
    };
    NOB_ASSERT(ARRAY_LEN(targets) == ARRAY_LEN(compilers));
    const char* so_name = "math";

    for (size_t i = 0; i < ARRAY_LEN(targets); i++) {
        compile_c_library(&cmd, targets[i], compilers[i], so_name, c_src, ARRAY_LEN(c_src));
    }

    // Resources compilation
    cmd_append(&cmd, aapt2, "compile");
    cmd_append(&cmd, "-o", "res.flata");
    cmd_append(&cmd, temp_sprintf("%s/res/mipmap-hdpi/ic_launcher.png", prj_dir));
    if (!cmd_run(&cmd)) return 1;

    // Unaligned apk linking
    cmd_append(&cmd, aapt2, "link");
    cmd_append(&cmd, "-o", "app.apk.unaligned");
    cmd_append(&cmd, "-I", temp_sprintf("%s/android.jar", android_p_dir));
    cmd_append(&cmd, "--manifest", temp_sprintf("%s/AndroidManifest.xml", prj_dir));
    cmd_append(&cmd, "--java", "java");
    cmd_append(&cmd, "--auto-add-overlay");
    cmd_append(&cmd, "res.flata");
    if (!cmd_run(&cmd)) return 1;

    const char* kotlin_src[] = {
        "MainActivity.kt",
    };

    // Compile kotlin code
    cmd_append(&cmd, "kotlinc");
    cmd_append(&cmd, "-d", "classes");
    cmd_append(&cmd, "-cp", temp_sprintf("%s/android.jar", android_p_dir));
    cmd_append(&cmd, "-Xmetadata-version=2.2.0");
    for (size_t i = 0; i < ARRAY_LEN(kotlin_src); i++) {
        cmd_append(&cmd, temp_sprintf("%s/%s", src_dir, kotlin_src[i]));
    }

    if (!cmd_run(&cmd)) return 1;

    // Doing something with compiled kotlin code before adding to apk
    cmd_append(&cmd, d8);
    cmd_append(&cmd, "--lib", temp_sprintf("%s/android.jar", android_p_dir));
    cmd_append(&cmd, "--output", ".");
    if (!cmd_append_by_extention(&cmd, "classes", ".class")) return 1;
    if (!cmd_run(&cmd)) return 1;

    // Adding resources and native libraries to apk
    cmd_append(&cmd, aapt);
    cmd_append(&cmd, "add", "app.apk.unaligned");
    cmd_append(&cmd, "classes.dex");
    for (size_t i = 0; i < ARRAY_LEN(targets); i++) {
        cmd_append(&cmd, temp_sprintf("lib/%s/lib%s.so", targets[i], so_name));
    }
    if (!cmd_run(&cmd)) return 1;

    // Aligning apk
    cmd_append(&cmd, zipalign);
    cmd_append(&cmd, "-v", "-p", "4");
    cmd_append(&cmd, "app.apk.unaligned");
    cmd_append(&cmd, "app.apk");
    if (!cmd_run(&cmd)) return 1;

    // Signing apk
    cmd_append(&cmd, apksigner);
    cmd_append(&cmd, "sign", "--ks");
    cmd_append(&cmd, temp_sprintf("%s/%s", prj_dir, keystore));
    cmd_append(&cmd, "app.apk");
    if (!cmd_run(&cmd)) return 1;

    return 0;
}
