extends Node3D

const PILOTS := [
	{"id":"cheikh", "name":"Cheikh", "color":Color("4aa8ff"), "accent":Color("d9cfb9"), "speed":0.080, "handling":2.7},
	{"id":"yvane", "name":"Yvane", "color":Color("ffc400"), "accent":Color("151820"), "speed":0.086, "handling":2.45},
	{"id":"nelvyn", "name":"Nelvyn", "color":Color("32f071"), "accent":Color("11161d"), "speed":0.077, "handling":3.15},
	{"id":"nova", "name":"Nova", "color":Color("ee49ff"), "accent":Color("261331"), "speed":0.082, "handling":2.8}
]

const TRACKS := [
	{"name":"Orbite Zéro", "radius":42.0, "width":13.0, "color":Color("3e7fc7"), "sky":Color("071326")},
	{"name":"Anneaux de Saturne", "radius":48.0, "width":12.5, "color":Color("c6a36d"), "sky":Color("1d1422")},
	{"name":"Nébuleuse Turbo", "radius":39.0, "width":12.0, "color":Color("a041cc"), "sky":Color("160b2a")},
	{"name":"Station Titan", "radius":45.0, "width":11.5, "color":Color("77838f"), "sky":Color("10151b")},
	{"name":"Trou Noir Final", "radius":36.0, "width":11.0, "color":Color("d64b8e"), "sky":Color("020207")}
]

const TOTAL_LAPS := 3
const POINTS := [10, 7, 5, 3]

var selected_pilot := 0
var track_index := 0
var state := "menu"
var player_progress := 0.0
var player_lane := 0.0
var steer_input := 0.0
var brake_pressed := false
var drift_pressed := false
var boost_time := 0.0
var item_ready := true
var race_time := 0.0
var countdown := 3.5
var championship_points := [0, 0, 0, 0]
var ai_progress := [0.0, -0.006, -0.012]
var ai_lane := [-2.8, 0.0, 2.8]
var ai_speed := [0.078, 0.081, 0.076]
var finish_order: Array[int] = []

var track_root: Node3D
var racers_root: Node3D
var player_kart: Node3D
var ai_karts: Array[Node3D] = []
var camera: Camera3D
var ui: CanvasLayer
var menu_panel: Control
var race_panel: Control
var result_panel: Control
var title_label: Label
var hud_label: Label
var countdown_label: Label
var item_button: Button
var result_label: Label
var pilot_buttons: Array[Button] = []

func _ready() -> void:
	_build_environment()
	_build_ui()
	_show_menu()
	set_process(true)
	set_process_input(true)

func _build_environment() -> void:
	var world := WorldEnvironment.new()
	var env := Environment.new()
	env.background_mode = Environment.BG_COLOR
	env.background_color = TRACKS[0].sky
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color = Color("90a6c0")
	env.ambient_light_energy = 0.75
	env.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	world.environment = env
	world.name = "WorldEnvironment"
	add_child(world)

	var light := DirectionalLight3D.new()
	light.rotation_degrees = Vector3(-52.0, -30.0, 0.0)
	light.light_energy = 1.45
	light.shadow_enabled = true
	add_child(light)

	track_root = Node3D.new()
	track_root.name = "Track"
	add_child(track_root)

	racers_root = Node3D.new()
	racers_root.name = "Racers"
	add_child(racers_root)

	camera = Camera3D.new()
	camera.current = true
	camera.fov = 67.0
	camera.position = Vector3(0.0, 18.0, 28.0)
	add_child(camera)

	_build_stars()

func _build_stars() -> void:
	var star_mesh := SphereMesh.new()
	star_mesh.radius = 0.12
	star_mesh.height = 0.24
	var material := StandardMaterial3D.new()
	material.albedo_color = Color(0.75, 0.88, 1.0)
	material.emission_enabled = true
	material.emission = Color(0.45, 0.72, 1.0)
	material.emission_energy_multiplier = 2.5
	star_mesh.material = material
	for i in range(120):
		var star := MeshInstance3D.new()
		star.mesh = star_mesh
		var a := float(i) * 2.399963
		var r := 70.0 + float((i * 17) % 80)
		star.position = Vector3(cos(a) * r, float((i * 23) % 70) - 15.0, sin(a) * r)
		add_child(star)

func _build_ui() -> void:
	ui = CanvasLayer.new()
	add_child(ui)

	menu_panel = Control.new()
	menu_panel.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	ui.add_child(menu_panel)

	var shade := ColorRect.new()
	shade.color = Color(0.01, 0.02, 0.06, 0.82)
	shade.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	menu_panel.add_child(shade)

	title_label = Label.new()
	title_label.text = "SPACE KART LEGENDS"
	title_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title_label.add_theme_font_size_override("font_size", 56)
	title_label.set_anchors_preset(Control.PRESET_CENTER_TOP)
	title_label.position = Vector2(-400, 65)
	title_label.size = Vector2(800, 80)
	menu_panel.add_child(title_label)

	var subtitle := Label.new()
	subtitle.text = "Choisis ton pilote"
	subtitle.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	subtitle.add_theme_font_size_override("font_size", 28)
	subtitle.set_anchors_preset(Control.PRESET_CENTER_TOP)
	subtitle.position = Vector2(-250, 145)
	subtitle.size = Vector2(500, 48)
	menu_panel.add_child(subtitle)

	for i in range(3):
		var button := Button.new()
		button.text = PILOTS[i].name
		button.add_theme_font_size_override("font_size", 30)
		button.set_anchors_preset(Control.PRESET_CENTER)
		button.position = Vector2(-360 + i * 250, -35)
		button.size = Vector2(220, 105)
		button.pressed.connect(_select_pilot.bind(i))
		menu_panel.add_child(button)
		pilot_buttons.append(button)

	var start_button := Button.new()
	start_button.text = "DÉMARRER LE CHAMPIONNAT"
	start_button.add_theme_font_size_override("font_size", 27)
	start_button.set_anchors_preset(Control.PRESET_CENTER)
	start_button.position = Vector2(-250, 125)
	start_button.size = Vector2(500, 84)
	start_button.pressed.connect(_start_championship)
	menu_panel.add_child(start_button)

	race_panel = Control.new()
	race_panel.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	ui.add_child(race_panel)

	hud_label = Label.new()
	hud_label.add_theme_font_size_override("font_size", 28)
	hud_label.position = Vector2(28, 22)
	hud_label.size = Vector2(650, 170)
	race_panel.add_child(hud_label)

	countdown_label = Label.new()
	countdown_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	countdown_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	countdown_label.add_theme_font_size_override("font_size", 96)
	countdown_label.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	race_panel.add_child(countdown_label)

	var left_button := _touch_button("◀", Vector2(34, -190), Vector2(150, 150), Control.PRESET_BOTTOM_LEFT)
	left_button.button_down.connect(func(): steer_input = -1.0)
	left_button.button_up.connect(func(): if steer_input < 0.0: steer_input = 0.0)
	race_panel.add_child(left_button)

	var right_button := _touch_button("▶", Vector2(204, -190), Vector2(150, 150), Control.PRESET_BOTTOM_LEFT)
	right_button.button_down.connect(func(): steer_input = 1.0)
	right_button.button_up.connect(func(): if steer_input > 0.0: steer_input = 0.0)
	race_panel.add_child(right_button)

	var drift_button := _touch_button("DRIFT", Vector2(-380, -190), Vector2(160, 150), Control.PRESET_BOTTOM_RIGHT)
	drift_button.button_down.connect(func(): drift_pressed = true)
	drift_button.button_up.connect(func(): drift_pressed = false)
	race_panel.add_child(drift_button)

	var brake_button := _touch_button("FREIN", Vector2(-200, -190), Vector2(160, 150), Control.PRESET_BOTTOM_RIGHT)
	brake_button.button_down.connect(func(): brake_pressed = true)
	brake_button.button_up.connect(func(): brake_pressed = false)
	race_panel.add_child(brake_button)

	item_button = _touch_button("OBJET", Vector2(-200, 24), Vector2(160, 115), Control.PRESET_TOP_RIGHT)
	item_button.pressed.connect(_use_item)
	race_panel.add_child(item_button)

	result_panel = Control.new()
	result_panel.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	ui.add_child(result_panel)
	var result_shade := ColorRect.new()
	result_shade.color = Color(0.01, 0.02, 0.06, 0.88)
	result_shade.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	result_panel.add_child(result_shade)

	result_label = Label.new()
	result_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	result_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	result_label.add_theme_font_size_override("font_size", 36)
	result_label.set_anchors_preset(Control.PRESET_CENTER)
	result_label.position = Vector2(-430, -230)
	result_label.size = Vector2(860, 360)
	result_panel.add_child(result_label)

	var next_button := Button.new()
	next_button.text = "COURSE SUIVANTE"
	next_button.add_theme_font_size_override("font_size", 28)
	next_button.set_anchors_preset(Control.PRESET_CENTER)
	next_button.position = Vector2(-210, 150)
	next_button.size = Vector2(420, 82)
	next_button.pressed.connect(_continue_after_result)
	result_panel.add_child(next_button)

func _touch_button(text_value: String, pos: Vector2, button_size: Vector2, preset: LayoutPreset) -> Button:
	var button := Button.new()
	button.text = text_value
	button.add_theme_font_size_override("font_size", 25)
	button.set_anchors_preset(preset)
	button.position = pos
	button.size = button_size
	button.modulate = Color(1, 1, 1, 0.82)
	return button

func _select_pilot(index: int) -> void:
	selected_pilot = index
	for i in pilot_buttons.size():
		pilot_buttons[i].disabled = i == selected_pilot

func _show_menu() -> void:
	state = "menu"
	menu_panel.visible = true
	race_panel.visible = false
	result_panel.visible = false
	_clear_race_nodes()
	_select_pilot(selected_pilot)

func _start_championship() -> void:
	championship_points = [0, 0, 0, 0]
	track_index = 0
	_start_race()

func _start_race() -> void:
	state = "race"
	menu_panel.visible = false
	race_panel.visible = true
	result_panel.visible = false
	player_progress = 0.0
	player_lane = 0.0
	boost_time = 0.0
	item_ready = true
	race_time = 0.0
	countdown = 3.5
	finish_order.clear()
	ai_progress = [-0.008, -0.016, -0.024]
	ai_lane = [-3.0, 0.0, 3.0]
	_clear_race_nodes()
	_build_track()
	_spawn_racers()

func _clear_race_nodes() -> void:
	if track_root:
		for child in track_root.get_children():
			child.queue_free()
	if racers_root:
		for child in racers_root.get_children():
			child.queue_free()
	ai_karts.clear()
	player_kart = null

func _build_track() -> void:
	var data: Dictionary = TRACKS[track_index]
	var radius: float = data.radius
	var width: float = data.width
	var segments := 96
	var segment_length := TAU * radius / float(segments)
	for i in range(segments):
		var angle := TAU * float(i) / float(segments)
		var segment := MeshInstance3D.new()
		var mesh := BoxMesh.new()
		mesh.size = Vector3(width, 0.5, segment_length * 1.1)
		var mat := StandardMaterial3D.new()
		var stripe := 0.78 + 0.12 * float(i % 2)
		mat.albedo_color = data.color * stripe
		mat.metallic = 0.25
		mat.roughness = 0.55
		mesh.material = mat
		segment.mesh = mesh
		segment.position = Vector3(cos(angle) * radius, 0.0, sin(angle) * radius)
		segment.rotation.y = -angle
		track_root.add_child(segment)

		if i % 4 == 0:
			for side in [-1.0, 1.0]:
				var rail := MeshInstance3D.new()
				var rail_mesh := BoxMesh.new()
				rail_mesh.size = Vector3(0.35, 1.2, segment_length * 4.1)
				var rail_mat := StandardMaterial3D.new()
				rail_mat.albedo_color = data.color.lightened(0.35)
				rail_mat.emission_enabled = true
				rail_mat.emission = data.color
				rail_mat.emission_energy_multiplier = 1.8
				rail_mesh.material = rail_mat
				rail.mesh = rail_mesh
				rail.position = Vector3(cos(angle) * (radius + side * width * 0.54), 0.75, sin(angle) * (radius + side * width * 0.54))
				rail.rotation.y = -angle
				track_root.add_child(rail)

	var center := MeshInstance3D.new()
	var center_mesh := SphereMesh.new()
	center_mesh.radius = 7.0 + track_index * 0.8
	center_mesh.height = center_mesh.radius * 2.0
	var center_mat := StandardMaterial3D.new()
	center_mat.albedo_color = data.sky.lightened(0.08)
	center_mat.emission_enabled = true
	center_mat.emission = data.color.darkened(0.25)
	center_mat.emission_energy_multiplier = 1.3
	center_mesh.material = center_mat
	center.mesh = center_mesh
	center.position = Vector3(0, -2, 0)
	track_root.add_child(center)

	var world := get_node("WorldEnvironment") as WorldEnvironment
	world.environment.background_color = data.sky

func _spawn_racers() -> void:
	player_kart = _create_kart(selected_pilot, true)
	racers_root.add_child(player_kart)
	var ai_pilot_indices: Array[int] = []
	for i in range(4):
		if i != selected_pilot:
			ai_pilot_indices.append(i)
	for i in range(3):
		var kart := _create_kart(ai_pilot_indices[i], false)
		kart.set_meta("pilot_index", ai_pilot_indices[i])
		racers_root.add_child(kart)
		ai_karts.append(kart)
	_update_racer_transforms(0.0)

func _create_kart(pilot_index: int, is_player: bool) -> Node3D:
	var root := Node3D.new()
	root.name = ("Player_" if is_player else "AI_") + PILOTS[pilot_index].name
	root.set_meta("pilot_index", pilot_index)

	var body := MeshInstance3D.new()
	var body_mesh := BoxMesh.new()
	body_mesh.size = Vector3(2.15, 0.65, 3.25)
	var body_mat := StandardMaterial3D.new()
	body_mat.albedo_color = PILOTS[pilot_index].accent
	body_mat.metallic = 0.55
	body_mat.roughness = 0.28
	body_mesh.material = body_mat
	body.mesh = body_mesh
	body.position.y = 0.75
	root.add_child(body)

	var nose := MeshInstance3D.new()
	var nose_mesh := BoxMesh.new()
	nose_mesh.size = Vector3(1.65, 0.32, 1.65)
	var nose_mat := StandardMaterial3D.new()
	nose_mat.albedo_color = PILOTS[pilot_index].color
	nose_mat.emission_enabled = true
	nose_mat.emission = PILOTS[pilot_index].color.darkened(0.25)
	nose_mat.emission_energy_multiplier = 1.15
	nose_mesh.material = nose_mat
	nose.mesh = nose_mesh
	nose.position = Vector3(0, 0.82, -1.85)
	root.add_child(nose)

	for wheel_position in [Vector3(-1.25, 0.45, -1.05), Vector3(1.25, 0.45, -1.05), Vector3(-1.25, 0.45, 1.05), Vector3(1.25, 0.45, 1.05)]:
		var wheel := MeshInstance3D.new()
		var wheel_mesh := CylinderMesh.new()
		wheel_mesh.top_radius = 0.42
		wheel_mesh.bottom_radius = 0.42
		wheel_mesh.height = 0.38
		var wheel_mat := StandardMaterial3D.new()
		wheel_mat.albedo_color = Color("101217")
		wheel_mesh.material = wheel_mat
		wheel.mesh = wheel_mesh
		wheel.position = wheel_position
		wheel.rotation.z = PI * 0.5
		root.add_child(wheel)

	var pilot_holder := Node3D.new()
	pilot_holder.name = "PilotHolder"
	pilot_holder.position = Vector3(0, 0.7, 0.45)
	root.add_child(pilot_holder)
	_add_pilot_visual(pilot_holder, pilot_index)
	return root

func _add_pilot_visual(holder: Node3D, pilot_index: int) -> void:
	var path := "res://private_models/%s.glb" % PILOTS[pilot_index].id
	if ResourceLoader.exists(path):
		var scene := load(path) as PackedScene
		if scene:
			var model := scene.instantiate()
			model.name = "RealPilotModel"
			model.scale = Vector3.ONE * 0.72
			model.rotation.y = PI
			holder.add_child(model)
			var animation_player := _find_animation_player(model)
			if animation_player and animation_player.has_animation("drive_idle"):
				animation_player.play("drive_idle")
			return

	var torso := MeshInstance3D.new()
	var torso_mesh := CapsuleMesh.new()
	torso_mesh.radius = 0.48
	torso_mesh.height = 1.35
	var torso_mat := StandardMaterial3D.new()
	torso_mat.albedo_color = PILOTS[pilot_index].color
	torso_mesh.material = torso_mat
	torso.mesh = torso_mesh
	torso.position = Vector3(0, 1.05, 0)
	holder.add_child(torso)
	var head := MeshInstance3D.new()
	var head_mesh := SphereMesh.new()
	head_mesh.radius = 0.42
	head_mesh.height = 0.84
	var head_mat := StandardMaterial3D.new()
	head_mat.albedo_color = Color("8a5b43") if pilot_index != 3 else Color("b78361")
	head_mesh.material = head_mat
	head.mesh = head_mesh
	head.position = Vector3(0, 2.0, -0.05)
	holder.add_child(head)

func _find_animation_player(node: Node) -> AnimationPlayer:
	if node is AnimationPlayer:
		return node as AnimationPlayer
	for child in node.get_children():
		var found := _find_animation_player(child)
		if found:
			return found
	return null

func _process(delta: float) -> void:
	if state != "race":
		return
	_update_inputs()
	if countdown > 0.0:
		countdown -= delta
		countdown_label.text = str(int(ceil(countdown))) if countdown > 0.35 else "GO !"
		if countdown <= 0.0:
			countdown_label.text = ""
		_update_racer_transforms(delta)
		return

	race_time += delta
	if boost_time > 0.0:
		boost_time -= delta
	var pilot: Dictionary = PILOTS[selected_pilot]
	var speed: float = pilot.speed
	if brake_pressed:
		speed *= 0.42
	if drift_pressed and abs(steer_input) > 0.1:
		speed *= 0.92
	if boost_time > 0.0:
		speed *= 1.55
	player_progress += speed * delta
	player_lane = move_toward(player_lane, steer_input * TRACKS[track_index].width * 0.34, pilot.handling * delta * 2.2)

	for i in range(3):
		var wobble := sin(race_time * (0.9 + i * 0.17) + i) * 0.006
		ai_progress[i] += (ai_speed[i] + wobble + track_index * 0.0008) * delta
		ai_lane[i] = lerp(ai_lane[i], sin(race_time * 0.55 + i * 2.1) * TRACKS[track_index].width * 0.26, delta * 0.7)

	_update_racer_transforms(delta)
	_update_hud()
	_check_finish()

func _update_inputs() -> void:
	var keyboard_steer := Input.get_axis("steer_left", "steer_right")
	if abs(keyboard_steer) > 0.05:
		steer_input = keyboard_steer
	brake_pressed = brake_pressed or Input.is_action_pressed("brake")
	drift_pressed = drift_pressed or Input.is_action_pressed("drift")
	if Input.is_action_just_pressed("use_item"):
		_use_item()

func _update_racer_transforms(delta: float) -> void:
	if not player_kart:
		return
	_place_kart(player_kart, player_progress, player_lane)
	for i in range(ai_karts.size()):
		_place_kart(ai_karts[i], ai_progress[i], ai_lane[i])
	var data: Dictionary = TRACKS[track_index]
	var angle := fmod(player_progress, 1.0) * TAU
	var radial := Vector3(cos(angle), 0, sin(angle))
	var tangent := Vector3(-sin(angle), 0, cos(angle))
	var target := radial * (data.radius + player_lane) + Vector3.UP * 1.1
	var camera_target := target - tangent * 10.5 + Vector3.UP * 6.0
	camera.position = camera.position.lerp(camera_target, clamp(delta * 5.0, 0.0, 1.0))
	camera.look_at(target + tangent * 4.5 + Vector3.UP * 1.0, Vector3.UP)

func _place_kart(kart: Node3D, progress: float, lane: float) -> void:
	var data: Dictionary = TRACKS[track_index]
	var angle := fmod(progress, 1.0) * TAU
	var radial := Vector3(cos(angle), 0, sin(angle))
	var tangent := Vector3(-sin(angle), 0, cos(angle))
	var position := radial * (data.radius + lane) + Vector3.UP * 0.55
	kart.position = position
	kart.look_at(position + tangent, Vector3.UP)
	if drift_pressed and kart == player_kart:
		kart.rotation.z = lerp(kart.rotation.z, -steer_input * 0.13, 0.18)
	else:
		kart.rotation.z = lerp(kart.rotation.z, 0.0, 0.18)

func _update_hud() -> void:
	var lap := min(TOTAL_LAPS, int(floor(max(player_progress, 0.0))) + 1)
	var ranking := _current_ranking()
	var place := ranking.find(selected_pilot) + 1
	hud_label.text = "%s\n%s  •  Tour %d/%d\nPosition %d/4  •  %.1f s" % [
		TRACKS[track_index].name,
		PILOTS[selected_pilot].name,
		lap,
		TOTAL_LAPS,
		place,
		race_time
	]
	item_button.text = "TURBO" if item_ready else "VIDE"
	item_button.disabled = not item_ready

func _current_ranking() -> Array[int]:
	var entries := [
		{"pilot":selected_pilot, "progress":player_progress}
	]
	for i in range(3):
		entries.append({"pilot":int(ai_karts[i].get_meta("pilot_index")), "progress":ai_progress[i]})
	entries.sort_custom(func(a, b): return a.progress > b.progress)
	var ranking: Array[int] = []
	for entry in entries:
		ranking.append(entry.pilot)
	return ranking

func _check_finish() -> void:
	if player_progress < float(TOTAL_LAPS):
		return
	var ranking := _current_ranking()
	for pilot_index in ranking:
		if pilot_index not in finish_order:
			finish_order.append(pilot_index)
	for i in range(4):
		var pilot_index := ranking[i]
		championship_points[pilot_index] += POINTS[i]
	_show_result(ranking)

func _show_result(ranking: Array[int]) -> void:
	state = "result"
	race_panel.visible = false
	result_panel.visible = true
	var lines := ["RÉSULTAT — %s" % TRACKS[track_index].name, ""]
	for i in range(ranking.size()):
		lines.append("%d. %s  +%d pts" % [i + 1, PILOTS[ranking[i]].name, POINTS[i]])
	lines.append("")
	lines.append("Championnat : %d points" % championship_points[selected_pilot])
	result_label.text = "\n".join(lines)

func _continue_after_result() -> void:
	track_index += 1
	if track_index >= TRACKS.size():
		_show_championship_result()
	else:
		_start_race()

func _show_championship_result() -> void:
	state = "championship_end"
	var ranking := [0, 1, 2, 3]
	ranking.sort_custom(func(a, b): return championship_points[a] > championship_points[b])
	var lines := ["CHAMPIONNAT TERMINÉ", ""]
	for i in range(ranking.size()):
		lines.append("%d. %s — %d points" % [i + 1, PILOTS[ranking[i]].name, championship_points[ranking[i]]])
	lines.append("")
	lines.append("Appuie pour recommencer")
	result_label.text = "\n".join(lines)
	var button := result_panel.get_child(result_panel.get_child_count() - 1) as Button
	button.text = "RECOMMENCER"
	button.pressed.disconnect(_continue_after_result)
	button.pressed.connect(_show_menu)

func _use_item() -> void:
	if state != "race" or countdown > 0.0 or not item_ready:
		return
	item_ready = false
	boost_time = 2.2

func _input(event: InputEvent) -> void:
	if event is InputEventScreenDrag and state == "race":
		var drag := event as InputEventScreenDrag
		if drag.position.x < get_viewport().get_visible_rect().size.x * 0.5:
			steer_input = clamp((drag.position.x / (get_viewport().get_visible_rect().size.x * 0.5) - 0.5) * 2.0, -1.0, 1.0)
