@tool
extends SceneTree

func _initialize() -> void:
	var settings = EditorInterface.get_editor_settings()
	if settings == null:
		printerr("EditorSettings instance is unavailable")
		quit(1)
		return

	var android_home = OS.get_environment("ANDROID_HOME")
	var java_home = OS.get_environment("JAVA_HOME")
	if android_home.is_empty() or java_home.is_empty():
		printerr("ANDROID_HOME or JAVA_HOME is missing")
		quit(2)
		return

	settings.set_setting("export/android/android_sdk_path", android_home)
	settings.set_setting("export/android/java_sdk_path", java_home)
	settings.save()

	print("Configured Android SDK: ", settings.get_setting("export/android/android_sdk_path"))
	print("Configured Java SDK: ", settings.get_setting("export/android/java_sdk_path"))
	quit(0)
