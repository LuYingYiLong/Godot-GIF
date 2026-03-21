#ifndef GIF_READER_H
#define GIF_READER_H

#include "gif_utils.hpp"

#include <mutex>

namespace godot {
	class Image;

	// 处置方式（定义在结构体之前）
	enum GifDisposalMethod {
		GIF_DISPOSAL_UNSPECIFIED = 0,									// 未指定
		GIF_DISPOSAL_DO_NOT = 1,										// 不处置，保留当前帧
		GIF_DISPOSAL_BACKGROUND = 2,									// 恢复到背景色
		GIF_DISPOSAL_PREVIOUS = 3										// 恢复到前一帧
	};

	// 帧原始数据，用于外部合成
	struct GIFFrameRawData {
		int left = 0;
		int top = 0;
		int width = 0;
		int height = 0;
		PackedByteArray pixel_indices;									// 8-bit 调色板索引
		int color_count = 0;											// 调色板颜色数
		PackedColorArray palette;										// 调色板颜色 (RGBA)
		int transparent_color = -1;										// 透明色索引，-1 表示无
		GifDisposalMethod disposal_method = GIF_DISPOSAL_UNSPECIFIED;
		int delay_ms = 0;
	};

	class GIFReader : public RefCounted {
		GDCLASS(GIFReader, RefCounted)

	private:
		static std::mutex giflib_mutex;									// 静态互斥锁保护 giflib 的全局状态 (giflib 不是线程安全的)

	public:
		// GIF 错误码
		enum GIFError {
			SUCCEEDED = 0,
			OPEN_FAILED = 101,											// 未能打开给定文件
			READ_FAILED = 102,											// 未能写入给定文件
			NOT_GIF_FILE = 103,											// 数据不是 GIF 文件
			NO_SCRN_DSCR = 104,											// 未检测到屏幕描述符
			NO_IMAG_DSCR = 105,											// 未检测到图像描述符
			NO_COLOR_MAP = 106,											// 既非全局也非局部色彩映射
			WRONG_RECORD = 107,											// 检测到错误的记录类型
			DATA_TOO_BIG = 108,											// 像素数大于 宽度 * 高度
			NOT_ENOUGH_MEM = 109,										// 未能分配
			CLOSE_FAILED = 110,											// 未能关闭给定文件
			NOT_READABLE = 111,											// 给定文件不是 打开读取
			IMAGE_DEFECT = 112,											// 图像有缺陷
			EOF_TOO_SOON = 113											// 检测到图像 EOF
		};

		// 处置方式（保持向后兼容）
		enum DisposalMethod {
			DISPOSAL_METHOD_UNSPECIFIED = GIF_DISPOSAL_UNSPECIFIED,
			DISPOSAL_METHOD_DO_NOT = GIF_DISPOSAL_DO_NOT,
			DISPOSAL_METHOD_BACKGROUND = GIF_DISPOSAL_BACKGROUND,
			DISPOSAL_METHOD_PREVIOUS = GIF_DISPOSAL_PREVIOUS
		};

	private:
		GifFileType* file_type = nullptr;

		// 内存读取用的数据
		PackedByteArray mem_data;
		int mem_read_pos = 0;

		static int _mem_input_func(GifFileType* gif, GifByteType* bytes, int size);

	protected:
		static void _bind_methods();

	public:
		~GIFReader();

		// 文件
		GIFError open(const String& p_path);							// 打开文件
		GIFError open_from_buffer(const PackedByteArray& p_buffer);		// 从内存数据打开 (支持从网络/压缩包加载)
		GIFError close();												// 关闭文件

		// 常规
		Vector2i get_size() const;										// 获取虚拟画布尺寸
		int get_color_resolution() const;								// 获取颜色分辨率
		Color get_background_color() const;								// 获取背景色
		int get_aspect_byte() const;									// 获取纵横比
		Dictionary get_color_map() const;								// 获取全局色彩映射
		int get_image_count() const;									// 获取图像数量
		Ref<Image> get_image(const int frame_index = 0) const;			// 获取当前帧
		TypedArray<Image> get_saved_images() const;						// 获取所有帧

		// 扩展块
		int get_frame_delay(const int frame_index) const;				// 获取帧延迟时间 (ms)
		DisposalMethod get_disposal_method(const int frame_index) const;// 获取帧处置方式
		int get_loop_count() const;										// 获取循环次数 (0=无限)
		PackedStringArray get_comments() const;							// 获取所有注释文本
		Dictionary get_frame_gcb(const int frame_index) const;			// 获取帧的图形控制块原始数据
		Dictionary get_global_metadata() const;							// 获取全局元数据

		// 获取帧原始数据 (用于合成)
		GIFFrameRawData get_frame_raw_data(const int frame_index) const;// 获取帧原始数据（包含调色板索引）
	};
}

VARIANT_ENUM_CAST(GIFReader::GIFError);
VARIANT_ENUM_CAST(GIFReader::DisposalMethod);

#endif // !GIF_READER_H
