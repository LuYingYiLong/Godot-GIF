# Godot-GIF Project Documentation

## Project Overview

Godot-GIF is a Godot GDExtension project that aims to provide GIF image manipulation capabilities for Godot Engine 4.x. It wraps the popular `giflib` C library (version 5.2.2) to enable reading and writing GIF files within Godot projects.

**Target Godot Version:** 4.5 (stable)  
**License:** MIT (giflib is also MIT licensed)  
**Language:** C++20 with C (for giflib integration)

## Project Structure

```
Godot-GIF/
├── godot-cpp/              # Godot C++ bindings (git submodule, branch 4.5)
│   ├── SConstruct          # Main build script for godot-cpp
│   ├── gdextension/        # GDExtension API definitions
│   ├── include/            # Godot C++ headers
│   └── src/                # Godot C++ source files
├── src/
│   └── thirdparty/
│       └── giflib-5.2.2/   # GIF manipulation library (MIT license)
│           ├── gif_lib.h   # Main public API header
│           ├── dgif_lib.c  # Decoding functions
│           ├── egif_lib.c  # Encoding functions
│           └── ...         # Additional giflib source files
├── Godot-GIF.slnx          # Visual Studio solution file
├── Godot-GIF.vcxproj       # Visual Studio project file (template)
└── README.md               # Project README
```

### Key Components

1. **godot-cpp/** - Git submodule containing official Godot C++ bindings
   - URL: https://github.com/godotengine/godot-cpp.git
   - Branch: 4.5
   - This provides the C++ interface to Godot's GDExtension API

2. **src/thirdparty/giflib-5.2.2/** - Third-party GIF library
   - MIT licensed C library for reading and writing GIF images
   - Key files for integration:
     - `gif_lib.h` - Public API header
     - `dgif_lib.c` - GIF decoding
     - `egif_lib.c` - GIF encoding
     - `gifalloc.c` - Memory allocation utilities

## Technology Stack

- **Build System:** SCons (primary), Visual Studio MSBuild (secondary/Windows)
- **Languages:** C++20, C99
- **Platform Support:** Windows (current), extendable to Linux/macOS/Android/iOS/Web via godot-cpp
- **Dependencies:**
  - godot-cpp (submodule)
  - giflib 5.2.2 (bundled in thirdparty)

## Build Instructions

### Prerequisites

1. Python 3.8+
2. SCons 4.0+
3. C++ compiler with C++20 support:
   - Windows: Visual Studio 2022 (v145 toolset) or later
   - Linux: GCC 10+ or Clang 12+
   - macOS: Xcode 13+

### Building godot-cpp (Required First Step)

Before building the extension itself, build the godot-cpp static library:

```bash
cd godot-cpp
scons platform=<platform> target=template_debug
scons platform=<platform> target=template_release
```

Replace `<platform>` with: `windows`, `linux`, `macos`, `android`, `ios`, or `web`

### Building the Extension

Once source files are added to the project, create a root `SConstruct` file that references godot-cpp:

```python
#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

# Add include paths
env.Append(CPPPATH=["src/", "src/thirdparty/giflib-5.2.2/"])

# Collect source files
sources = Glob("src/*.cpp")
# Add giflib C sources
sources += Glob("src/thirdparty/giflib-5.2.2/*.c")

# Build library
if env["platform"] == "macos":
    library = env.SharedLibrary(
        "demo/bin/libgodotgif.{}.{}.framework/libgodotgif.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
else:
    library = env.SharedLibrary(
        "demo/bin/libgodotgif{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)
```

Build with:
```bash
scons platform=<platform> target=template_debug
scons platform=<platform> target=template_release
```

## GDExtension Configuration

A `.gdextension` file must be created to register the extension with Godot. Example:

```ini
[configuration]

entry_symbol = "godotgif_library_init"
compatibility_minimum = "4.1"

[libraries]

macos.debug = "res://bin/libgodotgif.macos.template_debug.framework"
macos.release = "res://bin/libgodotgif.macos.template_release.framework"
windows.debug.x86_64 = "res://bin/libgodotgif.windows.template_debug.x86_64.dll"
windows.release.x86_64 = "res://bin/libgodotgif.windows.template_release.x86_64.dll"
linux.debug.x86_64 = "res://bin/libgodotgif.linux.template_debug.x86_64.so"
linux.release.x86_64 = "res://bin/libgodotgif.linux.template_release.x86_64.so"
```

## Code Style Guidelines

### C++ Code

Follow the godot-cpp conventions:
- Use `snake_case` for variables, functions, and file names
- Use `PascalCase` for class names
- Use `SCREAMING_SNAKE_CASE` for constants and macros
- Indent with tabs (Godot standard)
- Use `//` for single-line comments, `/* */` for multi-line
- Always use namespaces (prefer `godot::` prefix or explicit `using namespace godot;`)

### C Code (giflib integration)

Follow existing giflib conventions:
- Use `snake_case` consistently
- Prefix custom functions to avoid namespace conflicts
- Keep C-style comments (`/* */`)

### Example Class Structure

```cpp
#ifndef GIF_LOADER_H
#define GIF_LOADER_H

#include <godot_cpp/classes/ref_counted.hpp>

namespace godot {

class GifLoader : public RefCounted {
    GDCLASS(GifLoader, RefCounted)

protected:
    static void _bind_methods();

public:
    Error load_from_file(const String &p_path);
    // ... other methods
};

}

#endif
```

## Development Workflow

### Adding New Features

1. Create C++ wrapper class in `src/` that exposes giflib functionality
2. Register the class in the initialization function
3. Add bindings using `ClassDB::bind_method()`
4. Build and test in Godot

### Registering Classes

Classes must be registered in the initialization function:

```cpp
#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_godotgif_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    // Register your classes here
    // GDREGISTER_CLASS(GifLoader);
    // GDREGISTER_CLASS(GifSaver);
}

void uninitialize_godotgif_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

extern "C" {
GDExtensionBool GDE_EXPORT godotgif_library_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization) {
    
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
    init_obj.register_initializer(initialize_godotgif_module);
    init_obj.register_terminator(uninitialize_godotgif_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
    return init_obj.init();
}
}
```

## Testing

Currently, no test suite is implemented. Recommended testing approach:

1. Create a Godot project in a `demo/` folder
2. Add the `.gdextension` file to the project
3. Create GDScript tests that exercise the GIF functionality
4. Test various GIF formats (GIF87a, GIF89a, animated, transparent, interlaced)

## Current State

**Note:** This project is in early development stages:

- ✅ godot-cpp submodule configured (branch 4.5)
- ✅ giflib 5.2.2 integrated in thirdparty
- ✅ Visual Studio project files created (template)
- ⬜ GDExtension wrapper classes (not yet implemented)
- ⬜ SConstruct build file at project root (not yet created)
- ⬜ .gdextension configuration file (not yet created)
- ⬜ Demo/test project (not yet created)

## Security Considerations

When working with giflib:
- giflib is a mature C library but always validate input files
- Be cautious with large GIF files (decompression bombs)
- Check for null pointers from giflib allocation functions
- giflib 5.2.2 includes security fixes; avoid downgrading

## Third-Party Licenses

### giflib 5.2.2
MIT License - See `src/thirdparty/giflib-5.2.2/COPYING`

### godot-cpp
MIT License - See `godot-cpp/LICENSE.md`

## Resources

- [Godot GDExtension Documentation](https://docs.godotengine.org/en/stable/tutorials/scripting/gdextension/what_is_gdextension.html)
- [godot-cpp Template Project](https://github.com/godotengine/godot-cpp-template)
- [giflib Documentation](src/thirdparty/giflib-5.2.2/doc/)
- [Godot Engine Source Build Instructions](https://docs.godotengine.org/en/stable/contributing/development/compiling/index.html)
