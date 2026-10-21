extends RefCounted
# Port presentation of the active runtime driver, never the requested project setting.
static func label(driver: String, detail: String = "") -> String:
    match driver.to_lower():
        "opengl3":return "OpenGL"
        "opengl3_angle":
            var backend := detail.to_lower()
            return "OpenGL / DX11" if "direct3d11" in backend or "d3d11" in backend else "OpenGL (ANGLE)"
        "d3d12":return "DX12"
        "d3d11":return "DX11"
        "vulkan":return "Vulkan"
        "metal":return "Metal"
        "", "dummy":return "Headless"
    return driver
static func current() -> String:
    if DisplayServer.get_name()=="headless":return "Headless"
    return label(RenderingServer.get_current_rendering_driver_name(),RenderingServer.get_video_adapter_name()+" "+RenderingServer.get_video_adapter_api_version())
