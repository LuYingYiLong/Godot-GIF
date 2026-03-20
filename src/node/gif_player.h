#ifndef GIF_PLAYER_H
#define GIF_PLAYER_H

#include "../core/gif_texture.h"
#include <godot_cpp/classes/control.hpp>

namespace godot {
	class GIFPlayer : public Control {
		GDCLASS(GIFPlayer, Control)

	public:
		enum PlayMode {
			PLAY_MODE_FORWARD,											// 正序播放
			PLAY_MODE_BACKWARD,											// 倒序播放
			PLAY_MODE_PINGPONG,											// 乒乓播放
			PLAY_MODE_RANDOM											// 随机播放
		};

		enum ExpandMode {
			EXPAND_KEEP_SIZE,
			EXPAND_IGNORE_SIZE,
			EXPAND_FIT_WIDTH,
			EXPAND_FIT_WIDTH_PROPORTIONAL,
			EXPAND_FIT_HEIGHT,
			EXPAND_FIT_HEIGHT_PROPORTIONAL,
		};

		enum StretchMode {
			STRETCH_SCALE,												// 拉伸填满
			STRETCH_TILE,												// 平铺
			STRETCH_KEEP,												// 保持原尺寸
			STRETCH_KEEP_CENTERED,										// 居中保持
			STRETCH_KEEP_ASPECT,										// 保持比例适应
			STRETCH_KEEP_ASPECT_CENTERED,								// 居中保持比例
			STRETCH_KEEP_ASPECT_COVERED,								// 保持比例覆盖
		};

	private:
		Ref<GIFTexture> gif;

		// 播放状态
		bool playing = true;
		bool paused = false;
		int current_frame = 0;
		int current_loop = 0;
		float time_accumulator = 0.0f;
		bool pingpong_forward = true;

		// 播放设置
		PlayMode play_mode = PLAY_MODE_FORWARD;
		float speed_scale = 1.0f;
		int loop_count = 0;

		// 显示设置
		bool hflip = false;
		bool vflip = false;
		ExpandMode expand_mode = EXPAND_KEEP_SIZE;
		StretchMode stretch_mode = STRETCH_KEEP;

	protected:
		static void _bind_methods();
		void _notification(int p_what);

		void _advance_frame();
		void _update_display();

	public:
		GIFPlayer();

		virtual Vector2 _get_minimum_size() const override;

		// 设置 GIF 资源
		void set_gif(const Ref<GIFTexture>& p_gif);
		Ref<GIFTexture> get_gif() const;

		// 播放控制
		void play();

		void set_playing(const bool enable);
		bool is_playing() const;

		void set_paused(const bool enable);
		bool is_paused() const;

		void stop();

		// 帧控制
		void set_frame(int p_frame);
		int get_frame() const;
		int get_frame_count() const;

		// 播放设置
		void set_play_mode(PlayMode p_mode);
		PlayMode get_play_mode() const;

		void set_speed_scale(float p_speed);
		float get_speed_scale() const;

		void set_loop_count(int p_count);
		int get_loop_count() const;

		// 显示设置
		void set_expand_mode(ExpandMode p_mode);
		ExpandMode get_expand_mode() const;

		void set_stretch_mode(StretchMode p_mode);
		StretchMode get_stretch_mode() const;
	};
}

VARIANT_ENUM_CAST(GIFPlayer::PlayMode);
VARIANT_ENUM_CAST(GIFPlayer::ExpandMode);
VARIANT_ENUM_CAST(GIFPlayer::StretchMode);

#endif // !GIF_PLAYER_H
