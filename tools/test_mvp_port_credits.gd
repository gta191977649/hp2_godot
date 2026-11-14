extends "test_mvp_debug_menu.gd"

const PORT := "Port Credits"
const PORT_ITEM := 0x52e039f8
const MAKING_ITEM := 0x52e039f6
const REWARD_ITEM := 0x52e039f7
const CREDITS := "Credits"

func run() -> void:
    deadline = Time.get_ticks_msec() + 65000
    change_scene_to_file("res://native_game.tscn")
    while not is_instance_valid(race):
        await process_frame
        race = root.get_node_or_null("NativeGame/RacePresentation")
    await wait_screen(TITLE)
    await wait_for(func(state: Dictionary) -> bool: return state.get("video_request") == 2)
    await key(KEY_ENTER)
    await wait_for(func(state: Dictionary) -> bool: return state.get("video_request") == 0)
    await create_timer(4.5).timeout
    await key(KEY_ENTER)
    await wait_screen(MAIN)
    await select(FIRST + 2)
    await key(KEY_ENTER)
    await wait_screen("Options")
    await select(FIRST + 5)
    await key(KEY_ENTER)
    await wait_screen(CREDITS)
    await create_timer(0.7).timeout
    await key(KEY_ENTER)
    await wait_screen("EACredits")
    await capture("ea-credits")
    await key(KEY_BACKSPACE)
    await wait_screen(CREDITS)
    await select(MAKING_ITEM)
    await pad(JOY_BUTTON_DPAD_DOWN)
    if race.telemetry().get("selected_item") != PORT_ITEM:
        fail("Port Credits is not immediately after Making of HP2")
        return
    await key(KEY_DOWN)
    var unlocked := OS.get_environment("HP2_MVP_TEST_UNLOCK_RACES") == "1"
    var expected := REWARD_ITEM if unlocked else FIRST
    if race.telemetry().get("selected_item") != expected:
        fail("unexpected reward row visibility or position")
        return
    await key(KEY_UP)
    await key(KEY_ENTER)
    await wait_screen(PORT)
    await create_timer(1.7).timeout
    await capture("port-credits-keyboard")
    var fallback := race.get_node_or_null("FrontendCanvas/PortCreditsFallback")
    var email := (fallback.get_child(0) if is_instance_valid(fallback) and fallback.get_child_count() else null) as Label
    if not is_instance_valid(email) or email.text != "Email: episodes27@gmail.com" or not email.is_visible_in_tree():
        fail("complete email is missing")
        return
    await pad(JOY_BUTTON_X)
    var frozen := email.position.y
    await create_timer(0.4).timeout
    if email.position.y != frozen:
        fail("credits scroll did not pause")
        return
    await key(KEY_SPACE)
    if email.position.y >= frozen:
        fail("credits scroll did not resume")
        return
    await pad(JOY_BUTTON_B)
    await wait_screen(CREDITS)
    await create_timer(0.7).timeout
    var canvas := race.get_node("FrontendCanvas") as Node2D
    await click_at(canvas.to_global(Vector2(232, 293 if unlocked else 303)))
    await wait_screen(PORT)
    await create_timer(1.7).timeout
    await capture("port-credits-pointer")
    var second_fallback := race.get_node_or_null("FrontendCanvas/PortCreditsFallback") as Control
    if not is_instance_valid(second_fallback) or not second_fallback.is_visible_in_tree() or \
            (second_fallback.get_child(0) as Label).text != "Email: episodes27@gmail.com":
        fail("reopened credits did not restart at the beginning")
        return
    await create_timer(3.2).timeout
    await capture("port-credits-projects")
    if not OS.get_environment("HP2_MVP_SCREENSHOT_DIR").is_empty():
        await create_timer(6.3).timeout
        await capture("port-credits-libraries")
    await key(KEY_BACKSPACE)
    await wait_screen(CREDITS)
    if race.has_node("FrontendCanvas/PortCreditsFallback"):
        fail("fallback labels survived leaving Port Credits")
        return
    await select(FIRST)
    await key(KEY_ENTER)
    await wait_screen("EACredits")
    await create_timer(1.7).timeout
    await capture("ea-credits-after-port")
    print("HP2_MVP_PORT_CREDITS_PASS reward_unlocked=", unlocked)
    quit()
