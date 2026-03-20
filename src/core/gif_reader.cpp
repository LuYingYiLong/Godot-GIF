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

		BIND_ENUM_CONSTANT(DISPOSAL_METHOD_UNSPECIFIED);
		BIND_ENUM_CONSTANT(DISPOSAL_METHOD_DO_NOT);
		BIND_ENUM_CONSTANT(DISPOSAL_METHOD_BACKGROUND);
		BIND_ENUM_CONSTANT(DISPOSAL_METHOD_PREVIOUS);

		ClassDB::bind_method(D_METHOD("open", "path"), &GIFReader::open);
		ClassDB::bind_method(D_METHOD("open_from_buffer", "buffer"), &GIFReader::open_from_buffer);
		ClassDB::bind_method(D_METHOD("close"), &GIFReader::close);
		ClassDB::bind_method(D_METHOD("get_size"), &GIFReader::get_size);
		ClassDB::bind_method(D_METHOD("get_color_resolution"), &GIFReader::get_color_resolution);
		ClassDB::bind_method(D_METHOD("get_background_color"), &GIFReader::get_background_color);
		ClassDB::bind_method(D_METHOD("get_aspect_byte"), &GIFReader::get_aspect_byte);
		ClassDB::bind_method(D_METHOD("get_color_map"), &GIFReader::get_color_map);
		ClassDB::bind_method(D_METHOD("get_image_count"), &GIFReader::get_image_count);
		ClassDB::bind_method(D_METHOD("get_image", "frame_index"), &GIFReader::get_image, DEFVAL(0));
		ClassDB::bind_method(D_METHOD("get_saved_images"), &GIFReader::get_saved_images);

		ClassDB::bind_method(D_METHOD("get_frame_delay", "frame_index"), &GIFReader::get_frame_delay);
		ClassDB::bind_method(D_METHOD("get_disposal_method", "frame_index"), &GIFReader::get_disposal_method);
		ClassDB::bind_method(D_METHOD("get_loop_count"), &GIFReader::get_loop_count);
		ClassDB::bind_method(D_METHOD("get_comments"), &GIFReader::get_comments);
		ClassDB::bind_method(D_METHOD("get_frame_gcb", "frame_index"), &GIFReader::get_frame_gcb);
		ClassDB::bind_method(D_METHOD("get_global_metadata"), &GIFReader::get_global_metadata);
	}

	GIFReader::~GIFReader() {
		close();
	}

	int GIFReader::_mem_input_func(GifFileType* gif, GifByteType* bytes, int size) {
		GIFReader* reader = static_cast<GIFReader*>(gif->UserData);
		if (!reader) return 0;

		int remaining = reader->mem_data.size() - reader->mem_read_pos;
		int to_read = (size > remaining) ? remaining : size;

		if (to_read > 0) {
			memcpy(bytes, reader->mem_data.ptr() + reader->mem_read_pos, to_read);
			reader->mem_read_pos += to_read;
		}

		return to_read;
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
		// 注意: DGifOpenFileHandle 不会关闭文件句柄，我们需要自己管理
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

	GIFReader::GIFError GIFReader::open_from_buffer(const PackedByteArray& p_buffer) {
		if (file_type) {
			close();
		}

		if (p_buffer.is_empty()) {
			return OPEN_FAILED;
		}

		// 保存内存数据
		mem_data = p_buffer;
		mem_read_pos = 0;

		int err;

		// 使用 DGifOpen 自定义输入函数从内存读取
		file_type = DGifOpen(this, _mem_input_func, &err);

		if (!file_type) return static_cast<GIFError>(err);

		// 加载所有图像数据
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
		// 清理内存数据
		mem_data.clear();
		mem_read_pos = 0;
		return SUCCEEDED;
	}

	Vector2i GIFReader::get_size() const {
		if (!file_type) return Vector2i();
		return Vector2i(file_type->SWidth, file_type->SHeight);
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

	Ref<Image> GIFReader::get_image(const int frame_index) const {
		if (!file_type) return Ref<Image>();

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
		// -1 表示没有透明色
		int transparent_color = gcb.TransparentColor;

		// 创建图像数据缓冲区 (RGBA 格式)
		PackedByteArray image_data;
		image_data.resize(width * height * 4);

		// 获取图像光栅数据
		GifByteType* raster = saved_image->RasterBits;
		if (!raster) return Ref<Image>();

		// 转换为 RGBA
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				int64_t idx = y * width + x;
				int color_idx = raster[idx];

				if (color_idx >= cmap->ColorCount) color_idx = 0;

				GifColorType color = cmap->Colors[color_idx];
				int64_t pixel_idx = idx * 4;

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
		int count = get_image_count();
		TypedArray<Image> images;
		images.resize(count);

		for (int i = 0; i < count; i++) {
			images[i] = get_image(i);
		}

		return images;
	}

	int GIFReader::get_frame_delay(const int frame_index) const {
		if (!file_type || frame_index < 0 || frame_index >= file_type->ImageCount) return 0;

		GraphicsControlBlock gcb;
		if (DGifSavedExtensionToGCB(file_type, frame_index, &gcb) == GIF_ERROR) {
			return 0;
		}

		// DelayTime 单位是 1/100 秒
		// 所以转换为毫秒
		return gcb.DelayTime * 10;
	}

	GIFReader::DisposalMethod GIFReader::get_disposal_method(const int frame_index) const {
		if (!file_type || frame_index < 0 || frame_index >= file_type->ImageCount) {
			return DISPOSAL_METHOD_UNSPECIFIED;
		}

		GraphicsControlBlock gcb;
		if (DGifSavedExtensionToGCB(file_type, frame_index, &gcb) == GIF_ERROR) {
			return DISPOSAL_METHOD_UNSPECIFIED;
		}

		return static_cast<DisposalMethod>(gcb.DisposalMode);
	}

	int GIFReader::get_loop_count() const {
		if (!file_type) return 1;

		// Netscape 应用扩展通常在全局 ExtensionBlocks 中
		for (int i = 0; i < file_type->ExtensionBlockCount; i++) {
			ExtensionBlock* ext = &file_type->ExtensionBlocks[i];
			if (ext->Function == APPLICATION_EXT_FUNC_CODE && ext->ByteCount >= 11) {
				// 检查是否是 Netscape 循环扩展
				if (memcmp(ext->Bytes, "NETSCAPE2.0", 11) == 0 ||
				    memcmp(ext->Bytes, "ANIMEXTS1.0", 11) == 0) {
					// 子块包含循环次数
					if (i + 1 < file_type->ExtensionBlockCount) {
						ExtensionBlock* sub = &file_type->ExtensionBlocks[i + 1];
						if (sub->ByteCount >= 3 && sub->Bytes[0] == 1) {
							// 小端序读取循环次数
							int loop = sub->Bytes[1] | (sub->Bytes[2] << 8);
							return loop; // 0 表示无限循环
						}
					}
				}
			}
		}
		return 1;
	}

	PackedStringArray GIFReader::get_comments() const {
		PackedStringArray comments;
		if (!file_type) return comments;

		// 检查全局扩展块
		for (int i = 0; i < file_type->ExtensionBlockCount; i++) {
			ExtensionBlock* ext = &file_type->ExtensionBlocks[i];
			if (ext->Function == COMMENT_EXT_FUNC_CODE && ext->Bytes) {
				String comment;
				comment.parse_utf8((const char*)ext->Bytes, ext->ByteCount);
				comments.append(comment);
			}
		}

		// 检查每帧前的扩展块
		for (int img = 0; img < file_type->ImageCount; img++) {
			SavedImage* sp = &file_type->SavedImages[img];
			for (int i = 0; i < sp->ExtensionBlockCount; i++) {
				ExtensionBlock* ext = &sp->ExtensionBlocks[i];
				if (ext->Function == COMMENT_EXT_FUNC_CODE && ext->Bytes) {
					String comment;
					comment.parse_utf8((const char*)ext->Bytes, ext->ByteCount);
					comments.append(comment);
				}
			}
		}

		return comments;
	}

	Dictionary GIFReader::get_frame_gcb(const int frame_index) const {
		Dictionary gcb_data;
		if (!file_type || frame_index < 0 || frame_index >= file_type->ImageCount) {
			return gcb_data;
		}

		GraphicsControlBlock gcb;
		if (DGifSavedExtensionToGCB(file_type, frame_index, &gcb) == GIF_ERROR) {
			return gcb_data;
		}

		gcb_data["disposal_mode"] = gcb.DisposalMode;
		gcb_data["delay_time_ms"] = gcb.DelayTime * 10;
		gcb_data["transparent_color"] = gcb.TransparentColor;
		gcb_data["user_input_flag"] = gcb.UserInputFlag;

		return gcb_data;
	}

	Dictionary GIFReader::get_global_metadata() const {
		Dictionary metadata;
		if (!file_type) return metadata;

		// 基础信息
		metadata["width"] = (int)file_type->SWidth;
		metadata["height"] = (int)file_type->SHeight;
		metadata["color_resolution"] = (int)file_type->SColorResolution;
		metadata["background_color_index"] = (int)file_type->SBackGroundColor;
		metadata["frame_count"] = file_type->ImageCount;
		metadata["loop_count"] = get_loop_count();

		// 计算像素宽高比
		if (file_type->AspectByte != 0) {
			float aspect = ((float)file_type->AspectByte + 15.0f) / 64.0f;
			metadata["pixel_aspect_ratio"] = aspect;
		} else {
			metadata["pixel_aspect_ratio"] = 1.0f;
		}

		// 是否有全局颜色表
		metadata["has_global_color_map"] = (file_type->SColorMap != nullptr);

		// 收集所有注释
		PackedStringArray comments = get_comments();
		if (comments.size() > 0) {
			metadata["comments"] = comments;
		}

		return metadata;
	}
}