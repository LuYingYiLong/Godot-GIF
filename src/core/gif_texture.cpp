#include "gif_texture.h"

#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/image.hpp>

namespace godot {
	void GIFTexture::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_data", "data"), &GIFTexture::set_data);
		ClassDB::bind_method(D_METHOD("get_data"), &GIFTexture::get_data);
		ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "data",
			PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
			"set_data", "get_data");

		ClassDB::bind_method(D_METHOD("set_frame", "frame"), &GIFTexture::set_frame);
		ClassDB::bind_method(D_METHOD("get_frame"), &GIFTexture::get_frame);
		ADD_PROPERTY(PropertyInfo(Variant::INT, "frame"), "set_frame", "get_frame");

		ClassDB::bind_method(D_METHOD("get_frame_count"), &GIFTexture::get_frame_count);

		ClassDB::bind_method(D_METHOD("get_frame_texture", "frame"), &GIFTexture::get_frame_texture);
		ClassDB::bind_method(D_METHOD("get_current_texture"), &GIFTexture::get_current_texture);

		ClassDB::bind_method(D_METHOD("get_frame_delay", "frame"), &GIFTexture::get_frame_delay);
		ClassDB::bind_method(D_METHOD("get_total_duration"), &GIFTexture::get_total_duration);
		ClassDB::bind_method(D_METHOD("get_loop_count"), &GIFTexture::get_loop_count);
	}

	GIFTexture::GIFTexture() {
		
	}

	GIFTexture::~GIFTexture() {

	}

	Ref<GIFTexture> GIFTexture::load_from_file(const String& p_path) {
		Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
		ERR_FAIL_COND_V_MSG(file.is_null(), Ref<GIFTexture>(), "Failed to open GIF file");
		PackedByteArray buffer = file->get_buffer(file->get_length());
		return load_from_buffer(buffer);
	}

	Ref<GIFTexture> GIFTexture::load_from_buffer(const PackedByteArray& p_buffer) {
		Ref<GIFReader> reader;
		reader.instantiate();
		GIFReader::GIFError err = reader->open_from_buffer(p_buffer);
		ERR_FAIL_COND_V_MSG(err != GIFReader::SUCCEEDED, Ref<GIFTexture>(), "Failed to open GIF data");

		Ref<GIFTexture> new_texture;
		new_texture.instantiate();
		new_texture->set_data(p_buffer);
		return new_texture;
	}

	int GIFTexture::_get_width() const {
		return size.x;
	}

	int GIFTexture::_get_height() const {
		return size.y;
	}

	bool GIFTexture::_has_alpha() const {
		if (current_frame >= 0 && current_frame < frame_count) {
			Ref<ImageTexture> tex = frames[current_frame];
			if (tex.is_valid()) return tex->has_alpha();
		}
		return true;
	}

	RID GIFTexture::_get_rid() const {
		if (current_frame >= 0 && current_frame < frame_count) {
			Ref<ImageTexture> tex = frames[current_frame];
			if (tex.is_valid()) return tex->get_rid();
		}
		return RID();
	}

	void GIFTexture::set_data(const PackedByteArray& p_data) {
		if (gif_data == p_data) {
			return;
		}
		gif_data = p_data;
		frames.clear();
		frame_delays.clear();

		Ref<GIFReader> reader;
		reader.instantiate();
		GIFReader::GIFError err = reader->open_from_buffer(gif_data);
		ERR_FAIL_COND_MSG(err != GIFReader::SUCCEEDED, "Failed to load GIF data");

		size = reader->get_size();
		loop_count = reader->get_loop_count();
		frame_count = reader->get_image_count();

		if (frame_count == 0) {
			return;
		}

		int64_t canvas_width = size.x;
		int64_t canvas_height = size.y;

		// 创建画布 (前一帧备份用于 DISPOSAL_METHOD_PREVIOUS)
		PackedByteArray canvas;
		canvas.resize(canvas_width * canvas_height * 4);
		PackedByteArray previous_canvas;
		previous_canvas.resize(canvas_width * canvas_height * 4);

		// 初始化画布为全透明
		memset(canvas.ptrw(), 0, canvas.size());

		for (int frame_idx = 0; frame_idx < frame_count; frame_idx++) {
			GIFFrameRawData frame_data = reader->get_frame_raw_data(frame_idx);

			if (frame_data.width <= 0 || frame_data.height <= 0) {
				// 空帧，保存当前画布
				Ref<Image> img = Image::create_from_data(canvas_width, canvas_height, false, Image::FORMAT_RGBA8, canvas);
				Ref<ImageTexture> tex = ImageTexture::create_from_image(img);
				frames.append(tex);
				frame_delays.append(frame_data.delay_ms / 1000.0f);
				continue;
			}

			// 只有在需要恢复到前一帧时才备份
			// 注意: 备份的是绘制当前帧之前的画布状态
			if (frame_data.disposal_method == GIF_DISPOSAL_PREVIOUS) {
				memcpy(previous_canvas.ptrw(), canvas.ptr(), canvas.size());
			}

			// 获取指针以提高性能
			uint8_t* canvas_ptr = canvas.ptrw();
			const uint8_t* raster_ptr = frame_data.pixel_indices.ptr();
			const Color* palette_ptr = frame_data.palette.ptr();

			// 将当前帧绘制到画布
			for (int y = 0; y < frame_data.height; y++) {
				int canvas_y = frame_data.top + y;
				if (canvas_y < 0 || canvas_y >= canvas_height) continue;

				for (int x = 0; x < frame_data.width; x++) {
					int canvas_x = frame_data.left + x;
					if (canvas_x < 0 || canvas_x >= canvas_width) continue;

					int raster_idx = y * frame_data.width + x;
					int color_idx = raster_ptr[raster_idx];

					// 透明色：保持画布原样 (跳过)
					// 透明色的含义是 "这个位置不要动"，而不是 "设为透明"
					if (color_idx == frame_data.transparent_color) {
						continue;
					}

					// 安全检查：颜色索引必须在有效范围内
					if (color_idx >= 0 && color_idx < frame_data.color_count && color_idx < frame_data.palette.size()) {
						int canvas_idx = (canvas_y * canvas_width + canvas_x) * 4;
						const Color& c = palette_ptr[color_idx];
						canvas_ptr[canvas_idx + 0] = (uint8_t)(c.r * 255);
						canvas_ptr[canvas_idx + 1] = (uint8_t)(c.g * 255);
						canvas_ptr[canvas_idx + 2] = (uint8_t)(c.b * 255);
						canvas_ptr[canvas_idx + 3] = 255;			// 不透明
					}
				}
			}

			// 保存当前合成帧
			Ref<Image> img = Image::create_from_data(canvas_width, canvas_height, false, Image::FORMAT_RGBA8, canvas);
			Ref<ImageTexture> tex = ImageTexture::create_from_image(img);
			frames.append(tex);
			frame_delays.append(frame_data.delay_ms / 1000.0f);

			// 根据处置方式为下一帧准备画布
			if (frame_data.disposal_method == GIF_DISPOSAL_BACKGROUND) {
				// 将当前帧占据的矩形区域清除为透明 (不是背景色！)
				for (int y = 0; y < frame_data.height; y++) {
					int cy = frame_data.top + y;
					if (cy < 0 || cy >= canvas_height) continue;
					for (int x = 0; x < frame_data.width; x++) {
						int cx = frame_data.left + x;
						if (cx < 0 || cx >= canvas_width) continue;
						int idx = (cy * canvas_width + cx) * 4;
						canvas_ptr[idx + 0] = 0;
						canvas_ptr[idx + 1] = 0;
						canvas_ptr[idx + 2] = 0;
						canvas_ptr[idx + 3] = 0;
					}
				}
			} else if (frame_data.disposal_method == GIF_DISPOSAL_PREVIOUS) {
				// 恢复到绘制当前帧之前的状态
				memcpy(canvas.ptrw(), previous_canvas.ptr(), canvas.size());
			}
			// DISPOSAL_METHOD_DO_NOT / UNSPECIFIED: 保持画布不变
		}
	}

	PackedByteArray GIFTexture::get_data() const {
		return gif_data;
	}

	void GIFTexture::set_frame(int p_frame) {
		ERR_FAIL_INDEX(p_frame, frame_count);
		if (current_frame == p_frame) return;
		current_frame = p_frame;
		emit_changed();
	}

	int GIFTexture::get_frame() const {
		return current_frame;
	}

	int GIFTexture::get_frame_count() const {
		return frame_count;
	}

	Ref<ImageTexture> GIFTexture::get_frame_texture(int p_frame) const {
		ERR_FAIL_INDEX_V(p_frame, frame_count, Ref<ImageTexture>());
		return frames[p_frame];
	}

	Ref<ImageTexture> GIFTexture::get_current_texture() const {
		if (current_frame >= 0 && current_frame < frame_count) {
			return frames[current_frame];
		}
		return Ref<ImageTexture>();
	}

	float GIFTexture::get_frame_delay(int p_frame) const {
		ERR_FAIL_INDEX_V(p_frame, frame_count, 0.1f);
		return frame_delays[p_frame];
	}

	float GIFTexture::get_total_duration() const {
		float total = 0.0f;
		for (int i = 0; i < frame_count; i++) {
			total += frame_delays[i];
		}
		return total;
	}

	int GIFTexture::get_loop_count() const {
		return loop_count;
	}
}
