extends "test_mvp_menu_navigation.gd"

const GRAPHIC_ROW := FIRST + 7
const RESTORE_ROW := 0xaee77883
const WINDOWS_APIS := {"vulkan": "Vulkan", "d3d12": "DirectX 12"}

var settings: Node

func values() -> PackedStringArray:
    return settings.menu_settings(-1, 0)

func expect(condition: bool, description: String) -> void:
    if not condition:
        fail(description)
        assert(condition, description)

func run() -> void:
    deadline = Time.get_ticks_msec() + 120000
    change_scene_to_file("res://native_game.tscn")
    while not is_instance_valid(race):
        await process_frame
        race = root.get_node_or_null("NativeGame/RacePresentation")
    settings = root.get_node("NativeGame/GraphicsSettings")
    match OS.get_environment("HP2_MVP_GRAPHICS_SMOKE"):
        "restart":
            var driver := RenderingServer.get_current_rendering_driver_name()
            expect(driver == OS.get_environment("HP2_GRAPHIC_EXPECT_DRIVER"), "wrong API after restart: " + driver)
            expect(values()[0] == WINDOWS_APIS[driver], "stored driver label differs from running driver")
            expect(values()[7] == "On" and settings.get_node("FpsOverlay/FPS").visible, "FPS setting did not persist")
        "race":
            await check_race_aa()
        _:
            await graphic_menu()
    print("GRAPHIC_MENU_PASS ", OS.get_environment("HP2_MVP_GRAPHICS_SMOKE"))
    quit(0)

func graphic_menu() -> void:
    var fps := settings.get_node("FpsOverlay/FPS") as Label
    expect(values()[7] == "Off" and not fps.visible, "FPS overlay default must be Off")
    expect(Engine.max_fps == 0 and values()[4] == "No Restriction", "frame rate cap default")
    expect(values()[3] == "Auto Fit", "Aspect Ratio must default to Auto Fit")
    await wait_screen(TITLE)
    await wait_for(func(state: Dictionary) -> bool: return state.get("video_request") == 2)
    await key(KEY_ENTER)
    await wait_for(func(state: Dictionary) -> bool: return state.get("video_request") == 0)
    await create_timer(4.5).timeout
    await key(KEY_ENTER)
    await wait_screen(MAIN)
    await visit(2, "Options")
    await select(FIRST + 2)
    await pad(JOY_BUTTON_DPAD_DOWN)
    expect(race.telemetry().selected_item == GRAPHIC_ROW, "Graphic must immediately follow Display")
    check_rails(true)
    await capture("Options-Graphic")
    await pad(JOY_BUTTON_A)
    await wait_screen("Graphic")
    await create_timer(0.45).timeout
    await capture("Graphic")
    await check_scrolling_menu(8)

    await select(FIRST + 1)
    await key(KEY_LEFT)
    expect(root.mode == Window.MODE_FULLSCREEN and values()[1] == "Borderless", "Borderless mode did not apply")
    await check_fullscreen_resolution()
    await select(FIRST + 1)
    await key(KEY_LEFT)
    expect(root.mode == Window.MODE_EXCLUSIVE_FULLSCREEN and values()[1] == "Full Screen", "Full Screen did not apply")
    await check_fullscreen_resolution()
    await select(FIRST + 1)
    await key(KEY_LEFT)
    expect(root.mode == Window.MODE_WINDOWED and values()[1] == "Window", "Window mode did not restore")
    root.size = Vector2i(1040, 680)
    await create_timer(0.5).timeout
    expect(values()[2] == "%d x %d" % [root.size.x, root.size.y], "current resolution did not update")
    await select(FIRST + 2)
    await key(KEY_RIGHT)
    expect(values()[9] == "0" and root.size == Vector2i(1040, 680), "window resolution must be read-only")
    await select(FIRST + 3)
    for ratio in [16.0 / 9.0, 1.6, 4.0 / 3.0]:
        await key(KEY_RIGHT)
        var size := root.get_visible_rect().size
        expect(absf(size.x / size.y - ratio) < 0.004, "game viewport aspect did not apply: %s" % size)
        expect(root.size == Vector2i(1040, 680), "aspect selection resized the window")
    await key(KEY_RIGHT)
    expect(values()[3] == "Auto Fit" and Vector2i(root.get_visible_rect().size) == root.size,
        "Auto Fit did not fill the window")
    await select(FIRST + 4)
    for cap in [120, 60, 30, 0]:
        await key(KEY_RIGHT)
        expect(Engine.max_fps == cap, "wrong frame rate cap")
    await select(FIRST + 5)
    await key(KEY_RIGHT)
    expect(DisplayServer.window_get_vsync_mode() == DisplayServer.VSYNC_DISABLED, "VSync Off did not apply")
    await key(KEY_RIGHT)
    expect(DisplayServer.window_get_vsync_mode() == DisplayServer.VSYNC_ENABLED, "VSync On did not apply")
    await select(FIRST + 6)
    for mode_id in ["smaa", "msaa2", "msaa4", "msaa8", "taa", "ssaa", "fsr2", "off", "fxaa"]:
        await select(FIRST + 6)
        await key(KEY_RIGHT)
        if race.telemetry().selected_item != FIRST + 6 and ProjectSettings.get_setting("hp2/graphics_aa") != mode_id:
            await select(FIRST + 6)
            await key(KEY_RIGHT)
        expect(ProjectSettings.get_setting("hp2/graphics_aa") == mode_id, "AA setting did not apply: " + mode_id)
    await select(FIRST + 7)
    await key(KEY_RIGHT)
    expect(fps.visible and fps.text.ends_with(" FPS"), "Release global FPS counter did not turn on")
    var dpi := DisplayServer.screen_get_dpi(root.current_screen)
    var dpi_scale := float(dpi) / 96.0 if dpi > 0 else 1.0
    expect(fps.get_theme_font_size("font_size") == roundi(28.0 * dpi_scale), "FPS counter size did not follow DPI")
    expect(fps.get_theme_constant("outline_size") == roundi(6.0 * dpi_scale)
        and fps.get_theme_color("font_outline_color") == Color.BLACK, "FPS counter needs its DPI-scaled black outline")
    await select(FIRST + 8)
    await create_timer(0.55).timeout
    await hover_row(FIRST + 8)
    var row: Rect2 = race.telemetry().menu_alignment_bounds[FIRST + 8]
    await click_at((race.get_node("FrontendCanvas") as Node2D).to_global(row.get_center()))
    await wait_for(func(state: Dictionary) -> bool: return state.get("modal", false))
    await capture("Advanced-Setting")
    await pad(JOY_BUTTON_A)
    await wait_for(func(state: Dictionary) -> bool: return not state.get("modal", false))
    expect(race.telemetry().selected_item == FIRST + 8, "Advanced modal lost the selection")
    await select(RESTORE_ROW)
    await create_timer(0.55).timeout
    check_rails(true)
    var config_path: String = settings.get_parent().launch_root.path_join("launcher.cfg")
    var saved := ConfigFile.new()
    expect(saved.load(config_path) == OK, "graphic options must be in launcher.cfg")
    var iso: Variant = saved.get_value("game", "iso_path", "")
    await key(KEY_ENTER)
    if race.telemetry().get("modal", false):
        await key(KEY_ENTER)
    expect(values()[1] == "Window" and values()[3] == "Auto Fit" and values()[4] == "No Restriction"
        and values()[5] == "On" and values()[6] == "FXAA" and values()[7] == "Off" and not fps.visible,
        "restore did not apply the graphic defaults")
    saved.clear()
    expect(saved.load(config_path) == OK and not saved.has_section("graphics")
        and saved.get_value("game", "iso_path", "") == iso, "restore must preserve the ISO path")
    await key(KEY_UP)
    expect(race.telemetry().selected_item == FIRST + 8, "Restore must follow Advanced Setting")
    await select(FIRST + 7)
    await key(KEY_RIGHT)
    await select(FIRST)
    var active_driver := RenderingServer.get_current_rendering_driver_name()
    await key(KEY_RIGHT)
    if values()[0] == WINDOWS_APIS[active_driver]:
        await key(KEY_RIGHT)
    await wait_for(func(state: Dictionary) -> bool: return state.get("modal", false))
    await key(KEY_ENTER)
    await wait_for(func(state: Dictionary) -> bool: return not state.get("modal", false))
    expect(RenderingServer.get_current_rendering_driver_name() == active_driver, "Render API changed before restart")
    expect(values()[0] != WINDOWS_APIS[active_driver], "new renderer was not saved")
    await back("Options")
    await visit(2, "Display Options")
    check_rails(true)
    expect(fps.visible, "FPS overlay disappeared on Display")
    await capture("Display-restored")

func check_fullscreen_resolution() -> void:
    await select(FIRST + 2)
    var initial := values()[2]
    var output_size := root.size
    expect(values()[9] == "1", "fullscreen resolution must accept input")
    await key(KEY_RIGHT)
    expect(values()[2] != initial and root.size == output_size, "fullscreen rendering resolution did not change")
    var dimensions := Vector2i(root.get_visible_rect().size)
    expect(values()[2] == "%d x %d" % [dimensions.x, dimensions.y], "fullscreen label does not match render resolution")
    expect(absf(float(dimensions.x) / dimensions.y - float(output_size.x) / output_size.y) < 0.005,
        "Auto Fit must match the fullscreen output aspect")
    await key(KEY_LEFT)
    expect(values()[2] == initial, "fullscreen resolution did not cycle back")

func check_race_aa() -> void:
    await wait_for(func(state: Dictionary) -> bool: return state.get("race_state") == 3)
    var view := root.find_child("RaceRenderTarget", true, false) as SubViewport
    expect(view != null, "race viewport missing")
    for mode_id in ["off", "fxaa", "smaa", "msaa2", "msaa4", "msaa8", "taa", "ssaa", "fsr2"]:
        ProjectSettings.set_setting("hp2/graphics_aa", mode_id)
        await create_timer(0.55).timeout
        var msaa := ["off", "msaa2", "msaa4", "msaa8"].find(mode_id)
        expect(view.msaa_3d == maxi(msaa, 0), "race viewport MSAA mismatch")
        expect(view.use_taa == (mode_id == "taa"), "race viewport TAA mismatch")
        expect(view.screen_space_aa == ["off", "fxaa", "smaa"].find(mode_id)
            if mode_id in ["fxaa", "smaa"] else view.screen_space_aa == 0, "race viewport screen-space AA mismatch")
        expect(is_equal_approx(view.scaling_3d_scale, 1.5 if mode_id == "ssaa" else 1.0), "race viewport SSAA mismatch")
        expect(view.scaling_3d_mode == (Viewport.SCALING_3D_MODE_FSR2 if mode_id == "fsr2"
            else Viewport.SCALING_3D_MODE_BILINEAR), "race viewport scaling mismatch")
        await capture("Race-" + mode_id)
