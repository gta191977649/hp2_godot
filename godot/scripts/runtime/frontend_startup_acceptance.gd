extends SceneTree
# TASK-0104 regression: default boot, visible original movie, input through title.
var frontend: Control
var output := ""
var failures: Array[String] = []
func _initialize() -> void: call_deferred("_run")
func _press() -> void:
	# Do not enqueue another confirmation after startup has handed off to menus.
	if not frontend.startup_active or frontend.layout.menu_state().get("active_class","")=="New Main Menu": return
	var event := InputEventKey.new()
	event.keycode=KEY_ENTER
	event.physical_keycode=KEY_ENTER
	event.pressed=true
	Input.parse_input_event(event)
	await process_frame
	event=event.duplicate()
	event.pressed=false
	Input.parse_input_event(event)
	await process_frame
func _run() -> void:
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--controls-output="): output=argument.trim_prefix("--controls-output=")
	if output.is_empty() or DisplayServer.get_name()=="headless": quit(2); return
	frontend=load("res://scenes/frontend_native.tscn").instantiate()
	root.add_child(frontend)
	await create_timer(2.0).timeout
	await RenderingServer.frame_post_draw
	var movie_image := root.get_texture().get_image()
	var visible_pixels := 0
	for y in range(100,380):
		for x in range(70,500):
			var pixel := movie_image.get_pixel(x,y)
			if maxf(pixel.r,maxf(pixel.g,pixel.b))>0.15: visible_pixels+=1
	if visible_pixels<1000: failures.append("default boot movie is black")
	if movie_image.save_png(output.path_join("default-startup.png"))!=OK: failures.append("movie capture failed")
	var deadline := Time.get_ticks_msec()+40000
	while Time.get_ticks_msec()<deadline:
		if frontend.runtime_error_count>0: break
		if frontend.layout.menu_state().get("active_class","")=="New Main Menu" and not frontend.startup_active: break
		await _press()
		await create_timer(2.0).timeout
	if frontend.layout.menu_state().get("active_class","")!="New Main Menu" or frontend.startup_active:
		failures.append("default boot did not reach main menu with normal confirmation")
	await RenderingServer.frame_post_draw
	root.get_texture().get_image().save_png(output.path_join("default-main-menu.png"))
	var result := {"ok":failures.is_empty() and frontend.runtime_error_count==0,"failures":failures,"movie_visible_pixels":visible_pixels,"runtime_errors":frontend.runtime_error_count,"state":frontend.layout.menu_state()}
	var file := FileAccess.open(output.path_join("startup-result.json"),FileAccess.WRITE)
	file.store_string(JSON.stringify(result,"\t"))
	file.close()
	print("FRONTEND_DEFAULT_STARTUP_", "OK" if result.ok else "FAIL", " ",failures)
	frontend.queue_free()
	await process_frame
	await process_frame
	quit(0 if result.ok else 2)
