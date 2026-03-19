#ifndef GIF_READER_H
#define GIF_READER_H

#include "gif_utils.hpp"

namespace godot {
	class Image;

	class GIFReader : public RefCounted {
		GDCLASS(GIFReader, RefCounted)

	public:
		enum GIFError {
			SUCCEEDED = 0,
			OPEN_FAILED = 101,
			READ_FAILED = 102,
			NOT_GIF_FILE = 103,
			NO_SCRN_DSCR = 104,
			NO_IMAG_DSCR = 105,
			NO_COLOR_MAP = 106,
			WRONG_RECORD = 107,
			DATA_TOO_BIG = 108,
			NOT_ENOUGH_MEM = 109,
			CLOSE_FAILED = 110,
			NOT_READABLE = 111,
			IMAGE_DEFECT = 112,
			EOF_TOO_SOON = 113
		};

	private:
		GifFileType* file_type = nullptr;

	protected:
		static void _bind_methods();

	public:
		~GIFReader();

		GIFError open(const String& p_path);
		GIFError close();

		int get_width() const;
		int get_height() const;
		int get_color_resolution() const;
		Color get_background_color() const;
		int get_aspect_byte() const;
		Dictionary get_color_map() const;
		int get_image_count() const;
		Ref<Image> get_image(int frame_index = 0) const;
		TypedArray<Image> get_saved_images() const;
	};
}

VARIANT_ENUM_CAST(GIFReader::GIFError);

#endif // !GIF_READER_H
