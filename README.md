<p align="center">
  <img width="3400" height="920" alt="gif_banner" src="https://github.com/user-attachments/assets/a7b95e79-72ad-4958-9da4-6a146f7941f7" />
</p>

<h1 align="center">Godot-GIF</h1>

<p align="center">
  A lightweight GIF reader and writer for Godot Engine
</p>

## Overview

Godot-GIF is a GDExtension plugin for Godot Engine 4.x that provides GIF reading and writing capabilities. It wraps giflib 5.2.2 to enable importing, exporting, and playing GIF files in Godot projects.

## Features

- Read GIF files and extract frames as Godot Image objects
- Write GIF files from Image arrays with customizable frame delays
- GIFTexture resource class for animated GIF display
- GIFPlayer UI node for built-in animation playback
- Support for transparency, looping, and disposal methods
- Import plugin for direct GIF file usage in editor

## Installation

1. Copy or symlink the `addons/godot_gif/` folder into your Godot project's `addons/` directory
2. Restart Godot or enable the plugin in Project Settings

## Quick Start

### Loading a GIF

```gdscript
var gif = GIFTexture.load_from_file("res://animation.gif")
$TextureRect.texture = gif
```

### Playing Animation

```gdscript
$GIFPlayer.gif = GIFTexture.load_from_file("res://animation.gif")
$GIFPlayer.play()
```

### Writing a GIF

```gdscript
var images: Array[Image] = [frame1, frame2, frame3]
var delays: Array[int] = [100, 100, 100]  # milliseconds

GIFWriter.save_to_file("res://output.gif", images, delays)
```

## Building from Source

Requires Python 3.8+, SCons 4.0+, and a C++20 compatible compiler.

## License

MIT License. See LICENSE.txt for details.

Includes giflib 5.2.2 (MIT License)

</content>
</invoke>