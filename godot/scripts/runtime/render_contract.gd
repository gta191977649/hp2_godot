extends Node
# TASK0071 port-side startup contract. Explicit backend commands are diagnostic.
func _ready() -> void:
    if DisplayServer.get_name() == "headless":
        return
    var arguments := OS.get_cmdline_args()
    var diagnostic := "--rendering-method" in arguments or "--rendering-driver" in arguments
    var method := RenderingServer.get_current_rendering_method()
    var driver := RenderingServer.get_current_rendering_driver_name()
    if not diagnostic and (method != "forward_plus" or driver != "vulkan"):
        push_error("TASK0071 requires Vulkan + Forward+; actual backend: " + driver + "/" + method)
        get_tree().quit(1)
        return
    print("HP2_RENDER_BACKEND driver=", driver, " method=", method, " explicit_diagnostic=", diagnostic)
