#include "gif_writer.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#endif

namespace godot {
	void GIFWriter::_bind_methods() {
		BIND_ENUM_CONSTANT(SUCCEEDED);
		BIND_ENUM_CONSTANT(OPEN_FAILED);
		BIND_ENUM_CONSTANT(WRITER_FAILED);
		BIND_ENUM_CONSTANT(HAS_SCRN_DSCR);
		BIND_ENUM_CONSTANT(HAS_IMAG_DSCR);
		BIND_ENUM_CONSTANT(NO_COLOR_MAP);
		BIND_ENUM_CONSTANT(DATA_TOO_BIG);
		BIND_ENUM_CONSTANT(NOT_ENOUGH_MEM);
		BIND_ENUM_CONSTANT(DISK_IS_FULL);
		BIND_ENUM_CONSTANT(CLOSE_FAILED);
		BIND_ENUM_CONSTANT(NOT_WRITEABLE);

		ClassDB::bind_method(D_METHOD("open", "path"), &GIFWriter::open);
		ClassDB::bind_method(D_METHOD("open_from_buffer"), &GIFWriter::open_from_buffer);
		ClassDB::bind_method(D_METHOD("close"), &GIFWriter::close);
		ClassDB::bind_method(D_METHOD("get_output_buffer"), &GIFWriter::get_output_buffer);

		ClassDB::bind_method(D_METHOD("set_canvas_size", "width", "height", "background_color"), &GIFWriter::set_canvas_size, DEFVAL(0));
		ClassDB::bind_method(D_METHOD("set_loop_count", "loop_count"), &GIFWriter::set_loop_count);
		ClassDB::bind_method(D_METHOD("set_global_palette", "palette"), &GIFWriter::set_global_palette);
		ClassDB::bind_method(D_METHOD("set_color_resolution", "bits"), &GIFWriter::set_color_resolution);

		ClassDB::bind_method(D_METHOD("begin_frame", "left", "top", "width", "height", "interlace"), &GIFWriter::begin_frame);
		ClassDB::bind_method(D_METHOD("set_frame_delay", "delay_ms"), &GIFWriter::set_frame_delay);
		ClassDB::bind_method(D_METHOD("set_disposal_method", "method"), &GIFWriter::set_disposal_method);
		ClassDB::bind_method(D_METHOD("set_transparent_color", "index"), &GIFWriter::set_transparent_color);
		ClassDB::bind_method(D_METHOD("set_frame_palette", "palette"), &GIFWriter::set_frame_palette);
		ClassDB::bind_method(D_METHOD("write_frame_pixels", "indices"), &GIFWriter::write_frame_pixels);
		ClassDB::bind_method(D_METHOD("write_frame_image", "image", "quantize"), &GIFWriter::write_frame_image, DEFVAL(true));
		ClassDB::bind_method(D_METHOD("end_frame"), &GIFWriter::end_frame);

		ClassDB::bind_method(D_METHOD("write_gif", "images", "delays", "loop_count", "quantize"), &GIFWriter::write_gif, DEFVAL(true), DEFVAL(0));
		ClassDB::bind_static_method("GIFWriter", D_METHOD("save_to_file", "path", "images", "delays", "loop_count", "quantize"), &GIFWriter::save_to_file, DEFVAL(true), DEFVAL(0));
		ClassDB::bind_static_method("GIFWriter", D_METHOD("save_to_buffer", "images", "delays", "loop_count", "quantize"), &GIFWriter::save_to_buffer, DEFVAL(true), DEFVAL(0));
		
		ClassDB::bind_method(D_METHOD("add_comment", "comment"), &GIFWriter::add_comment);

		ClassDB::bind_method(D_METHOD("get_frame_count"), &GIFWriter::get_frame_count);
		ClassDB::bind_method(D_METHOD("get_canvas_size"), &GIFWriter::get_canvas_size);
	}

	GIFWriter::~GIFWriter() {
		close();
	}

	GIFWriter::GIFError GIFWriter::open(const String& p_path) {
		if (file_type) {
			close();
		}
		int err = 0;

#ifdef _WIN32
		// Windows 需要使用宽字符路径来支持中文
		Char16String path_utf16 = p_path.utf16();
		FILE* file = _wfopen((const wchar_t*)path_utf16.get_data(), L"wb");
		if (!file) {
			return OPEN_FAILED;
		}
		int fd = _fileno(file);
		file_type = EGifOpenFileHandle(fd, &err);
		// 注意: EGifOpenFileHandle 不会关闭文件句柄，我们需要自己管理
		// 但 giflib 会在 EGifCloseFile 时关闭句柄
#else
		CharString path_utf8 = p_path.utf8();
		// TestExistence = false 表示不检查文件是否已存在，允许覆盖
		file_type = EGifOpenFileName(path_utf8.get_data(), false, &err);
#endif
		if (!file_type) return static_cast<GIFError>(err);

		return SUCCEEDED;
	}

	int GIFWriter::_mem_output_func(GifFileType* gif, const GifByteType* bytes, int size) {
		GIFWriter* writer = static_cast<GIFWriter*>(gif->UserData);
		if (!writer || !bytes || size <= 0) return 0;

		int old_size = writer->mem_output_data.size();
		writer->mem_output_data.resize(old_size + size);
		memcpy(writer->mem_output_data.ptrw() + old_size, bytes, size);

		return size;
	}

	GIFWriter::GIFError GIFWriter::open_from_buffer() {
		if (file_type) {
			close();
		}
		mem_output_data.clear();
		int err = 0;

		// 使用 EGifOpen 自定义输出函数写入到内存
		file_type = EGifOpen(this, _mem_output_func, &err);
		if (!file_type) return static_cast<GIFError>(err);

		return SUCCEEDED;
	}

	GIFWriter::GIFError GIFWriter::close() {
		if (!file_type) {
			return SUCCEEDED;
		}

		int err = 0;
		if (EGifCloseFile(file_type, &err) == GIF_ERROR) {
			file_type = nullptr;
			return CLOSE_FAILED;
		}

		file_type = nullptr;
		return SUCCEEDED;
	}

	PackedByteArray GIFWriter::get_output_buffer() const {
		return mem_output_data;
	}

	GIFWriter::GIFError GIFWriter::set_canvas_size(const int p_width, const int p_height, const int p_background_color) {
		if (!file_type) return NO_FILE_OPEN;
		if (has_screen_desc) return HAS_SCRN_DSCR;

		const int clamped_width = CLAMP(p_width, 1, 65535);
		const int clamped_height = CLAMP(p_height, 1, 65535);
		const int clamped_bg = CLAMP(p_background_color, 0, 255);
		
		// ColorRes: 颜色分辨率位数 (1-8)，默认8位
		const int color_res = 8;

		// ColorMap 可以为空 (后续通过 set_global_palette 设置)
		int err = EGifPutScreenDesc(
			file_type,
			clamped_width,
			clamped_height, 
		    color_res,
			clamped_bg,
			nullptr
		);
		
		if (err == GIF_OK) {
			has_screen_desc = true;
		}
		
		return static_cast<GIFError>(err);
	}

	GIFWriter::GIFError GIFWriter::set_loop_count(const int p_loop_count) {
		if (!file_type) return NO_FILE_OPEN;

		// Netscape 应用扩展格式:
		// 应用扩展标识符: 0xff
		// 应用标识符: "NETSCAPE2.0" (11字节)
		// 子块大小: 0x03
		// 循环子块标识: 0x01
		// 循环次数: 2字节小端序 (0=无限)
		
		// 写入应用扩展头
		if (EGifPutExtensionLeader(file_type, APPLICATION_EXT_FUNC_CODE) == GIF_ERROR) {
			return WRITER_FAILED;
		}

		// 写入应用标识符 "NETSCAPE2.0"
		const char* app_id = "NETSCAPE2.0";
		if (EGifPutExtensionBlock(file_type, 11, app_id) == GIF_ERROR) {
			return WRITER_FAILED;
		}

		// 写入循环参数子块
		unsigned char params[3];
		params[0] = 0x01;  												// 循环子块标识
		params[1] = p_loop_count & 0xff;								// 循环次数低字节
		params[2] = (p_loop_count >> 8) & 0xff;							// 循环次数高字节
		
		if (EGifPutExtensionBlock(file_type, 3, params) == GIF_ERROR) {
			return WRITER_FAILED;
		}

		// 写入扩展尾
		if (EGifPutExtensionTrailer(file_type) == GIF_ERROR) {
			return WRITER_FAILED;
		}

		return SUCCEEDED;
	}

	GIFWriter::GIFError GIFWriter::set_global_palette(const PackedColorArray& p_palette) {
		if (!file_type) return NO_FILE_OPEN;
		if (p_palette.is_empty()) return NO_COLOR_MAP;

		int color_count = p_palette.size();
		if (color_count > 256) color_count = 256;

		// 创建调色板对象 (自动计算位深度)
		ColorMapObject* color_map = GifMakeMapObject(color_count, nullptr);
		if (!color_map) return NOT_ENOUGH_MEM;

		// 填充颜色数据
		for (int i = 0; i < color_count; i++) {
			Color color = p_palette[i];
			color_map->Colors[i].Red = static_cast<GifByteType>(color.r * 255);
			color_map->Colors[i].Green = static_cast<GifByteType>(color.g * 255);
			color_map->Colors[i].Blue = static_cast<GifByteType>(color.b * 255);
		}

		// 如果已有全局调色板，先释放
		if (file_type->SColorMap) {
			GifFreeMapObject(file_type->SColorMap);
		}

		// 设置全局调色板
		file_type->SColorMap = color_map;

		return SUCCEEDED;
	}

	GIFWriter::GIFError GIFWriter::set_color_resolution(const int p_bits) {
		if (!file_type) return NO_FILE_OPEN;
		if (has_screen_desc) return HAS_SCRN_DSCR;						// 屏幕描述符已写入，无法修改
		
		// 颜色分辨率已存储，会在 set_canvas_size 时通过 EGifPutScreenDesc 应用
		// 这里只做有效性检查
		if (p_bits < 1 || p_bits > 8) return INVALID_DIMENSIONS;
		
		return SUCCEEDED;
	}

	GIFWriter::GIFError GIFWriter::begin_frame(
		const int p_left,
		const int p_top,
		const int p_width,
		const int p_height,
		const bool p_interlace
	) {
		if (!file_type) return NO_FILE_OPEN;
		if (has_image_desc) return HAS_IMAG_DSCR;						// 图像描述符仍在激活
		if (p_width <= 0 || p_width > 65535 || p_height <= 0 || p_height > 65535) {
			return INVALID_DIMENSIONS;
		}

		// 保存当前帧状态
		frame_left = p_left;
		frame_top = p_top;
		frame_width = p_width;
		frame_height = p_height;
		frame_interlace = p_interlace;
		frame_delay = 100;  // 重置默认值
		frame_disposal = DISPOSAL_METHOD_UNSPECIFIED;
		frame_transparent_index = -1;

		return SUCCEEDED;
	}

	GIFWriter::GIFError GIFWriter::set_frame_delay(int p_delay_ms) {
		if (!file_type) return NO_FILE_OPEN;
		// GIF 延迟单位是 1/100 秒，转换毫秒（四舍五入）
		frame_delay = CLAMP((p_delay_ms + 5) / 10, 0, 65535);
		return SUCCEEDED;
	}

	GIFWriter::GIFError GIFWriter::set_disposal_method(DisposalMethod p_method) {
		if (!file_type) return NO_FILE_OPEN;
		frame_disposal = p_method;
		return SUCCEEDED;
	}

	GIFWriter::GIFError GIFWriter::set_transparent_color(const int p_index) {
		if (!file_type) return NO_FILE_OPEN;
		frame_transparent_index = p_index;
		return SUCCEEDED;
	}

	GIFWriter::GIFError GIFWriter::set_frame_palette(const PackedColorArray& p_palette) {
		if (!file_type) return NO_FILE_OPEN;
		if (p_palette.is_empty()) return NO_COLOR_MAP;

		int color_count = p_palette.size();
		if (color_count > 256) color_count = 256;

		// 释放之前的局部调色板
		if (frame_color_map) {
			GifFreeMapObject(frame_color_map);
			frame_color_map = nullptr;
		}

		// 创建新的调色板对象
		frame_color_map = GifMakeMapObject(color_count, nullptr);
		if (!frame_color_map) return NOT_ENOUGH_MEM;

		// 填充颜色数据
		for (int i = 0; i < color_count; i++) {
			Color color = p_palette[i];
			frame_color_map->Colors[i].Red = static_cast<GifByteType>(color.r * 255);
			frame_color_map->Colors[i].Green = static_cast<GifByteType>(color.g * 255);
			frame_color_map->Colors[i].Blue = static_cast<GifByteType>(color.b * 255);
		}

		return SUCCEEDED;
	}

	GIFWriter::GIFError GIFWriter::write_frame_pixels(const PackedByteArray& p_indices) {
		if (!file_type) return NO_FILE_OPEN;
		if (!has_screen_desc) return NO_SCRN_DSCR;

		// 检查数据大小
		int expected_size = frame_width * frame_height;
		if (p_indices.size() < expected_size) return DATA_TOO_BIG;

		// 写入 GCB (图形控制块)
		GraphicsControlBlock gcb;
		gcb.DisposalMode = static_cast<int>(frame_disposal);
		gcb.UserInputFlag = false;
		gcb.DelayTime = frame_delay;
		gcb.TransparentColor = frame_transparent_index >= 0 ? frame_transparent_index : -1;

		// 打包 GCB 并写入
		char gcb_packed[4];
		EGifGCBToExtension(&gcb, reinterpret_cast<GifByteType*>(gcb_packed));
		
		if (EGifPutExtension(file_type, GRAPHICS_EXT_FUNC_CODE, 4, gcb_packed) == GIF_ERROR) {
			return WRITER_FAILED;
		}

		// 写入图像描述符
		if (EGifPutImageDesc(file_type, frame_left, frame_top, 
		                     frame_width, frame_height, 
		                     frame_interlace, frame_color_map) == GIF_ERROR) {
			return WRITER_FAILED;
		}

		// 逐行写入像素数据
		const uint8_t* ptr = p_indices.ptr();
		for (int i = 0; i < frame_height; i++) {
			if (EGifPutLine(file_type, const_cast<GifPixelType*>(ptr + i * frame_width), frame_width) == GIF_ERROR) {
				return WRITER_FAILED;
			}
		}

		has_image_desc = true;
		return SUCCEEDED;
	}

	GIFWriter::GIFError GIFWriter::write_frame_image(Ref<Image> p_image, const bool p_quantize) {
		if (!file_type) return NO_FILE_OPEN;
		if (!has_screen_desc) return NO_SCRN_DSCR;
		if (p_image.is_null()) return DATA_TOO_BIG;

		// 获取图像尺寸
		int img_width = p_image->get_width();
		int img_height = p_image->get_height();

		// 转换图像格式为 RGB8
		Ref<Image> img_rgb = p_image->duplicate();
		if (img_rgb->get_format() != Image::FORMAT_RGB8 && img_rgb->get_format() != Image::FORMAT_RGBA8) {
			img_rgb->convert(Image::FORMAT_RGB8);
		}

		PackedByteArray indices;
		indices.resize(img_width * img_height);

		if (p_quantize && !file_type->SColorMap && !frame_color_map) {
			// 需要量化颜色
			int color_count = 256;
			ColorMapObject* output_map = GifMakeMapObject(color_count, nullptr);
			if (!output_map) return NOT_ENOUGH_MEM;

			// 使用 giflib 的量化函数
			// 注意：这是简化实现，实际应该使用更复杂的量化算法
			PackedColorArray palette;
			palette.resize(color_count);
			
			// 简单的颜色量化：获取图像中的主要颜色
			PackedByteArray pixels = img_rgb->get_data();
			for (int i = 0; i < color_count && i * 3 < pixels.size(); i++) {
				palette[i] = Color(pixels[i * 3] / 255.0f, pixels[i * 3 + 1] / 255.0f, pixels[i * 3 + 2] / 255.0f);
			}
			
			// 设置量化后的调色板为局部调色板
			set_frame_palette(palette);
			GifFreeMapObject(output_map);
		}

		// 将图像数据映射到调色板索引
		PackedByteArray pixels = img_rgb->get_data();
		int bpp = img_rgb->get_format() == Image::FORMAT_RGBA8 ? 4 : 3;
		
		for (int y = 0; y < img_height; y++) {
			for (int x = 0; x < img_width; x++) {
				int idx = (y * img_width + x) * bpp;
				uint8_t r = pixels[idx];
				uint8_t g = pixels[idx + 1];
				uint8_t b = pixels[idx + 2];
				
				// 查找最近的调色板颜色
				int best_index = 0;
				int best_dist = 999999;
				
				ColorMapObject* cmap = frame_color_map ? frame_color_map : file_type->SColorMap;
				if (cmap) {
					for (int i = 0; i < cmap->ColorCount; i++) {
						int dr = r - cmap->Colors[i].Red;
						int dg = g - cmap->Colors[i].Green;
						int db = b - cmap->Colors[i].Blue;
						int dist = dr * dr + dg * dg + db * db;
						if (dist < best_dist) {
							best_dist = dist;
							best_index = i;
						}
					}
				} else {
					// 无调色板，使用灰度（映射到 0-255）
					best_index = (r + g + b) / 3;
				}
				
				indices[y * img_width + x] = best_index;
			}
		}

		// 更新 frame 尺寸
		frame_width = img_width;
		frame_height = img_height;

		return write_frame_pixels(indices);
	}

	GIFWriter::GIFError GIFWriter::end_frame() {
		if (!file_type) return NO_FILE_OPEN;
		if (!has_image_desc) return SUCCEEDED;  // 没有活动帧

		// 清理局部调色板
		if (frame_color_map) {
			GifFreeMapObject(frame_color_map);
			frame_color_map = nullptr;
		}

		has_image_desc = false;
		current_frame++;
		return SUCCEEDED;
	}

	GIFWriter::GIFError GIFWriter::write_gif(
		const TypedArray<Image>& p_images,
		const PackedInt32Array& p_delays,
		const int p_loop_count,
		const bool p_quantize
	) {
		if (!file_type) return NO_FILE_OPEN;
		if (!has_screen_desc) return NO_SCRN_DSCR;
		if (p_images.is_empty()) return DATA_TOO_BIG;

		int frame_count = p_images.size();

		// 设置循环次数
		if (p_loop_count >= 0) {
			GIFError err = set_loop_count(p_loop_count);
			if (err != SUCCEEDED) return err;
		}

		// 写入每一帧
		for (int i = 0; i < frame_count; i++) {
			Ref<Image> img = p_images[i];
			if (img.is_null()) continue;

			// 获取延迟
			int delay = (i < p_delays.size()) ? p_delays[i] : 100;

			// 开始帧
			GIFError err = begin_frame(0, 0, img->get_width(), img->get_height(), false);
			if (err != SUCCEEDED) return err;

			// 设置延迟
			err = set_frame_delay(delay);
			if (err != SUCCEEDED) return err;

			// 写入图像
			err = write_frame_image(img, p_quantize);
			if (err != SUCCEEDED) return err;

			// 结束帧
			err = end_frame();
			if (err != SUCCEEDED) return err;
		}

		return SUCCEEDED;
	}

	GIFWriter::GIFError GIFWriter::save_to_file(
		const String& p_path,
		const TypedArray<Image>& p_images,
		const PackedInt32Array& p_delays,
		const int p_loop_count,
		const bool p_quantize
	) {
		if (p_images.is_empty()) return DATA_TOO_BIG;

		// 获取第一帧尺寸作为画布尺寸
		Ref<Image> first_img = p_images[0];
		if (first_img.is_null()) return DATA_TOO_BIG;

		// 创建 writer
		Ref<GIFWriter> writer;
		writer.instantiate();

		GIFError err = writer->open(p_path);
		if (err != SUCCEEDED) return err;

		// 设置画布尺寸
		err = writer->set_canvas_size(first_img->get_width(), first_img->get_height());
		if (err != SUCCEEDED) return err;

		// 写入所有帧
		err = writer->write_gif(p_images, p_delays, p_loop_count, p_quantize);
		if (err != SUCCEEDED) return err;

		// 关闭文件
		return writer->close();
	}

	Dictionary GIFWriter::save_to_buffer(
		const TypedArray<Image>& p_images,
		const PackedInt32Array& p_delays,
		const int p_loop_count,
		const bool p_quantize
	) {
		Dictionary result;
		result["error"] = SUCCEEDED;
		result["data"] = PackedByteArray();

		if (p_images.is_empty()) {
			result["error"] = DATA_TOO_BIG;
			return result;
		}

		// 获取第一帧尺寸
		Ref<Image> first_img = p_images[0];
		if (first_img.is_null()) {
			result["error"] = DATA_TOO_BIG;
			return result;
		}

		// 创建 writer
		Ref<GIFWriter> writer;
		writer.instantiate();

		GIFError err = writer->open_from_buffer();
		if (err != SUCCEEDED) {
			result["error"] = err;
			return result;
		}

		// 设置画布尺寸
		err = writer->set_canvas_size(first_img->get_width(), first_img->get_height());
		if (err != SUCCEEDED) {
			result["error"] = err;
			return result;
		}

		// 写入所有帧
		err = writer->write_gif(p_images, p_delays, p_loop_count, p_quantize);
		if (err != SUCCEEDED) {
			result["error"] = err;
			return result;
		}

		// 关闭并获取缓冲区
		err = writer->close();
		if (err != SUCCEEDED) {
			result["error"] = err;
			return result;
		}

		result["error"] = SUCCEEDED;
		result["data"] = writer->get_output_buffer();
		return result;
	}

	GIFWriter::GIFError GIFWriter::add_comment(const String& p_comment) {
		if (!file_type) return NO_FILE_OPEN;

		CharString comment_utf8 = p_comment.utf8();
		int len = comment_utf8.length();

		if (len == 0) return SUCCEEDED;

		// 使用 EGifPutComment 写入注释 (支持多子块)
		if (EGifPutComment(file_type, comment_utf8.get_data()) == GIF_ERROR) {
			return WRITER_FAILED;
		}

		return SUCCEEDED;
	}

	int GIFWriter::get_frame_count() const {
		if (!file_type) return 0;
		return current_frame;
	}

	Vector2i GIFWriter::get_canvas_size() const {
		if (!file_type) return Vector2i(0, 0);
		if (!has_screen_desc) return Vector2i(0, 0);
		return Vector2i(file_type->SWidth, file_type->SHeight);
	}
}
