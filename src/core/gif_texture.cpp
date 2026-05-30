#include "gif_texture.h"

#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/image.hpp>

namespace godot {
	void GIFTexture::_bind_methods() {
		ClassDB::bind_static_method("GIFTexture", D_METHOD("load_from_file", "path"), &GIFTexture::load_from_file);
		ClassDB::bind_static_method("GIFTexture", D_METHOD("load_from_buffer", "buffer"), &GIFTexture::load_from_buffer);

		ClassDB::bind_method(D_METHOD("set_data", "data"), &GIFTexture::set_data);
		ClassDB::bind_method(D_METHOD("get_data"), &GIFTexture::get_data);
		ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "data",
			PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
			"set_data", "get_data");

		ClassDB::bind_method(D_METHOD("set_metadata_size", "size"), &GIFTexture::set_metadata_size);
		ClassDB::bind_method(D_METHOD("get_metadata_size"), &GIFTexture::get_metadata_size);
		ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "metadata/size",
			PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
			"set_metadata_size", "get_metadata_size");

		ClassDB::bind_method(D_METHOD("set_metadata_frame_count", "frame_count"), &GIFTexture::set_metadata_frame_count);
		ClassDB::bind_method(D_METHOD("get_metadata_frame_count"), &GIFTexture::get_metadata_frame_count);
		ADD_PROPERTY(PropertyInfo(Variant::INT, "metadata/frame_count",
			PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
			"set_metadata_frame_count", "get_metadata_frame_count");

		ClassDB::bind_method(D_METHOD("set_metadata_frame_delays", "frame_delays"), &GIFTexture::set_metadata_frame_delays);
		ClassDB::bind_method(D_METHOD("get_metadata_frame_delays"), &GIFTexture::get_metadata_frame_delays);
		ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "metadata/frame_delays",
			PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
			"set_metadata_frame_delays", "get_metadata_frame_delays");

		ClassDB::bind_method(D_METHOD("set_metadata_loop_count", "loop_count"), &GIFTexture::set_metadata_loop_count);
		ClassDB::bind_method(D_METHOD("get_metadata_loop_count"), &GIFTexture::get_metadata_loop_count);
		ADD_PROPERTY(PropertyInfo(Variant::INT, "metadata/loop_count",
			PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
			"set_metadata_loop_count", "get_metadata_loop_count");

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
		Ref<GIFTexture> new_texture;
		new_texture.instantiate();
		new_texture->set_data(p_buffer);
		ERR_FAIL_COND_V_MSG(!new_texture->_ensure_metadata(), Ref<GIFTexture>(), "Failed to open GIF data");
		return new_texture;
	}

	int GIFTexture::_get_width() const {
		_ensure_metadata();
		return size.x;
	}

	int GIFTexture::_get_height() const {
		_ensure_metadata();
		return size.y;
	}

	bool GIFTexture::_has_alpha() const {
		if (_ensure_frame_decoded(current_frame)) {
			Ref<ImageTexture> tex = frames[current_frame];
			if (tex.is_valid()) return tex->has_alpha();
		}
		return true;
	}

	RID GIFTexture::_get_rid() const {
		if (_ensure_frame_decoded(current_frame)) {
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
		frame_delays.clear();
		size = Size2i();
		frame_count = 0;
		loop_count = 0;
		current_frame = 0;
		_reset_runtime_cache();
		emit_changed();
	}

	void GIFTexture::_reset_runtime_cache() const {
		lazy_reader.unref();
		frames.clear();
		canvas.clear();
		previous_canvas.clear();
		decoded_frame_count = 0;
	}

	bool GIFTexture::_ensure_metadata() const {
		if (size.x > 0 && size.y > 0 && frame_count > 0) {
			return true;
		}

		if (gif_data.is_empty()) {
			return false;
		}

		Ref<GIFReader> reader;
		reader.instantiate();
		GIFReader::GIFError err = reader->open_from_buffer(gif_data);
		if (err != GIFReader::SUCCEEDED) {
			return false;
		}

		size = reader->get_size();
		loop_count = reader->get_loop_count();
		frame_count = reader->get_image_count();
		frame_delays.clear();
		frame_delays.resize(frame_count);
		for (int frame_idx = 0; frame_idx < frame_count; frame_idx++) {
			frame_delays[frame_idx] = reader->get_frame_delay(frame_idx) / 1000.0f;
		}

		return size.x > 0 && size.y > 0 && frame_count > 0;
	}

	bool GIFTexture::_ensure_decoder() const {
		if (lazy_reader.is_valid()) {
			return true;
		}

		if (!_ensure_metadata()) {
			return false;
		}

		lazy_reader.instantiate();
		GIFReader::GIFError err = lazy_reader->open_from_buffer(gif_data);
		if (err != GIFReader::SUCCEEDED) {
			lazy_reader.unref();
			return false;
		}

		frames.clear();
		frames.resize(frame_count);

		int64_t canvas_size = static_cast<int64_t>(size.x) * static_cast<int64_t>(size.y) * 4;
		canvas.resize(canvas_size);
		previous_canvas.resize(canvas_size);
		memset(canvas.ptrw(), 0, canvas.size());
		memset(previous_canvas.ptrw(), 0, previous_canvas.size());
		decoded_frame_count = 0;

		return true;
	}

	bool GIFTexture::_ensure_frame_decoded(int p_frame) const {
		if (!_ensure_metadata()) {
			return false;
		}

		ERR_FAIL_INDEX_V(p_frame, frame_count, false);

		if (frames.size() == frame_count) {
			Ref<ImageTexture> cached = frames[p_frame];
			if (cached.is_valid()) {
				return true;
			}
		}

		if (!_ensure_decoder()) {
			return false;
		}

		if (p_frame < decoded_frame_count) {
			_reset_runtime_cache();
			if (!_ensure_decoder()) {
				return false;
			}
		}

		for (int frame_idx = decoded_frame_count; frame_idx <= p_frame; frame_idx++) {
			_decode_frame_to_cache(frame_idx);
			decoded_frame_count = frame_idx + 1;
		}

		Ref<ImageTexture> decoded = frames[p_frame];
		return decoded.is_valid();
	}

	void GIFTexture::_decode_frame_to_cache(int p_frame) const {
		GIFFrameRawData frame_data = lazy_reader->get_frame_raw_data(p_frame);

		if (frame_data.width <= 0 || frame_data.height <= 0) {
			Ref<Image> img = Image::create_from_data(size.x, size.y, false, Image::FORMAT_RGBA8, canvas);
			Ref<ImageTexture> tex = ImageTexture::create_from_image(img);
			frames[p_frame] = tex;
			return;
		}

		if (frame_delays.size() < frame_count) {
			frame_delays.resize(frame_count);
		}
		frame_delays[p_frame] = frame_data.delay_ms / 1000.0f;

		if (frame_data.disposal_method == GIF_DISPOSAL_PREVIOUS) {
			memcpy(previous_canvas.ptrw(), canvas.ptr(), canvas.size());
		}

		uint8_t* canvas_ptr = canvas.ptrw();
		const uint8_t* raster_ptr = frame_data.pixel_indices.ptr();
		const Color* palette_ptr = frame_data.palette.ptr();

		for (int y = 0; y < frame_data.height; y++) {
			int canvas_y = frame_data.top + y;
			if (canvas_y < 0 || canvas_y >= size.y) continue;

			for (int x = 0; x < frame_data.width; x++) {
				int canvas_x = frame_data.left + x;
				if (canvas_x < 0 || canvas_x >= size.x) continue;

				int raster_idx = y * frame_data.width + x;
				int color_idx = raster_ptr[raster_idx];

				if (color_idx == frame_data.transparent_color) {
					continue;
				}

				if (color_idx >= 0 && color_idx < frame_data.color_count && color_idx < frame_data.palette.size()) {
					int canvas_idx = (canvas_y * size.x + canvas_x) * 4;
					const Color& c = palette_ptr[color_idx];
					canvas_ptr[canvas_idx + 0] = static_cast<uint8_t>(c.r * 255);
					canvas_ptr[canvas_idx + 1] = static_cast<uint8_t>(c.g * 255);
					canvas_ptr[canvas_idx + 2] = static_cast<uint8_t>(c.b * 255);
					canvas_ptr[canvas_idx + 3] = 255;
				}
			}
		}

		Ref<Image> img = Image::create_from_data(size.x, size.y, false, Image::FORMAT_RGBA8, canvas);
		Ref<ImageTexture> tex = ImageTexture::create_from_image(img);
		frames[p_frame] = tex;

		if (frame_data.disposal_method == GIF_DISPOSAL_BACKGROUND) {
			for (int y = 0; y < frame_data.height; y++) {
				int cy = frame_data.top + y;
				if (cy < 0 || cy >= size.y) continue;
				for (int x = 0; x < frame_data.width; x++) {
					int cx = frame_data.left + x;
					if (cx < 0 || cx >= size.x) continue;
					int idx = (cy * size.x + cx) * 4;
					canvas_ptr[idx + 0] = 0;
					canvas_ptr[idx + 1] = 0;
					canvas_ptr[idx + 2] = 0;
					canvas_ptr[idx + 3] = 0;
				}
			}
		} else if (frame_data.disposal_method == GIF_DISPOSAL_PREVIOUS) {
			memcpy(canvas.ptrw(), previous_canvas.ptr(), canvas.size());
		}
	}

	void GIFTexture::set_metadata_size(const Vector2i& p_size) {
		if (size == p_size) {
			return;
		}
		size = p_size;
		_reset_runtime_cache();
		emit_changed();
	}

	Vector2i GIFTexture::get_metadata_size() const {
		_ensure_metadata();
		return size;
	}

	void GIFTexture::set_metadata_frame_count(int p_frame_count) {
		p_frame_count = MAX(p_frame_count, 0);
		if (frame_count == p_frame_count) {
			return;
		}
		frame_count = p_frame_count;
		_reset_runtime_cache();
		emit_changed();
	}

	int GIFTexture::get_metadata_frame_count() const {
		_ensure_metadata();
		return frame_count;
	}

	void GIFTexture::set_metadata_frame_delays(const PackedFloat32Array& p_frame_delays) {
		frame_delays = p_frame_delays;
	}

	PackedFloat32Array GIFTexture::get_metadata_frame_delays() const {
		_ensure_metadata();
		return frame_delays;
	}

	void GIFTexture::set_metadata_loop_count(int p_loop_count) {
		loop_count = p_loop_count;
	}

	int GIFTexture::get_metadata_loop_count() const {
		_ensure_metadata();
		return loop_count;
	}

	void GIFTexture::set_metadata(const Vector2i& p_size, int p_frame_count, const PackedFloat32Array& p_frame_delays, int p_loop_count) {
		size = p_size;
		frame_count = MAX(p_frame_count, 0);
		frame_delays = p_frame_delays;
		loop_count = p_loop_count;
		_reset_runtime_cache();
		emit_changed();
	}

	PackedByteArray GIFTexture::get_data() const {
		return gif_data;
	}

	void GIFTexture::set_frame(int p_frame) {
		_ensure_metadata();
		ERR_FAIL_INDEX(p_frame, frame_count);
		if (current_frame == p_frame) return;
		current_frame = p_frame;
		emit_changed();
	}

	int GIFTexture::get_frame() const {
		return current_frame;
	}

	int GIFTexture::get_frame_count() const {
		_ensure_metadata();
		return frame_count;
	}

	Ref<ImageTexture> GIFTexture::get_frame_texture(int p_frame) const {
		_ensure_metadata();
		ERR_FAIL_INDEX_V(p_frame, frame_count, Ref<ImageTexture>());
		if (!_ensure_frame_decoded(p_frame)) {
			return Ref<ImageTexture>();
		}
		return frames[p_frame];
	}

	Ref<ImageTexture> GIFTexture::get_current_texture() const {
		_ensure_metadata();
		if (current_frame >= 0 && current_frame < frame_count) {
			if (!_ensure_frame_decoded(current_frame)) {
				return Ref<ImageTexture>();
			}
			return frames[current_frame];
		}
		return Ref<ImageTexture>();
	}

	float GIFTexture::get_frame_delay(int p_frame) const {
		_ensure_metadata();
		ERR_FAIL_INDEX_V(p_frame, frame_count, 0.1f);
		if (p_frame >= frame_delays.size()) {
			return 0.1f;
		}
		return frame_delays[p_frame];
	}

	float GIFTexture::get_total_duration() const {
		_ensure_metadata();
		float total = 0.0f;
		for (int i = 0; i < frame_count; i++) {
			total += (i < frame_delays.size()) ? frame_delays[i] : 0.1f;
		}
		return total;
	}

	int GIFTexture::get_loop_count() const {
		_ensure_metadata();
		return loop_count;
	}
}
