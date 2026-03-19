#include "gif_reader.h"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/core/error_macros.hpp>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#endif

namespace godot {
	void GIFReader::_bind_methods() {
		BIND_ENUM_CONSTANT(SUCCEEDED);
		BIND_ENUM_CONSTANT(OPEN_FAILED);
		BIND_ENUM_CONSTANT(READ_FAILED);
		BIND_ENUM_CONSTANT(NOT_GIF_FILE);
		BIND_ENUM_CONSTANT(NO_SCRN_DSCR);
		BIND_ENUM_CONSTANT(NO_IMAG_DSCR);
		BIND_ENUM_CONSTANT(NO_COLOR_MAP);
		BIND_ENUM_CONSTANT(WRONG_RECORD);
		BIND_ENUM_CONSTANT(DATA_TOO_BIG);
		BIND_ENUM_CONSTANT(NOT_ENOUGH_MEM);
		BIND_ENUM_CONSTANT(CLOSE_FAILED);
		BIND_ENUM_CONSTANT(NOT_READABLE);
		BIND_ENUM_CONSTANT(IMAGE_DEFECT);
		BIND_ENUM_CONSTANT(EOF_TOO_SOON);

		ClassDB::bind_method(D_METHOD("open", "path"), &GIFReader::open);
		ClassDB::bind_method(D_METHOD("get_width"), &GIFReader::get_width);
		ClassDB::bind_method(D_METHOD("get_height"), &GIFReader::get_height);
		ClassDB::bind_method(D_METHOD("get_color_resolution"), &GIFReader::get_color_resolution);
		ClassDB::bind_method(D_METHOD("get_background_color"), &GIFReader::get_background_color);
		ClassDB::bind_method(D_METHOD("get_aspect_byte"), &GIFReader::get_aspect_byte);
		ClassDB::bind_method(D_METHOD("get_color_map"), &GIFReader::get_color_map);
		ClassDB::bind_method(D_METHOD("get_image_count"), &GIFReader::get_image_count);
		ClassDB::bind_method(D_METHOD("get_image", "frame_index"), &GIFReader::get_image, DEFVAL(0));
	}

	GIFReader::~GIFReader() {
		close();
	}

	GIFReader::GIFError GIFReader::open(const String& p_path) {
		if (file_type) {
			close();
		}

		int err;

#ifdef _WIN32
		// Windows 需要使用宽字符路径来支持中文
		Char16String path_utf16 = p_path.utf16();
		FILE* file = _wfopen((const wchar_t*)path_utf16.get_data(), L"rb");
		if (!file) {
			return OPEN_FAILED;
		}
		int fd = _fileno(file);
		file_type = DGifOpenFileHandle(fd, &err);
		// 注意：DGifOpenFileHandle 不会关闭文件句柄，我们需要自己管理
		// 但 giflib 会在 DGifCloseFile 时关闭句柄
#else
		CharString path_utf8 = p_path.utf8();
		file_type = DGifOpenFileName(path_utf8.get_data(), &err);
#endif
		if (!file_type) return static_cast<GIFError>(err);

		// 加载所有图像数据 (DGifSlurp 会填充 ImageCount 等信息)
		if (DGifSlurp(file_type) == GIF_ERROR) {
			close();
			return READ_FAILED;
		}

		return SUCCEEDED;
	}

	GIFReader::GIFError GIFReader::close() {
		if (file_type) {
			int err;
			DGifCloseFile(file_type, &err);
			if (err == GIF_ERROR) return static_cast<GIFError>(err);
			file_type = nullptr;
		}
		return SUCCEEDED;
	}

	int GIFReader::get_width() const {
		if (!file_type) return 0;
		return (int)file_type->SWidth;
	}

	int GIFReader::get_height() const {
		if (!file_type) return 0;
		return (int)file_type->SHeight;
	}

	int GIFReader::get_color_resolution() const {
		if (!file_type) return 0;
		return (int)file_type->SColorResolution;
	}

	Color GIFReader::get_background_color() const {
		if (!file_type) return Color();
		int bg_index = file_type->SBackGroundColor;

		// 从全局/局部调色板获取颜色
		ColorMapObject* cmap = file_type->SColorMap;
		if (!cmap || bg_index >= cmap->ColorCount) return Color();

		GifColorType color = cmap->Colors[bg_index];
		return Color(color.Red / 255.0f, color.Green / 255.0f, color.Blue / 255.0f);
	}

	int GIFReader::get_aspect_byte() const {
		if (!file_type) return 0;
		return (int)file_type->AspectByte;
	}

	Dictionary GIFReader::get_color_map() const {
		if (!file_type || !file_type->SColorMap) return Dictionary();
		ColorMapObject* color_map = file_type->SColorMap;

		// 转换为 Godot Color (0-1 范围)
		GifColorType* gif_color = color_map->Colors;
		Color color;
		if (gif_color) color = Color(gif_color->Red / 255.0f, gif_color->Green / 255.0f, gif_color->Blue / 255.0f);

		Dictionary info;
		info["color_count"] = color_map->ColorCount;
		info["bits_per_pixel"] = color_map->BitsPerPixel;
		info["sort_flag"] = color_map->SortFlag;
		info["color"] = color;

		return info;
	}

	int GIFReader::get_image_count() const {
		if (!file_type) return 0;
		return file_type->ImageCount;
	}

	Ref<Image> GIFReader::get_image(int frame_index) const {
		if (!file_type) return Ref<Image>();

		// 数据已在 open() 时通过 DGifSlurp 加载

		// 验证帧索引
		if (frame_index < 0 || frame_index >= file_type->ImageCount) {
			return Ref<Image>();
		}

		// 获取指定帧
		SavedImage* saved_image = &file_type->SavedImages[frame_index];
		GifImageDesc* img_desc = &saved_image->ImageDesc;
		int64_t width = img_desc->Width;
		int64_t height = img_desc->Height;
		int64_t left = img_desc->Left;
		int64_t top = img_desc->Top;

		if (width <= 0 || height <= 0) return Ref<Image>();

		// 获取调色板 (优先使用局部调色板)
		ColorMapObject* cmap = img_desc->ColorMap;
		if (!cmap) cmap = file_type->SColorMap;
		if (!cmap) return Ref<Image>();

		// 获取 Graphics Control Block (包含透明色信息)
		GraphicsControlBlock gcb;
		DGifSavedExtensionToGCB(file_type, frame_index, &gcb);
		int transparent_color = gcb.TransparentColor; // -1 表示没有透明色

		// 创建图像数据缓冲区 (RGBA 格式)
		PackedByteArray image_data;
		image_data.resize(width * height * 4);

		// 获取图像光栅数据
		GifByteType* raster = saved_image->RasterBits;
		if (!raster) return Ref<Image>();

		// 转换为 RGBA
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				int idx = y * width + x;
				int color_idx = raster[idx];

				if (color_idx >= cmap->ColorCount) color_idx = 0;

				GifColorType color = cmap->Colors[color_idx];
				int pixel_idx = idx * 4;

				image_data[pixel_idx + 0] = color.Red;
				image_data[pixel_idx + 1] = color.Green;
				image_data[pixel_idx + 2] = color.Blue;
				// 透明色处理: 如果该像素是透明色索引，则 Alpha 设为 0
				image_data[pixel_idx + 3] = (color_idx == transparent_color) ? 0 : 255;
			}
		}

		// 创建 Image
		Ref<Image> image = memnew(Image());
		image->set_data(width, height, false, Image::FORMAT_RGBA8, image_data);

		return image;
	}

	TypedArray<Image> GIFReader::get_saved_images() const {
		if (!file_type) return TypedArray<Image>();
	}
}