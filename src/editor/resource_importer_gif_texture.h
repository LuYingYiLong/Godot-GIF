#ifndef RESOURCE_IMPORTER_GIF_TEXTURE_H
#define RESOURCE_IMPORTER_GIF_TEXTURE_H

#include <godot_cpp/classes/editor_import_plugin.hpp>

namespace godot {
	class ResourceImporterGIFTexture : public EditorImportPlugin {
		GDCLASS(ResourceImporterGIFTexture, EditorImportPlugin)

	protected:
		static void _bind_methods();

	public:
		virtual String _get_importer_name() const override;
		virtual String _get_visible_name() const override;
		virtual int32_t _get_preset_count() const override;
		virtual String _get_preset_name(int32_t p_preset_index) const override;
		virtual PackedStringArray _get_recognized_extensions() const override;
		virtual TypedArray<Dictionary> _get_import_options(const String& p_path, int32_t p_preset_index) const override;
		virtual String _get_save_extension() const override;
		virtual String _get_resource_type() const override;
		virtual float _get_priority() const override;
		virtual int32_t _get_import_order() const override;
		virtual int32_t _get_format_version() const override;
		virtual bool _get_option_visibility(const String& p_path, const StringName& p_option_name, const Dictionary& p_options) const override;
		virtual Error _import(const String& p_source_file, const String& p_save_path, const Dictionary& p_options, const TypedArray<String>& p_platform_variants, const TypedArray<String>& p_gen_files) const override;
	};
}

#endif // !RESOURCE_IMPORTER_GIF_TEXTURE_H
