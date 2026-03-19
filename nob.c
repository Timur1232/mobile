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

Cmd cmd = {0};

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
bool clear_build_files()
{
    if (file_exists(BUILD_DIR)) {
        cmd_append(&cmd, "rm", "-r", BUILD_DIR);
        if (!cmd_run(&cmd)) {
            nob_log(NOB_ERROR, "Unable to delete build directory");
            return false;
        }
        cmd_append(&cmd, "mkdir", BUILD_DIR);
        if (!cmd_run(&cmd)) {
            nob_log(NOB_ERROR, "Unable to create build directory");
            return false;
        }
        nob_log(NOB_INFO, "Build files cleared");
    }
    return true;
}

typedef struct {
    const char* target;
    const char* compiler;
} TargetCompilerPair;

typedef struct {
    const char** items;
    size_t count;
    size_t capacity;
} StringDA;

bool compile_c_libraries(const TargetCompilerPair* targets, size_t targets_count, const char* so_name, const char** source_files, size_t source_files_count)
{
    for (size_t i = 0; i < targets_count; i++) {
        // Create directory for specified target
        cmd_append(&cmd, "mkdir", "-p");
        cmd_append(&cmd, temp_sprintf("lib/%s", targets[i].target));
        if (!cmd_run(&cmd)) {
            nob_log(NOB_ERROR, "Unable to create directory for target %s", targets[i].target);
            return false;
        }

        // Compile C code
        cmd_append(&cmd, targets[i].compiler);
        cmd_append(&cmd, "-shared");
        cmd_append(&cmd, "-o", temp_sprintf("lib/%s/lib%s.so", targets[i].target, so_name));
        for (size_t j = 0; j < source_files_count; j++) {
            cmd_append(&cmd, source_files[j]);
        }

        if (!cmd_run(&cmd)) {
            nob_log(NOB_ERROR, "Unable to compile files for target %s", targets[i].target);
            return false;
        }
    }
    return true;
}

typedef struct {
    const char* ext;
    StringDA* files;
} OnFileData;

bool on_file(Walk_Entry entry)
{
    OnFileData* data = entry.data;
    if (entry.type == NOB_FILE_REGULAR) {
        String_View name = sv_from_cstr(entry.path);
        if (sv_ends_with_cstr(name, data->ext)) {
            da_append(data->files, temp_strdup(entry.path));
        }
    }
    return true;
}

bool find_files_by_extention(StringDA* files, const char* dir, const char* ext)
{
    OnFileData data = {
        .ext = ext,
        .files = files,
    };
    return walk_dir(dir, on_file, .post_order = false, .data = &data);
}

bool compile_resources(const char** resources, size_t resources_count)
{
    cmd_append(&cmd, aapt2, "compile");
    cmd_append(&cmd, "-o", "res.flata");
    for (size_t i = 0; i < resources_count; i++) {
        cmd_append(&cmd, temp_sprintf("%s/%s", prj_dir, resources[i]));
    }
    return cmd_run(&cmd);
}

bool link_unaligned()
{
    cmd_append(&cmd, aapt2, "link");
    cmd_append(&cmd, "-o", "app.apk.unaligned");
    cmd_append(&cmd, "-I", temp_sprintf("%s/android.jar", android_p_dir));
    cmd_append(&cmd, "--manifest", temp_sprintf("%s/AndroidManifest.xml", prj_dir));
    cmd_append(&cmd, "--java", "java");
    cmd_append(&cmd, "--auto-add-overlay");
    cmd_append(&cmd, "res.flata");
    return cmd_run(&cmd);
}

bool generate_keystore_if_not_exist(const char* keystore)
{
    if (!file_exists(keystore)) {
        cmd_append(&cmd, "keytool");
        cmd_append(&cmd, "-genkey", "-v");
        cmd_append(&cmd, "-keystore", keystore);
        cmd_append(&cmd, "-alias", "my-alias");
        cmd_append(&cmd, "-keyalg", "RSA");
        cmd_append(&cmd, "-keysize", "2048");
        cmd_append(&cmd, "-validity", "10000");
        return cmd_run(&cmd);
    }
    return true;
}

bool compile_kotlin(const char** kotlin_src, size_t kotlin_src_count)
{
    cmd_append(&cmd, "kotlinc");
    cmd_append(&cmd, "-d", "classes");
    cmd_append(&cmd, "-cp", temp_sprintf("%s/android.jar", android_p_dir));
    cmd_append(&cmd, "-Xmetadata-version=2.2.0");
    for (size_t i = 0; i < kotlin_src_count; i++) {
        cmd_append(&cmd, kotlin_src[i]);
    }
    return cmd_run(&cmd);
}

bool compile_dex()
{
    cmd_append(&cmd, d8);
    cmd_append(&cmd, "--lib", temp_sprintf("%s/android.jar", android_p_dir));
    cmd_append(&cmd, "--output", ".");
    StringDA classes = {0};
    if (!find_files_by_extention(&classes, "classes", ".class")) return false;
    da_foreach(const char*, class, &classes) {
        cmd_append(&cmd, *class);
    }
    return cmd_run(&cmd);
}

bool add_dex_to_apk()
{
    cmd_append(&cmd, aapt);
    cmd_append(&cmd, "add", "app.apk.unaligned");
    cmd_append(&cmd, "classes.dex");
    return cmd_run(&cmd);
}

bool add_c_libraries_to_apk(const TargetCompilerPair* targets, size_t targets_count, const char* so_name)
{
    cmd_append(&cmd, aapt);
    cmd_append(&cmd, "add", "app.apk.unaligned");
    for (size_t i = 0; i < targets_count; i++) {
        cmd_append(&cmd, temp_sprintf("lib/%s/lib%s.so", targets[i].target, so_name));
    }
    return cmd_run(&cmd);
}

bool align_apk()
{
    cmd_append(&cmd, zipalign);
    cmd_append(&cmd, "-v", "-p", "4");
    cmd_append(&cmd, "app.apk.unaligned");
    cmd_append(&cmd, "app.apk");
    return cmd_run(&cmd);
}

bool sign_apk(const char* keystore)
{
    cmd_append(&cmd, apksigner);
    cmd_append(&cmd, "sign", "--ks");
    cmd_append(&cmd, temp_sprintf("%s/%s", prj_dir, keystore));
    cmd_append(&cmd, "app.apk");
    return cmd_run(&cmd);
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    get_paths();

    const char* keystore = "keystore.jks";

    StringDA c_src = {0};
    if (!find_files_by_extention(&c_src, csrc_dir, ".c")) return 1;

    TargetCompilerPair targets[] = {
        { .target = "x86_64", .compiler = x86_64_clang },
        { .target = "armeabi-v7a", .compiler = armv7a_clang },
        { .target = "aarch64", .compiler = aarch64_clang },
    };
    const char* so_name = "math";

    StringDA kotlin_src = {0};
    if (!find_files_by_extention(&kotlin_src, src_dir, ".kt")) return 1;

    const char* resources[] = {
        "res/mipmap-hdpi/ic_launcher.png",
    };

    if (!generate_keystore_if_not_exist(keystore)) return 1;
    if (!clear_build_files()) return 1;

    if (!set_current_dir(BUILD_DIR)) return 1;

    if (!compile_c_libraries(targets, ARRAY_LEN(targets), so_name, c_src.items, c_src.count)) return 1;
    if (!compile_resources(resources, ARRAY_LEN(resources))) return 1;
    if (!link_unaligned()) return 1;

    if (!compile_kotlin(kotlin_src.items, kotlin_src.count)) return 1;
    if (!compile_dex()) return 1;

    if (!add_dex_to_apk()) return 1;
    if (!add_c_libraries_to_apk(targets, ARRAY_LEN(targets), so_name)) return 1;

    if (!align_apk()) return 1;
    if (!sign_apk(keystore))

    return 0;
}
