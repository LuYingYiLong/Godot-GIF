#include "gif_texture.h"

#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/classes/file_access.hpp>

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
		TypedArray<Image> images = reader->get_saved_images();
		frame_count = images.size();
		for (int i = 0; i < images.size(); i++) {
			Ref<Image> img = images[i];
			Ref<ImageTexture> image_texture = ImageTexture::create_from_image(img);
			frames.append(image_texture);
			float delay = reader->get_frame_delay(i);
			frame_delays.append(delay);
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
