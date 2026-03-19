# Godot-GIF Project Documentation

## Project Overview

Godot-GIF is a Godot GDExtension project that provides GIF image reading capabilities for Godot Engine 4.x. It wraps the `giflib` C library (version 5.2.2) to enable reading GIF files within Godot projects.

**Target Godot Version:** 4.1+ (tested with 4.5)  
**License:** MIT (giflib is also MIT licensed)  
**Language:** C++20 with C (for giflib integration)

## Project Structure

```
Godot-GIF/
├── addons/
│   └── godot_gif/
│       └── bin/
│           ├── godot_gif.gdextension    # GDExtension configuration
│           └── *.dll/*.so/*.framework   # Built library files
├── godot-cpp/                           # Godot C++ bindings (git submodule, branch 4.5)
│   ├── SConstruct                       # godot-cpp build script
│   ├── gdextension/                     # GDExtension API definitions
│   ├── include/                         # Godot C++ headers
│   └── src/                             # Godot C++ source files
├── src/
│   ├── gif_reader.h                     # GIFReader class header
│   ├── gif_reader.cpp                   # GIFReader implementation
│   ├── gif_utils.hpp                    # Common utilities header
│   ├── register_types.h                 # Module registration header
│   ├── register_types.cpp               # Module registration implementation
│   └── thirdparty/
│       └── giflib-5.2.2/                # GIF manipulation library (MIT license)
│           ├── gif_lib.h                # Main public API header
│           ├── dgif_lib.c               # Decoding functions
│           ├── egif_lib.c               # Encoding functions
│           └── ...                      # Additional giflib source files
├── Godot-GIF.slnx                       # Visual Studio solution file
├── Godot-GIF.vcxproj                    # Visual Studio project file
├── SConstruct                           # Main build script
└── README.md                            # Project README
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
     - `gif_err.c` - Error handling
     - `gif_hash.c` - Hash table utilities
     - `quantize.c` - Color quantization

3. **GIFReader Class** - Main C++ wrapper class
   - Inherits from `RefCounted` for Godot integration
   - Provides methods to open, read, and extract GIF data
   - Converts GIF frames to Godot `Image` objects

## Technology Stack

- **Build System:** SCons 4.0+ (primary), Visual Studio MSBuild (secondary/Windows)
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

From the project root:

```bash
scons platform=<platform> target=template_debug
scons platform=<platform> target=template_release
```

Build outputs are placed in `addons/godot_gif/bin/`:
- Windows: `godot_gif.windows.template_debug.x86_64.dll`
- Linux: `godot_gif.linux.template_debug.x86_64.so`
- macOS: `libgodot_gif.macos.template_debug.framework`
- iOS: `libgodot_gif.ios.template_debug.a` (static library)

### Supported Build Options

| Platform | Architectures | Debug | Release |
|----------|---------------|-------|---------|
| windows | x86_64 | ✓ | ✓ |
| linux | x86_64 | ✓ | ✓ |
| macos | universal | ✓ | ✓ |
| ios | arm64/simulator | ✓ | ✓ |
| android | arm64/x86_64 | ✓ | ✓ |
| web | wasm32 | ✓ | ✓ |

## GDExtension Configuration

The `.gdextension` file at `addons/godot_gif/bin/godot_gif.gdextension` registers the extension with Godot:

```ini
[configuration]

entry_symbol = "godot_gif_init"
compatibility_minimum = "4.1"
reloadable = true

[libraries]

windows.debug.x86_64 = "godot_gif.windows.template_debug.x86_64.dll"
```

To use the extension in a Godot project, copy or symlink the `addons/godot_gif/` folder into your project's `addons/` directory.

## API Reference

### GIFReader Class

The `GIFReader` class provides methods to read GIF files and extract image data.

#### Methods

| Method | Return Type | Description |
|--------|-------------|-------------|
| `open(path: String)` | GIFError | Opens a GIF file for reading |
| `close()` | GIFError | Closes the opened GIF file |
| `get_width()` | int | Returns the canvas width |
| `get_height()` | int | Returns the canvas height |
| `get_color_resolution()` | int | Returns the color resolution |
| `get_background_color()` | Color | Returns the background color |
| `get_aspect_byte()` | int | Returns the pixel aspect ratio byte |
| `get_color_map()` | Dictionary | Returns the global color map info |
| `get_image_count()` | int | Returns the number of images/frames |
| `get_image()` | Image | Returns the current frame as Godot Image |

#### GIFError Enum

| Constant | Value | Description |
|----------|-------|-------------|
| SUCCEEDED | 0 | Operation successful |
| OPEN_FAILED | 101 | Failed to open file |
| READ_FAILED | 102 | Failed to read from file |
| NOT_GIF_FILE | 103 | Not a GIF file |
| NO_SCRN_DSCR | 104 | No screen descriptor |
| NO_IMAG_DSCR | 105 | No image descriptor |
| NO_COLOR_MAP | 106 | No color map |
| WRONG_RECORD | 107 | Wrong record type |
| DATA_TOO_BIG | 108 | Data too big |
| NOT_ENOUGH_MEM | 109 | Not enough memory |
| CLOSE_FAILED | 110 | Failed to close file |
| NOT_READABLE | 111 | File not readable |
| IMAGE_DEFECT | 112 | Defective image |
| EOF_TOO_SOON | 113 | Unexpected EOF |

### Example Usage (GDScript)

```gdscript
var reader = GIFReader.new()

var err = reader.open("res://example.gif")
if err == GIFReader.SUCCEEDED:
    print("Width: ", reader.get_width())
    print("Height: ", reader.get_height())
    print("Frame count: ", reader.get_image_count())
    
    var image = reader.get_image()
    var texture = ImageTexture.create_from_image(image)
    $Sprite2D.texture = texture
    
    reader.close()
else:
    print("Failed to open GIF: ", err)
```

## Code Style Guidelines

### C++ Code

Follow the godot-cpp conventions:
- Use `snake_case` for variables, functions, and file names
- Use `PascalCase` for class names
- Use `SCREAMING_SNAKE_CASE` for constants and enums
- Indent with tabs (Godot standard)
- Use `//` for single-line comments, `/* */` for multi-line
- Always use namespaces (prefer `godot::` prefix or explicit `using namespace godot;`)
- Use `GDCLASS` macro for class registration
- Use `D_METHOD` macro for method binding

### C Code (giflib integration)

Follow existing giflib conventions:
- Use `snake_case` consistently
- Keep C-style comments (`/* */`)
- Wrap C headers with `extern "C"` when including in C++

### Example Class Structure

```cpp
#ifndef GIF_READER_H
#define GIF_READER_H

#include "gif_utils.hpp"

namespace godot {
    class Image;

    class GIFReader : public RefCounted {
        GDCLASS(GIFReader, RefCounted)

    public:
        enum GIFError {
            SUCCEEDED = 0,
            // ... error codes
        };

    private:
        GifFileType* file_type = nullptr;

    protected:
        static void _bind_methods();

    public:
        ~GIFReader();
        GIFError open(const String& p_path);
        // ... other methods
    };
}

VARIANT_ENUM_CAST(GIFReader::GIFError);

#endif // GIF_READER_H
```

## Development Workflow

### Adding New Features

1. Create C++ wrapper class in `src/` that exposes giflib functionality
2. Register the class in `register_types.cpp` using `GDREGISTER_CLASS()`
3. Add method bindings using `ClassDB::bind_method()` in `_bind_methods()`
4. Build and test in Godot

### Class Registration

Classes must be registered in `register_types.cpp`:

```cpp
#include "register_types.h"
#include "your_new_class.h"

void initialize_godot_gif_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    GDREGISTER_CLASS(YourNewClass);
}
```

## Testing

Currently, no automated test suite is implemented. Recommended testing approach:

1. Create a Godot project with a test scene
2. Add the `addons/godot_gif/` folder to the project
3. Create GDScript tests that exercise the GIF functionality:
   - Test various GIF formats (GIF87a, GIF89a)
   - Test animated GIFs
   - Test transparent GIFs
   - Test interlaced GIFs
   - Test error handling with invalid files

## Security Considerations

When working with giflib:
- giflib is a mature C library but always validate input files
- Be cautious with large GIF files (decompression bombs)
- Check for null pointers from giflib allocation functions
- giflib 5.2.2 includes security fixes; avoid downgrading
- The `DGifSlurp()` function reads entire GIF into memory; be careful with large files

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
