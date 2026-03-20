@tool
extends EditorPlugin

var importer: ResourceImporterGIFTexture

func _enter_tree() -> void:
	importer = ResourceImporterGIFTexture.new()
	add_import_plugin(importer)

func _exit_tree() -> void:
	if importer:
		remove_import_plugin(importer)
		importer = null
