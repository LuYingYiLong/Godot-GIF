#ifndef GIF_TEXTURE_H
#define GIF_TEXTURE_H

#include "gif_reader.h"

#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/image_texture.hpp>

namespace godot {
	class GIFTexture : public Texture2D {
		GDCLASS(GIFTexture, Texture2D)

	private:
		// GIF 原始数据
		PackedByteArray gif_data;
		Size2i size;

		// 帧数据
		TypedArray<ImageTexture> frames;								// 所有帧纹理
		PackedFloat32Array frame_delays;								// 每帧延迟（秒）

		// 当前显示
		int current_frame = 0;
		int frame_count = 0;

		// 全局属性（来自 GIF 文件）
		int loop_count = 0;

	protected:
		static void _bind_methods();

	public:
		GIFTexture();
		~GIFTexture();

		virtual int _get_width() const override;
		virtual int _get_height() const override;
		virtual bool _has_alpha() const override;
		virtual RID _get_rid() const override;

		void set_data(const PackedByteArray& p_data);
		PackedByteArray get_data() const;

		// 加载
		static Ref<GIFTexture> load_from_file(const String& p_path);
		static Ref<GIFTexture> load_from_buffer(const PackedByteArray& p_buffer);

		// 帧访问
		void set_frame(int p_frame);									// 设置当前显示帧
		int get_frame() const;											// 获取当前帧索引
		int get_frame_count() const;									// 获取总帧数
		
		Ref<ImageTexture> get_frame_texture(int p_frame) const;			// 获取指定帧纹理
		Ref<ImageTexture> get_current_texture() const;					// 获取当前帧纹理

		// 帧元数据
		float get_frame_delay(int p_frame) const;						// 获取帧延迟（秒）
		float get_total_duration() const;								// 获取总时长（秒）

		// 全局属性
		int get_loop_count() const;										// GIF 本身循环次数 (0=无限)
	};
}

#endif // !GIF_TEXTURE_H
