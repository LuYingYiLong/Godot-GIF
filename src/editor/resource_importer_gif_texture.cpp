#include "resource_importer_gif_texture.h"

#include "../core/gif_texture.h"
#include "../core/gif_reader.h"

#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/classes/file_access.hpp>

namespace godot {
	void ResourceImporterGIFTexture::_bind_methods() {
		
	}

	String ResourceImporterGIFTexture::_get_importer_name() const {
		return "texture_gif";
	}

	String ResourceImporterGIFTexture::_get_visible_name() const {
		return "GIF Texture";
	}

	int32_t ResourceImporterGIFTexture::_get_preset_count() const {
		return 1;
	}

	String ResourceImporterGIFTexture::_get_preset_name(int32_t p_preset_index) const {
		return "Default";
	}

	PackedStringArray ResourceImporterGIFTexture::_get_recognized_extensions() const {
		PackedStringArray ext;
		ext.append("gif");
		return ext;
	}

	TypedArray<Dictionary> ResourceImporterGIFTexture::_get_import_options(const String& p_path, int32_t p_preset_index) const {
		TypedArray<Dictionary> options;

		// 存储选项
		Dictionary compress_option;
		compress_option["name"] = "storage/compress";
		compress_option["default_value"] = true;
		options.append(compress_option);

		return options;
	}

	String ResourceImporterGIFTexture::_get_save_extension() const {
		return "res";
	}

	String ResourceImporterGIFTexture::_get_resource_type() const {
		return "GIFTexture";
	}

	float ResourceImporterGIFTexture::_get_priority() const {
		return 1.0f;
	}

	int32_t ResourceImporterGIFTexture::_get_import_order() const {
		return IMPORT_ORDER_DEFAULT;
	}

	int32_t ResourceImporterGIFTexture::_get_format_version() const {
		return 1;
	}

	bool ResourceImporterGIFTexture::_get_option_visibility(const String& p_path, const StringName& p_option_name, const Dictionary& p_options) const {
		return true;
	}

	Error ResourceImporterGIFTexture::_import(const String& p_source_file, const String& p_save_path, const Dictionary& p_options, const TypedArray<String>& p_platform_variants, const TypedArray<String>& p_gen_files) const {
		// 读取导入选项
		bool compress = p_options.get("storage/compress", true);

		// 读取源文件数据
		Ref<FileAccess> f = FileAccess::open(p_source_file, FileAccess::READ);
		if (f.is_null()) {
			ERR_PRINT("Cannot open gif file: " + p_source_file);
			return ERR_CANT_OPEN;
		}

		uint64_t file_size = f->get_length();
		PackedByteArray gif_data;
		gif_data.resize(file_size);
		f->get_buffer(gif_data.ptrw(), file_size);
		f->close();

		if (gif_data.is_empty()) {
			ERR_PRINT("GIF file is empty: " + p_source_file);
			return ERR_FILE_CORRUPT;
		}

		// 创建 GIFTexture 资源
		Ref<GIFTexture> gif_texture;
		gif_texture.instantiate();
		
		// 通过 set_data 加载 GIF 数据
		// set_data 会自动调用 load_from_data 解析 GIF
		gif_texture->set_data(gif_data);

		// 保存资源
		String save_path = p_save_path + String(".") + _get_save_extension();
		uint32_t save_flags = 0;
		if (compress) {
			save_flags |= ResourceSaver::FLAG_COMPRESS;
		}
		Error err = ResourceSaver::get_singleton()->save(gif_texture, save_path, save_flags);
		if (err != OK) {
			ERR_PRINT("Failed to save GIFTexture resource: " + save_path + ", error: " + String::num_int64(err));
			return err;
		}

		return OK;
	}
}
