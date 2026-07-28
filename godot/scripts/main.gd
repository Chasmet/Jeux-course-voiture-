extends Node3D

const PILOTS = [
	{"id": "cheikh", "name": "Cheikh", "color": Color("4aa8ff"), "accent": Color("d9cfb9"), "speed": 0.080, "handling": 2.70},
	{"id": "yvane", "name": "Yvane", "color": Color("ffc400"), "accent": Color("151820"), "speed": 0.086, "handling": 2.45},
	{"id": "nelvyn", "name": "Nelvyn", "color": Color("32f071"), "accent": Color("11161d"), "speed": 0.077, "handling": 3.15},
	{"id": "nova", "name": "Nova", "color": Color("ee49ff"), "accent": Color("261331"), "speed": 0.082, "handling": 2.80}
]

const TRACKS = [
	{"name": "Orbite Zéro", "radius": 42.0, "width": 13.0, "color": Color("3e7fc7"), "sky": Color("071326")},
	{"name": "Anneaux de Saturne", "radius": 48.0, "width": 12.5, "color": Color("c6a36d"), "sky": Color("1d1422")},
	{"name": "Nébuleuse Turbo", "radius": 39.0, "width": 12.0, "color": Color("a041cc"), "sky": Color("160b2a")},
	{"name": "Station Titan", "radius": 45.0, "width": 11.5, "color": Color("77838f"), "sky": Color("10151b")},
	{"name": "Trou Noir Final", "radius": 36.0, "width": 11.0, "color": Color("d64b8e"), "sky": Color("020207")}
]

const TOTAL_LAPS = 3
const POINTS = [10, 7, 5, 3]

var selected_pilot = 0
var track_index = 0
var game_state = "menu"
var player_progress = 0.0
var player_lane = 0.0
var steer_input = 0.0
var touch_brake = false
var touch_drift = false
var boost_time = 0.0
var item_ready = true
var race_time = 0.0
var countdown = 3.5
var championship_points = [0, 0, 0, 0]
var ai_progress = [-0.008, -0.016, -0.024]
var ai_lane = [-3.0, 0.0, 3.0]
var ai_speed = [0.078, 0.081, 0.076]

var track_root: Node3D
var racers_root: Node3D
var player_kart: Node3D
var ai_karts: Array[Node3D] = []
var camera: Camera3D
var menu_panel: Control
var race_panel: Control
var result_panel: Control
var hud_label: Label
var countdown_label: Label
var item_button: Button
var result_label: Label
var next_button: Button
var pilot_buttons: Array[Button] = []

func _ready() -> void:
	_build_environment()
	_build_ui()
	_show_menu()

func _build_environment() -> void:
	var world = WorldEnvironment.new()
	world.name = "WorldEnvironment"
	var environment = Environment.new()
	environment.background_mode = Environment.BG_COLOR
	environment.background_color = TRACKS[0]["sky"]
	environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	environment.ambient_light_color = Color("90a6c0")
	environment.ambient_light_energy = 0.75
	environment.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	world.environment = environment
	add_child(world)

	var light = DirectionalLight3D.new()
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
	var star_mesh = SphereMesh.new()
	star_mesh.radius = 0.12
	star_mesh.height = 0.24
	var star_material = StandardMaterial3D.new()
	star_material.albedo_color = Color(0.75, 0.88, 1.0)
	star_material.emission_enabled = true
	star_material.emission = Color(0.45, 0.72, 1.0)
	star_material.emission_energy_multiplier = 2.5
	star_mesh.material = star_material
	for index in range(120):
		var star = MeshInstance3D.new()
		star.mesh = star_mesh
		var angle = float(index) * 2.399963
		var radius = 70.0 + float((index * 17) % 80)
		star.position = Vector3(cos(angle) * radius, float((index * 23) % 70) - 15.0, sin(angle) * radius)
		add_child(star)

func _build_ui() -> void:
	var canvas = CanvasLayer.new()
	add_child(canvas)

	menu_panel = Control.new()
	menu_panel.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	canvas.add_child(menu_panel)
	var menu_shade = ColorRect.new()
	menu_shade.color = Color(0.01, 0.02, 0.06, 0.84)
	menu_shade.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	menu_panel.add_child(menu_shade)

	var title = Label.new()
	title.text = "SPACE KART LEGENDS"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 56)
	title.set_anchors_preset(Control.PRESET_CENTER_TOP)
	title.position = Vector2(-400, 65)
	title.size = Vector2(800, 80)
	menu_panel.add_child(title)

	var subtitle = Label.new()
	subtitle.text = "Choisis ton pilote"
	subtitle.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	subtitle.add_theme_font_size_override("font_size", 28)
	subtitle.set_anchors_preset(Control.PRESET_CENTER_TOP)
	subtitle.position = Vector2(-250, 145)
	subtitle.size = Vector2(500, 48)
	menu_panel.add_child(subtitle)

	for index in range(3):
		var pilot_button = Button.new()
		pilot_button.text = PILOTS[index]["name"]
		pilot_button.add_theme_font_size_override("font_size", 30)
		pilot_button.set_anchors_preset(Control.PRESET_CENTER)
		pilot_button.position = Vector2(-360 + index * 250, -35)
		pilot_button.size = Vector2(220, 105)
		pilot_button.pressed.connect(_select_pilot.bind(index))
		menu_panel.add_child(pilot_button)
		pilot_buttons.append(pilot_button)

	var start_button = Button.new()
	start_button.text = "DÉMARRER LE CHAMPIONNAT"
	start_button.add_theme_font_size_override("font_size", 27)
	start_button.set_anchors_preset(Control.PRESET_CENTER)
	start_button.position = Vector2(-250, 125)
	start_button.size = Vector2(500, 84)
	start_button.pressed.connect(_start_championship)
	menu_panel.add_child(start_button)

	race_panel = Control.new()
	race_panel.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	canvas.add_child(race_panel)

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

	var left_button = _make_touch_button("◀", Vector2(34, -190), Vector2(150, 150), Control.PRESET_BOTTOM_LEFT)
	left_button.button_down.connect(_left_down)
	left_button.button_up.connect(_left_up)
	race_panel.add_child(left_button)

	var right_button = _make_touch_button("▶", Vector2(204, -190), Vector2(150, 150), Control.PRESET_BOTTOM_LEFT)
	right_button.button_down.connect(_right_down)
	right_button.button_up.connect(_right_up)
	race_panel.add_child(right_button)

	var drift_button = _make_touch_button("DRIFT", Vector2(-380, -190), Vector2(160, 150), Control.PRESET_BOTTOM_RIGHT)
	drift_button.button_down.connect(_drift_down)
	drift_button.button_up.connect(_drift_up)
	race_panel.add_child(drift_button)

	var brake_button = _make_touch_button("FREIN", Vector2(-200, -190), Vector2(160, 150), Control.PRESET_BOTTOM_RIGHT)
	brake_button.button_down.connect(_brake_down)
	brake_button.button_up.connect(_brake_up)
	race_panel.add_child(brake_button)

	item_button = _make_touch_button("TURBO", Vector2(-200, 24), Vector2(160, 115), Control.PRESET_TOP_RIGHT)
	item_button.pressed.connect(_use_item)
	race_panel.add_child(item_button)

	result_panel = Control.new()
	result_panel.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	canvas.add_child(result_panel)
	var result_shade = ColorRect.new()
	result_shade.color = Color(0.01, 0.02, 0.06, 0.90)
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

	next_button = Button.new()
	next_button.text = "COURSE SUIVANTE"
	next_button.add_theme_font_size_override("font_size", 28)
	next_button.set_anchors_preset(Control.PRESET_CENTER)
	next_button.position = Vector2(-210, 150)
	next_button.size = Vector2(420, 82)
	next_button.pressed.connect(_on_next_pressed)
	result_panel.add_child(next_button)

func _make_touch_button(text_value: String, button_position: Vector2, button_size: Vector2, preset: int) -> Button:
	var button = Button.new()
	button.text = text_value
	button.add_theme_font_size_override("font_size", 25)
	button.set_anchors_preset(preset)
	button.position = button_position
	button.size = button_size
	button.modulate = Color(1.0, 1.0, 1.0, 0.84)
	return button

func _left_down() -> void:
	steer_input = -1.0

func _left_up() -> void:
	if steer_input < 0.0:
		steer_input = 0.0

func _right_down() -> void:
	steer_input = 1.0

func _right_up() -> void:
	if steer_input > 0.0:
		steer_input = 0.0

func _drift_down() -> void:
	touch_drift = true

func _drift_up() -> void:
	touch_drift = false

func _brake_down() -> void:
	touch_brake = true

func _brake_up() -> void:
	touch_brake = false

func _select_pilot(index: int) -> void:
	selected_pilot = index
	for button_index in range(pilot_buttons.size()):
		pilot_buttons[button_index].disabled = button_index == selected_pilot

func _show_menu() -> void:
	game_state = "menu"
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
	game_state = "race"
	menu_panel.visible = false
	race_panel.visible = true
	result_panel.visible = false
	player_progress = 0.0
	player_lane = 0.0
	steer_input = 0.0
	touch_brake = false
	touch_drift = false
	boost_time = 0.0
	item_ready = true
	race_time = 0.0
	countdown = 3.5
	ai_progress = [-0.008, -0.016, -0.024]
	ai_lane = [-3.0, 0.0, 3.0]
	_clear_race_nodes()
	_build_track()
	_spawn_racers()

func _clear_race_nodes() -> void:
	if is_instance_valid(track_root):
		for child in track_root.get_children():
			child.queue_free()
	if is_instance_valid(racers_root):
		for child in racers_root.get_children():
			child.queue_free()
	ai_karts.clear()
	player_kart = null

func _build_track() -> void:
	var data: Dictionary = TRACKS[track_index]
	var radius: float = data["radius"]
	var width: float = data["width"]
	var segments = 96
	var segment_length = TAU * radius / float(segments)
	for index in range(segments):
		var angle = TAU * float(index) / float(segments)
		var segment = MeshInstance3D.new()
		var segment_mesh = BoxMesh.new()
		segment_mesh.size = Vector3(width, 0.5, segment_length * 1.10)
		var segment_material = StandardMaterial3D.new()
		var stripe = 0.78 + 0.12 * float(index % 2)
		segment_material.albedo_color = data["color"] * stripe
		segment_material.metallic = 0.25
		segment_material.roughness = 0.55
		segment_mesh.material = segment_material
		segment.mesh = segment_mesh
		segment.position = Vector3(cos(angle) * radius, 0.0, sin(angle) * radius)
		segment.rotation.y = -angle
		track_root.add_child(segment)

		if index % 4 == 0:
			_add_track_rail(angle, radius - width * 0.54, segment_length * 4.1, data["color"])
			_add_track_rail(angle, radius + width * 0.54, segment_length * 4.1, data["color"])

	var center = MeshInstance3D.new()
	var center_mesh = SphereMesh.new()
	center_mesh.radius = 7.0 + float(track_index) * 0.8
	center_mesh.height = center_mesh.radius * 2.0
	var center_material = StandardMaterial3D.new()
	center_material.albedo_color = data["sky"].lightened(0.08)
	center_material.emission_enabled = true
	center_material.emission = data["color"].darkened(0.25)
	center_material.emission_energy_multiplier = 1.3
	center_mesh.material = center_material
	center.mesh = center_mesh
	center.position = Vector3(0.0, -2.0, 0.0)
	track_root.add_child(center)

	var world = get_node("WorldEnvironment") as WorldEnvironment
	world.environment.background_color = data["sky"]

func _add_track_rail(angle: float, radius: float, length: float, rail_color: Color) -> void:
	var rail = MeshInstance3D.new()
	var rail_mesh = BoxMesh.new()
	rail_mesh.size = Vector3(0.35, 1.2, length)
	var rail_material = StandardMaterial3D.new()
	rail_material.albedo_color = rail_color.lightened(0.35)
	rail_material.emission_enabled = true
	rail_material.emission = rail_color
	rail_material.emission_energy_multiplier = 1.8
	rail_mesh.material = rail_material
	rail.mesh = rail_mesh
	rail.position = Vector3(cos(angle) * radius, 0.75, sin(angle) * radius)
	rail.rotation.y = -angle
	track_root.add_child(rail)

func _spawn_racers() -> void:
	player_kart = _create_kart(selected_pilot, true)
	racers_root.add_child(player_kart)
	var available_pilots: Array[int] = []
	for pilot_index in range(4):
		if pilot_index != selected_pilot:
			available_pilots.append(pilot_index)
	for ai_index in range(3):
		var kart = _create_kart(available_pilots[ai_index], false)
		kart.set_meta("pilot_index", available_pilots[ai_index])
		racers_root.add_child(kart)
		ai_karts.append(kart)
	_update_racer_transforms(0.0)

func _create_kart(pilot_index: int, is_player: bool) -> Node3D:
	var root = Node3D.new()
	root.name = ("Player_" if is_player else "AI_") + str(PILOTS[pilot_index]["name"])
	root.set_meta("pilot_index", pilot_index)

	var body = MeshInstance3D.new()
	var body_mesh = BoxMesh.new()
	body_mesh.size = Vector3(2.15, 0.65, 3.25)
	var body_material = StandardMaterial3D.new()
	body_material.albedo_color = PILOTS[pilot_index]["accent"]
	body_material.metallic = 0.55
	body_material.roughness = 0.28
	body_mesh.material = body_material
	body.mesh = body_mesh
	body.position.y = 0.75
	root.add_child(body)

	var nose = MeshInstance3D.new()
	var nose_mesh = BoxMesh.new()
	nose_mesh.size = Vector3(1.65, 0.32, 1.65)
	var nose_material = StandardMaterial3D.new()
	nose_material.albedo_color = PILOTS[pilot_index]["color"]
	nose_material.emission_enabled = true
	nose_material.emission = PILOTS[pilot_index]["color"].darkened(0.25)
	nose_material.emission_energy_multiplier = 1.15
	nose_mesh.material = nose_material
	nose.mesh = nose_mesh
	nose.position = Vector3(0.0, 0.82, -1.85)
	root.add_child(nose)

	var wheel_positions = [
		Vector3(-1.25, 0.45, -1.05), Vector3(1.25, 0.45, -1.05),
		Vector3(-1.25, 0.45, 1.05), Vector3(1.25, 0.45, 1.05)
	]
	for wheel_position in wheel_positions:
		var wheel = MeshInstance3D.new()
		var wheel_mesh = CylinderMesh.new()
		wheel_mesh.top_radius = 0.42
		wheel_mesh.bottom_radius = 0.42
		wheel_mesh.height = 0.38
		var wheel_material = StandardMaterial3D.new()
		wheel_material.albedo_color = Color("101217")
		wheel_mesh.material = wheel_material
		wheel.mesh = wheel_mesh
		wheel.position = wheel_position
		wheel.rotation.z = PI * 0.5
		root.add_child(wheel)

	var pilot_holder = Node3D.new()
	pilot_holder.name = "PilotHolder"
	pilot_holder.position = Vector3(0.0, 0.7, 0.45)
	root.add_child(pilot_holder)
	_add_pilot_visual(pilot_holder, pilot_index)
	return root

func _add_pilot_visual(holder: Node3D, pilot_index: int) -> void:
	var path = "res://private_models/%s.glb" % str(PILOTS[pilot_index]["id"])
	if ResourceLoader.exists(path):
		var packed_scene = load(path) as PackedScene
		if packed_scene != null:
			var model = packed_scene.instantiate()
			model.name = "RealPilotModel"
			model.scale = Vector3.ONE * 0.72
			model.rotation.y = PI
			holder.add_child(model)
			var animation_player = _find_animation_player(model)
			if animation_player != null and animation_player.has_animation("drive_idle"):
				animation_player.play("drive_idle")
			return

	var torso = MeshInstance3D.new()
	var torso_mesh = CapsuleMesh.new()
	torso_mesh.radius = 0.48
	torso_mesh.height = 1.35
	var torso_material = StandardMaterial3D.new()
	torso_material.albedo_color = PILOTS[pilot_index]["color"]
	torso_mesh.material = torso_material
	torso.mesh = torso_mesh
	torso.position = Vector3(0.0, 1.05, 0.0)
	holder.add_child(torso)

	var head = MeshInstance3D.new()
	var head_mesh = SphereMesh.new()
	head_mesh.radius = 0.42
	head_mesh.height = 0.84
	var head_material = StandardMaterial3D.new()
	if pilot_index == 3:
		head_material.albedo_color = Color("b78361")
	else:
		head_material.albedo_color = Color("8a5b43")
	head_mesh.material = head_material
	head.mesh = head_mesh
	head.position = Vector3(0.0, 2.0, -0.05)
	holder.add_child(head)

func _find_animation_player(node: Node) -> AnimationPlayer:
	if node is AnimationPlayer:
		return node as AnimationPlayer
	for child in node.get_children():
		var found = _find_animation_player(child)
		if found != null:
			return found
	return null

func _process(delta: float) -> void:
	if game_state != "race":
		return
	_update_keyboard_input()
	if countdown > 0.0:
		countdown -= delta
		if countdown > 0.35:
			countdown_label.text = str(int(ceil(countdown)))
		else:
			countdown_label.text = "GO !"
		if countdown <= 0.0:
			countdown_label.text = ""
		_update_racer_transforms(delta)
		return

	race_time += delta
	if boost_time > 0.0:
		boost_time -= delta
	var pilot: Dictionary = PILOTS[selected_pilot]
	var speed: float = pilot["speed"]
	var braking = touch_brake or Input.is_action_pressed("brake")
	var drifting = touch_drift or Input.is_action_pressed("drift")
	if braking:
		speed *= 0.42
	if drifting and abs(steer_input) > 0.1:
		speed *= 0.92
	if boost_time > 0.0:
		speed *= 1.55
	player_progress += speed * delta
	var lane_target = steer_input * float(TRACKS[track_index]["width"]) * 0.34
	player_lane = move_toward(player_lane, lane_target, float(pilot["handling"]) * delta * 2.2)

	for ai_index in range(3):
		var wobble = sin(race_time * (0.9 + float(ai_index) * 0.17) + float(ai_index)) * 0.006
		ai_progress[ai_index] += (ai_speed[ai_index] + wobble + float(track_index) * 0.0008) * delta
		var ai_target = sin(race_time * 0.55 + float(ai_index) * 2.1) * float(TRACKS[track_index]["width"]) * 0.26
		ai_lane[ai_index] = lerp(ai_lane[ai_index], ai_target, delta * 0.7)

	_update_racer_transforms(delta)
	_update_hud()
	_check_finish()

func _update_keyboard_input() -> void:
	var keyboard_steer = Input.get_axis("steer_left", "steer_right")
	if abs(keyboard_steer) > 0.05:
		steer_input = keyboard_steer
	elif not Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT):
		steer_input = 0.0
	if Input.is_action_just_pressed("use_item"):
		_use_item()

func _update_racer_transforms(delta: float) -> void:
	if not is_instance_valid(player_kart):
		return
	_place_kart(player_kart, player_progress, player_lane, touch_drift or Input.is_action_pressed("drift"))
	for ai_index in range(ai_karts.size()):
		_place_kart(ai_karts[ai_index], ai_progress[ai_index], ai_lane[ai_index], false)

	var data: Dictionary = TRACKS[track_index]
	var angle = fmod(player_progress, 1.0) * TAU
	var radial = Vector3(cos(angle), 0.0, sin(angle))
	var tangent = Vector3(-sin(angle), 0.0, cos(angle))
	var target = radial * (float(data["radius"]) + player_lane) + Vector3.UP * 1.1
	var camera_target = target - tangent * 10.5 + Vector3.UP * 6.0
	camera.position = camera.position.lerp(camera_target, clamp(delta * 5.0, 0.0, 1.0))
	camera.look_at(target + tangent * 4.5 + Vector3.UP, Vector3.UP)

func _place_kart(kart: Node3D, progress: float, lane: float, drifting: bool) -> void:
	var data: Dictionary = TRACKS[track_index]
	var angle = fmod(progress, 1.0) * TAU
	var radial = Vector3(cos(angle), 0.0, sin(angle))
	var tangent = Vector3(-sin(angle), 0.0, cos(angle))
	var position = radial * (float(data["radius"]) + lane) + Vector3.UP * 0.55
	kart.position = position
	kart.look_at(position + tangent, Vector3.UP)
	if drifting and kart == player_kart:
		kart.rotation.z = lerp(kart.rotation.z, -steer_input * 0.13, 0.18)
	else:
		kart.rotation.z = lerp(kart.rotation.z, 0.0, 0.18)

func _update_hud() -> void:
	var lap = min(TOTAL_LAPS, int(floor(max(player_progress, 0.0))) + 1)
	var ranking = _current_ranking()
	var place = ranking.find(selected_pilot) + 1
	hud_label.text = "%s\n%s  •  Tour %d/%d\nPosition %d/4  •  %.1f s" % [
		TRACKS[track_index]["name"], PILOTS[selected_pilot]["name"], lap,
		TOTAL_LAPS, place, race_time
	]
	item_button.text = "TURBO" if item_ready else "VIDE"
	item_button.disabled = not item_ready

func _current_ranking() -> Array[int]:
	var entries: Array[Dictionary] = []
	entries.append({"pilot": selected_pilot, "progress": player_progress})
	for ai_index in range(3):
		entries.append({"pilot": int(ai_karts[ai_index].get_meta("pilot_index")), "progress": ai_progress[ai_index]})
	entries.sort_custom(_sort_race_entry)
	var ranking: Array[int] = []
	for entry in entries:
		ranking.append(int(entry["pilot"]))
	return ranking

func _sort_race_entry(first: Dictionary, second: Dictionary) -> bool:
	return float(first["progress"]) > float(second["progress"])

func _check_finish() -> void:
	if player_progress < float(TOTAL_LAPS):
		return
	var ranking = _current_ranking()
	for place_index in range(4):
		championship_points[ranking[place_index]] += POINTS[place_index]
	_show_race_result(ranking)

func _show_race_result(ranking: Array[int]) -> void:
	game_state = "result"
	race_panel.visible = false
	result_panel.visible = true
	next_button.text = "COURSE SUIVANTE"
	var lines: Array[String] = []
	lines.append("RÉSULTAT — %s" % str(TRACKS[track_index]["name"]))
	lines.append("")
	for place_index in range(ranking.size()):
		lines.append("%d. %s  +%d pts" % [place_index + 1, PILOTS[ranking[place_index]]["name"], POINTS[place_index]])
	lines.append("")
	lines.append("Championnat : %d points" % championship_points[selected_pilot])
	result_label.text = "\n".join(lines)

func _on_next_pressed() -> void:
	if game_state == "championship_end":
		_show_menu()
		return
	track_index += 1
	if track_index >= TRACKS.size():
		_show_championship_result()
	else:
		_start_race()

func _show_championship_result() -> void:
	game_state = "championship_end"
	var ranking: Array[int] = [0, 1, 2, 3]
	ranking.sort_custom(_sort_championship_entry)
	var lines: Array[String] = []
	lines.append("CHAMPIONNAT TERMINÉ")
	lines.append("")
	for place_index in range(ranking.size()):
		var pilot_index = ranking[place_index]
		lines.append("%d. %s — %d points" % [place_index + 1, PILOTS[pilot_index]["name"], championship_points[pilot_index]])
	result_label.text = "\n".join(lines)
	next_button.text = "RECOMMENCER"

func _sort_championship_entry(first: int, second: int) -> bool:
	return championship_points[first] > championship_points[second]

func _use_item() -> void:
	if game_state != "race" or countdown > 0.0 or not item_ready:
		return
	item_ready = false
	boost_time = 2.2

func _input(event: InputEvent) -> void:
	if game_state != "race":
		return
	if event is InputEventScreenDrag:
		var drag_event = event as InputEventScreenDrag
		var viewport_width = get_viewport().get_visible_rect().size.x
		if drag_event.position.x < viewport_width * 0.5:
			steer_input = clamp((drag_event.position.x / (viewport_width * 0.5) - 0.5) * 2.0, -1.0, 1.0)
