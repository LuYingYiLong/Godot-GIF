#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# tweak this if you want to use different folders, or more folders, to store your source code in.
env.Append(CPPPATH=["src/", "src/thirdparty/giflib-5.2.2/"])

# Collect C++ sources
sources = Glob("src/*.cpp") + Glob("src/core/*.cpp") + Glob("src/editor/*.cpp") + \
          Glob("src/node/*.cpp")

# Collect giflib C sources (只包含核心库文件，不包含工具程序)
giflib_sources = [
    "src/thirdparty/giflib-5.2.2/dgif_lib.c",               # GIF 解码
    "src/thirdparty/giflib-5.2.2/egif_lib.c",               # GIF 编码
    "src/thirdparty/giflib-5.2.2/gifalloc.c",               # 内存分配
    "src/thirdparty/giflib-5.2.2/gif_err.c",                # 错误处理
    "src/thirdparty/giflib-5.2.2/gif_hash.c",               # 哈希表
    "src/thirdparty/giflib-5.2.2/quantize.c",               # 颜色量化
    "src/thirdparty/giflib-5.2.2/openbsd-reallocarray.c",   # 兼容层
]
# Use SharedObject for Android compatibility with shared libraries
if env["platform"] == "android":
    sources += [env.SharedObject(f) for f in giflib_sources]
else:
    sources += [env.Object(f) for f in giflib_sources]

if env["target"] in ["editor", "template_debug"]:
    try:
        doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=Glob("doc_classes/*.xml"))
        sources.append(doc_data)
    except AttributeError:
        print("Not including class reference as we're targeting a pre-4.3 baseline.")

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "addons/godot_gif/bin/libgodot_gif.{}.{}.framework/libgodot_gif.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
elif env["platform"] == "ios":
    if env["ios_simulator"]:
        library = env.StaticLibrary(
            "addons/godot_gif/bin/libgodot_gif.{}.{}.simulator.a".format(env["platform"], env["target"]),
            source=sources,
        )
    else:
        library = env.StaticLibrary(
            "addons/godot_gif/bin/libgodot_gif.{}.{}.a".format(env["platform"], env["target"]),
            source=sources,
        )

elif env["platform"] == "android":
    library = env.SharedLibrary(
        "addons/godot_gif/bin/libgodot_gif{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

else:
    library = env.SharedLibrary(
        "addons/godot_gif/bin/godot_gif{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)
