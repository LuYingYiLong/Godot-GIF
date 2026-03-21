#include "gif_player.h"

#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/classes/time.hpp>

namespace godot {
	void GIFPlayer::_bind_methods() {
		BIND_ENUM_CONSTANT(PLAY_MODE_FORWARD);
		BIND_ENUM_CONSTANT(PLAY_MODE_BACKWARD);
		BIND_ENUM_CONSTANT(PLAY_MODE_PINGPONG);
		BIND_ENUM_CONSTANT(PLAY_MODE_RANDOM);

		BIND_ENUM_CONSTANT(EXPAND_KEEP_SIZE);
		BIND_ENUM_CONSTANT(EXPAND_IGNORE_SIZE);
		BIND_ENUM_CONSTANT(EXPAND_FIT_WIDTH);
		BIND_ENUM_CONSTANT(EXPAND_FIT_WIDTH_PROPORTIONAL);
		BIND_ENUM_CONSTANT(EXPAND_FIT_HEIGHT);
		BIND_ENUM_CONSTANT(EXPAND_FIT_HEIGHT_PROPORTIONAL);

		BIND_ENUM_CONSTANT(STRETCH_SCALE);
		BIND_ENUM_CONSTANT(STRETCH_TILE);
		BIND_ENUM_CONSTANT(STRETCH_KEEP);
		BIND_ENUM_CONSTANT(STRETCH_KEEP_CENTERED);
		BIND_ENUM_CONSTANT(STRETCH_KEEP_ASPECT);
		BIND_ENUM_CONSTANT(STRETCH_KEEP_ASPECT_CENTERED);
		BIND_ENUM_CONSTANT(STRETCH_KEEP_ASPECT_COVERED);

		ClassDB::bind_method(D_METHOD("set_gif", "gif"), &GIFPlayer::set_gif);
		ClassDB::bind_method(D_METHOD("get_gif"), &GIFPlayer::get_gif);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "gif", PROPERTY_HINT_RESOURCE_TYPE, "GIFTexture"), "set_gif", "get_gif");

		ClassDB::bind_method(D_METHOD("play"), &GIFPlayer::play);

		ClassDB::bind_method(D_METHOD("set_playing", "playing"), &GIFPlayer::set_playing);
		ClassDB::bind_method(D_METHOD("is_playing"), &GIFPlayer::is_playing);
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "playing"), "set_playing", "is_playing");

		ClassDB::bind_method(D_METHOD("set_paused", "paused"), &GIFPlayer::set_paused);
		ClassDB::bind_method(D_METHOD("is_paused"), &GIFPlayer::is_paused);
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "paused"), "set_paused", "is_paused");

		ClassDB::bind_method(D_METHOD("stop"), &GIFPlayer::stop);

		ClassDB::bind_method(D_METHOD("set_frame", "frame"), &GIFPlayer::set_frame);
		ClassDB::bind_method(D_METHOD("get_frame"), &GIFPlayer::get_frame);
		ClassDB::bind_method(D_METHOD("get_frame_count"), &GIFPlayer::get_frame_count);

		ClassDB::bind_method(D_METHOD("set_play_mode", "mode"), &GIFPlayer::set_play_mode);
		ClassDB::bind_method(D_METHOD("get_play_mode"), &GIFPlayer::get_play_mode);
		ADD_PROPERTY(PropertyInfo(Variant::INT, "play_mode", PROPERTY_HINT_ENUM, "Forward,Backward,PingPong,Random"), "set_play_mode", "get_play_mode");

		ClassDB::bind_method(D_METHOD("set_speed_scale", "speed"), &GIFPlayer::set_speed_scale);
		ClassDB::bind_method(D_METHOD("get_speed_scale"), &GIFPlayer::get_speed_scale);
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed_scale", PROPERTY_HINT_RANGE, "0.0,10.0,0.1"), "set_speed_scale", "get_speed_scale");

		ClassDB::bind_method(D_METHOD("set_loop_count", "count"), &GIFPlayer::set_loop_count);
		ClassDB::bind_method(D_METHOD("get_loop_count"), &GIFPlayer::get_loop_count);
		ADD_PROPERTY(PropertyInfo(Variant::INT, "loop_count", PROPERTY_HINT_RANGE, "-1,100,1,or_greater"), "set_loop_count", "get_loop_count");

		ClassDB::bind_method(D_METHOD("set_expand_mode", "mode"), &GIFPlayer::set_expand_mode);
		ClassDB::bind_method(D_METHOD("get_expand_mode"), &GIFPlayer::get_expand_mode);
		ADD_PROPERTY(PropertyInfo(Variant::INT, "expand_mode", PROPERTY_HINT_ENUM, "Keep Size,Ignore Size,Fit Width,Fit Width Proportional,Fit Height,Fit Height Proportional"), "set_expand_mode", "get_expand_mode");

		ClassDB::bind_method(D_METHOD("set_stretch_mode", "mode"), &GIFPlayer::set_stretch_mode);
		ClassDB::bind_method(D_METHOD("get_stretch_mode"), &GIFPlayer::get_stretch_mode);
		ADD_PROPERTY(PropertyInfo(Variant::INT, "stretch_mode", PROPERTY_HINT_ENUM, "Scale,Tile,Keep,Keep Centered,Keep Aspect,Keep Aspect Centered,Keep Aspect Covered"), "set_stretch_mode", "get_stretch_mode");
		
		ClassDB::bind_method(D_METHOD("set_flip_h", "enable"), &GIFPlayer::set_flip_h);
		ClassDB::bind_method(D_METHOD("is_flipped_h"), &GIFPlayer::is_flipped_h);
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flip_h"), "set_flip_h", "is_flipped_h");

		ClassDB::bind_method(D_METHOD("set_flip_v", "enable"), &GIFPlayer::set_flip_v);
		ClassDB::bind_method(D_METHOD("is_flipped_v"), &GIFPlayer::is_flipped_v);
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flip_v"), "set_flip_v", "is_flipped_v");
	}

	GIFPlayer::GIFPlayer() {
		set_process(true);
	}

	void GIFPlayer::_notification(int p_what) {
		switch (p_what) {
			case NOTIFICATION_DRAW: {
				if (gif.is_null() || gif->get_frame_count() == 0) return;

				Ref<Texture2D> texture = gif->get_current_texture();
				if (texture.is_null()) return;

				Size2 size;
				Point2 offset;
				Rect2 region;
				bool tile = false;

				switch (stretch_mode) {
				case STRETCH_SCALE: {
					size = get_size();
				} break;
				case STRETCH_TILE: {
					size = get_size();
					tile = true;
				} break;
				case STRETCH_KEEP: {
					size = texture->get_size();
				} break;
				case STRETCH_KEEP_CENTERED: {
					offset = (get_size() - texture->get_size()) / 2;
					size = texture->get_size();
				} break;
				case STRETCH_KEEP_ASPECT_CENTERED:
				case STRETCH_KEEP_ASPECT: {
					size = get_size();
					int tex_width = texture->get_width() * size.height / texture->get_height();
					int tex_height = size.height;

					if (tex_width > size.width) {
						tex_width = size.width;
						tex_height = texture->get_height() * tex_width / texture->get_width();
					}

					if (stretch_mode == STRETCH_KEEP_ASPECT_CENTERED) {
						offset.x += (size.width - tex_width) / 2;
						offset.y += (size.height - tex_height) / 2;
					}

					size.width = tex_width;
					size.height = tex_height;
				} break;
				case STRETCH_KEEP_ASPECT_COVERED: {
					size = get_size();

					Size2 tex_size = texture->get_size();
					Size2 scale_size(size.width / tex_size.width, size.height / tex_size.height);
					float scale = scale_size.width > scale_size.height ? scale_size.width : scale_size.height;
					Size2 scaled_tex_size = tex_size * scale;

					region.position = ((scaled_tex_size - size) / scale).abs() / 2.0f;
					region.size = size / scale;
				} break;
				}

				size.width *= hflip ? -1.0f : 1.0f;
				size.height *= vflip ? -1.0f : 1.0f;

				if (region.has_area()) {
					draw_texture_rect_region(texture, Rect2(offset, size), region);
				}
				else {
					draw_texture_rect(texture, Rect2(offset, size), tile);
				}
			} break;

			case NOTIFICATION_PROCESS: {
				if (!playing || paused || gif.is_null()) return;
				if (gif->get_frame_count() <= 1) return;

				float delta = get_process_delta_time();
				time_accumulator += delta * speed_scale;

				// get_frame_delay 返回秒
				float current_delay = gif->get_frame_delay(current_frame);
				if (current_delay <= 0.0f) current_delay = 0.1f;

				while (time_accumulator >= current_delay) {
					time_accumulator -= current_delay;
					_advance_frame();
					if (!playing) break;
					current_delay = gif->get_frame_delay(current_frame);
					if (current_delay <= 0.0f) current_delay = 0.1f;
				}
			} break;

			case NOTIFICATION_RESIZED: {
				update_minimum_size();
			} break;
		}
	}

	Vector2 GIFPlayer::_get_minimum_size() const {
		if (gif.is_valid()) {
			switch (expand_mode) {
			case EXPAND_KEEP_SIZE: {
				return gif->get_size();
			} break;
			case EXPAND_IGNORE_SIZE: {
				return Size2();
			} break;
			case EXPAND_FIT_WIDTH: {
				return Size2(get_size().y, 0);
			} break;
			case EXPAND_FIT_WIDTH_PROPORTIONAL: {
				real_t ratio = real_t(gif->get_width()) / gif->get_height();
				return Size2(get_size().y * ratio, 0);
			} break;
			case EXPAND_FIT_HEIGHT: {
				return Size2(0, get_size().x);
			} break;
			case EXPAND_FIT_HEIGHT_PROPORTIONAL: {
				real_t ratio = real_t(gif->get_height()) / gif->get_width();
				return Size2(0, get_size().x * ratio);
			} break;
			}
		}
		return Size2();
	}

	void GIFPlayer::_advance_frame() {
		int frame_count = gif->get_frame_count();
		if (frame_count <= 1) return;

		int effective_loop = (loop_count < 0) ? gif->get_loop_count() : loop_count;

		switch (play_mode) {
			case PLAY_MODE_FORWARD: {
				current_frame++;
				if (current_frame >= frame_count) {
					current_loop++;
					if (effective_loop > 0 && current_loop >= effective_loop) {
						current_frame = frame_count - 1;
						playing = false;
					} else {
						current_frame = 0;
					}
				}
			} break;

			case PLAY_MODE_BACKWARD: {
				current_frame--;
				if (current_frame < 0) {
					current_loop++;
					if (effective_loop > 0 && current_loop >= effective_loop) {
						current_frame = 0;
						playing = false;
					} else {
						current_frame = frame_count - 1;
					}
				}
			} break;

			case PLAY_MODE_PINGPONG: {
				if (pingpong_forward) {
					current_frame++;
					if (current_frame >= frame_count) {
						current_frame = frame_count - 2;
						pingpong_forward = false;
						if (current_frame < 0) current_frame = 0;
					}
				} else {
					current_frame--;
					if (current_frame < 0) {
						current_frame = 1;
						pingpong_forward = true;
						current_loop++;
						if (effective_loop > 0 && current_loop >= effective_loop) {
							current_frame = 0;
							playing = false;
						}
					}
				}
			} break;

			case PLAY_MODE_RANDOM: {
				current_frame = rand() % frame_count;
			} break;
		}

		_update_display();
	}

	void GIFPlayer::_update_display() {
		if (gif.is_valid()) {
			gif->set_frame(current_frame);
			queue_redraw();
		}
	}

	void GIFPlayer::set_gif(const Ref<GIFTexture>& p_gif) {
		gif = p_gif;
		current_frame = 0;
		current_loop = 0;
		pingpong_forward = true;
		time_accumulator = 0.0f;
		if (gif.is_valid()) {
			gif->set_frame(0);
		}
		update_minimum_size();
		queue_redraw();
	}

	Ref<GIFTexture> GIFPlayer::get_gif() const {
		return gif;
	}

	void GIFPlayer::play() {
		if (gif.is_null() || gif->get_frame_count() == 0) return;
		playing = true;
		paused = false;
		time_accumulator = 0.0f;
	}

	void GIFPlayer::set_playing(const bool enable) {
		if (enable) {
			play();
		} else {
			stop();
		}
	}

	bool GIFPlayer::is_playing() const {
		return playing;
	}


	void GIFPlayer::set_paused(const bool enable) {
		paused = enable;
	}

	bool GIFPlayer::is_paused() const {
		return playing && paused;
	}

	void GIFPlayer::stop() {
		playing = false;
		paused = false;
		current_frame = 0;
		current_loop = 0;
		pingpong_forward = true;
		time_accumulator = 0.0f;
		_update_display();
	}

	void GIFPlayer::set_frame(int p_frame) {
		if (gif.is_null()) return;
		ERR_FAIL_INDEX(p_frame, gif->get_frame_count());
		current_frame = p_frame;
		time_accumulator = 0.0f;
		_update_display();
	}

	int GIFPlayer::get_frame() const {
		return current_frame;
	}

	int GIFPlayer::get_frame_count() const {
		if (gif.is_null()) return 0;
		return gif->get_frame_count();
	}

	void GIFPlayer::set_play_mode(PlayMode p_mode) {
		play_mode = p_mode;
	}

	GIFPlayer::PlayMode GIFPlayer::get_play_mode() const {
		return play_mode;
	}

	void GIFPlayer::set_speed_scale(float p_speed) {
		speed_scale = MAX(p_speed, 0.0f);
	}

	float GIFPlayer::get_speed_scale() const {
		return speed_scale;
	}

	void GIFPlayer::set_loop_count(int p_count) {
		loop_count = p_count;
	}

	int GIFPlayer::get_loop_count() const {
		return loop_count;
	}

	void GIFPlayer::set_expand_mode(ExpandMode p_mode) {
		expand_mode = p_mode;
	}

	GIFPlayer::ExpandMode GIFPlayer::get_expand_mode() const {
		return expand_mode;
	}

	void GIFPlayer::set_stretch_mode(StretchMode p_mode) {
		stretch_mode = p_mode;
		queue_redraw();
	}

	GIFPlayer::StretchMode GIFPlayer::get_stretch_mode() const {
		return stretch_mode;
	}

	void GIFPlayer::set_flip_h(bool p_flip) {
		if (hflip == p_flip) {
			return;
		}

		hflip = p_flip;
		queue_redraw();
	}

	bool GIFPlayer::is_flipped_h() const {
		return hflip;
	}

	void GIFPlayer::set_flip_v(bool p_flip) {
		if (vflip == p_flip) {
			return;
		}

		vflip = p_flip;
		queue_redraw();
	}

	bool GIFPlayer::is_flipped_v() const {
		return vflip;
	}
}
