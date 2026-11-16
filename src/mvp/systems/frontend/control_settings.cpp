#include "control_settings.hpp"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <algorithm>
#include <cmath>

namespace hp2 {
using namespace godot;

namespace {
Key physical_key(const InputEventKey& event) {
    return event.get_physical_keycode() != KEY_NONE ? event.get_physical_keycode() : event.get_keycode();
}
}

ControlSettings::ControlSettings(Callable storage) : storage_(std::move(storage)) {
    auto& map = *InputMap::get_singleton();
    for (std::size_t i = 0; i < bindings.size(); ++i) {
        const auto events = map.action_get_events(bindings[i].action);
        for (int64_t j = 0; j < events.size(); ++j) {
            if (const Ref<InputEventKey> key = events[j]; key.is_valid()) {
                keys_[i] = defaults_[i] = physical_key(*key.ptr());
                break;
            }
        }
    }
    if (!storage_.is_valid()) return;
    const Dictionary saved = storage_.call();
    const Variant deadZone = saved.get("steering_dead_zone", defaultDeadZone);
    const Variant sensitivity = saved.get("steering_sensitivity", defaultSensitivity);
    if (deadZone.get_type() == Variant::INT && int64_t(deadZone) >= 0 && int64_t(deadZone) <= 50)
        deadZone_ = deadZone;
    if (sensitivity.get_type() == Variant::INT && int64_t(sensitivity) >= 50 && int64_t(sensitivity) <= 200)
        sensitivity_ = sensitivity;

    const Variant keyboard = saved.get("keyboard", Dictionary{});
    if (keyboard.get_type() != Variant::DICTIONARY) return;
    const Dictionary overrides = keyboard;
    auto candidate = defaults_;
    for (std::size_t i = 0; i < bindings.size(); ++i) {
        const Variant stored = overrides.get(bindings[i].action, defaults_[i]);
        if (stored.get_type() != Variant::INT || int64_t(stored) <= 0 || int64_t(stored) > KEY_UNKNOWN) continue;
        const auto key = static_cast<Key>(int64_t(stored));
        if (available_key(key)) candidate[i] = key;
    }
    // A new default can occupy a key from an old profile. Keep the other
    // saved bindings, including swaps whose final keys are still distinct.
    bool restored;
    do {
        restored = false;
        for (std::size_t i = 0; i < candidate.size(); ++i) {
            if (candidate[i] == defaults_[i] || conflict(candidate, i, candidate[i]) < 0) continue;
            candidate[i] = defaults_[i];
            restored = true;
        }
    } while (restored);
    apply(candidate);
}

ControlSettings::~ControlSettings() {
    if (InputMap::get_singleton() && Input::get_singleton()) apply(defaults_);
}

String ControlSettings::key_name(std::size_t row) const { return key_name(keys_[row]); }

String ControlSettings::key_name(Key key) {
    const auto local = DisplayServer::get_singleton()->keyboard_get_keycode_from_physical(key);
    return OS::get_singleton()->get_keycode_string(local == KEY_NONE ? key : local);
}

bool ControlSettings::available_key(Key key) const {
    if (key == KEY_NONE || key == KEY_UNKNOWN || OS::get_singleton()->get_keycode_string(key).is_empty())
        return false;
    auto& map = *InputMap::get_singleton();
    const auto actions = map.get_actions();
    for (int64_t i = 0; i < actions.size(); ++i) {
        const StringName action = actions[i];
        if (!String(action).begins_with("hp2_") || std::any_of(bindings.begin(), bindings.end(),
            [&](const Binding& binding) { return action == StringName(binding.action); })) continue;
        const auto events = map.action_get_events(action);
        for (int64_t j = 0; j < events.size(); ++j)
            if (const Ref<InputEventKey> fixed = events[j]; fixed.is_valid() && physical_key(*fixed.ptr()) == key)
                return false;
    }
    return true;
}

bool ControlSettings::overlaps(std::size_t first, std::size_t second) {
    const auto a = bindings[first].context;
    const auto b = bindings[second].context;
    return a == b || a == Context::Shared || b == Context::Shared;
}

int ControlSettings::conflict(const Keys& keys, std::size_t row, Key key) {
    for (std::size_t i = 0; i < keys.size(); ++i)
        if (i != row && keys[i] == key && overlaps(row, i)) return static_cast<int>(i);
    return -1;
}

int ControlSettings::conflict(std::size_t row, Key key) const { return conflict(keys_, row, key); }

ControlSettings::Keys ControlSettings::swapped_keys(std::size_t row, Key key) const {
    auto next = keys_;
    const int other = conflict(row, key);
    if (other >= 0) next[other] = next[row];
    next[row] = key;
    return next;
}

bool ControlSettings::can_bind(std::size_t row, Key key) const {
    if (!available_key(key)) return false;
    const auto next = swapped_keys(row, key);
    for (std::size_t i = 0; i < next.size(); ++i)
        if (conflict(next, i, next[i]) >= 0) return false;
    return true;
}

bool ControlSettings::bind(std::size_t row, Key key) {
    if (!can_bind(row, key)) return false;
    const auto next = swapped_keys(row, key);
    if (!save(next, deadZone_, sensitivity_)) return false;
    apply(next);
    return true;
}

bool ControlSettings::restore_keyboard() {
    if (!save(defaults_, deadZone_, sensitivity_)) return false;
    apply(defaults_);
    return true;
}

bool ControlSettings::tune_stick(int deadZone, int sensitivity) {
    deadZone = std::clamp(deadZone, 0, 50);
    sensitivity = std::clamp(sensitivity, 50, 200);
    if (deadZone == deadZone_ && sensitivity == sensitivity_) return true;
    if (!save(keys_, deadZone, sensitivity)) return false;
    deadZone_ = deadZone;
    sensitivity_ = sensitivity;
    return true;
}

float ControlSettings::steering(float value) const {
    if (std::abs(value) <= static_cast<float>(deadZone_) / 100.0f) return 0.0f;
    return std::clamp(value * (static_cast<float>(sensitivity_) / 100.0f), -1.0f, 1.0f);
}

bool ControlSettings::save(const Keys& keys, int deadZone, int sensitivity) const {
    if (!storage_.is_valid()) return false;
    Dictionary keyboard;
    for (std::size_t i = 0; i < keys.size(); ++i)
        if (keys[i] != defaults_[i]) keyboard[bindings[i].action] = keys[i];
    Dictionary next;
    next["keyboard"] = keyboard;
    next["steering_dead_zone"] = deadZone;
    next["steering_sensitivity"] = sensitivity;
    return bool(storage_.call(next));
}

void ControlSettings::apply(const Keys& keys) {
    auto& map = *InputMap::get_singleton();
    for (std::size_t i = 0; i < keys.size(); ++i) {
        if (keys[i] == keys_[i]) continue;
        const StringName action = bindings[i].action;
        Input::get_singleton()->action_release(action);
        const auto events = map.action_get_events(action);
        for (int64_t j = 0; j < events.size(); ++j) {
            const Ref<InputEvent> binding = events[j];
            if (const Ref<InputEventKey> key = binding; key.is_valid()) map.action_erase_event(action, binding);
        }
        Ref<InputEventKey> binding;
        binding.instantiate();
        binding->set_physical_keycode(keys[i]);
        map.action_add_event(action, binding);
    }
    keys_ = keys;
}

}
