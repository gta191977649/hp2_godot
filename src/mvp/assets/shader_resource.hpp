#pragma once

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/shader.hpp>

#include <stdexcept>
#include <string>

namespace hp2 {

inline godot::Ref<godot::Shader> load_shader(const godot::String& path) {
    godot::Ref<godot::Shader> shader = godot::ResourceLoader::get_singleton()->load(path, "Shader");
    if (shader.is_null()) throw std::runtime_error("Unable to load shader " + std::string(path.utf8().get_data()));
    return shader;
}

}
