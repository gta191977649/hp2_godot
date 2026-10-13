#include "ps2_event_tree.hpp"
#include "platform/ps2_asset_root.hpp"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <cstring>

namespace port::godot_bridge {
namespace {

std::string search_start_dir() {
    godot::String project_path = godot::ProjectSettings::get_singleton()->globalize_path("res://");
    if (!project_path.is_empty()) return std::string(project_path.utf8().get_data());
    return std::string(
        godot::OS::get_singleton()->get_executable_path().get_base_dir().utf8().get_data());
}

} // namespace

void Ps2EventTree::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("load", "relative_path", "mode_id"),
                                &Ps2EventTree::load);
    godot::ClassDB::bind_method(godot::D_METHOD("get_nodes"), &Ps2EventTree::get_nodes);
    godot::ClassDB::bind_method(godot::D_METHOD("get_cursor"), &Ps2EventTree::get_cursor);
    godot::ClassDB::bind_method(godot::D_METHOD("move_cursor", "direction"),
                                &Ps2EventTree::move_cursor);
    godot::ClassDB::bind_method(godot::D_METHOD("get_cursor_info"), &Ps2EventTree::get_cursor_info);
    godot::ClassDB::bind_method(godot::D_METHOD("get_last_error"), &Ps2EventTree::get_last_error);
}

const hp2::frontend::EventTree_001FFC40* Ps2EventTree::tree() const {
    if (selected_tree_ < 0 || selected_tree_ >= static_cast<int>(trees_.size())) return nullptr;
    return &trees_[static_cast<std::size_t>(selected_tree_)];
}

godot::Dictionary Ps2EventTree::load(const godot::String& relative_path, int mode_id) {
    godot::Dictionary result;
    trees_.clear();
    selected_tree_ = -1;
    cursor_ = -1;

    auto root = hp2::platform::resolve_zzdata_root(search_start_dir());
    if (!root) {
        last_error_ = "PS2 asset root not configured (set HP2_PS2_ASSET_ROOT or create "
                      "config/ps2_assets.local.json).";
        result["ok"] = false;
        result["error"] = last_error_;
        return result;
    }
    godot::String full_path = godot::String::utf8(root->c_str()).path_join(relative_path);
    godot::Ref<godot::FileAccess> file =
        godot::FileAccess::open(full_path, godot::FileAccess::READ);
    if (file.is_null()) {
        last_error_ = godot::String("Could not open ") + full_path;
        result["ok"] = false;
        result["error"] = last_error_;
        return result;
    }
    godot::PackedByteArray bytes = file->get_buffer(file->get_length());
    file->close();
    std::vector<std::uint8_t> buffer(static_cast<std::size_t>(bytes.size()));
    if (!buffer.empty()) std::memcpy(buffer.data(), bytes.ptr(), buffer.size());

    trees_ = hp2::frontend::parse_event_trees(buffer);
    // FUN_00185568: profile+0x1072C == 8 selects "Championship", == 9 selects "Hot Pursuit".
    const int wanted_type = mode_id == 9 ? 1 : (mode_id == 8 ? 0 : -1);
    for (std::size_t i = 0; i < trees_.size(); ++i) {
        if (trees_[i].type_0x08 == wanted_type) selected_tree_ = static_cast<int>(i);
    }
    const auto* selected = tree();
    if (selected == nullptr || selected->records.empty()) {
        last_error_ = godot::String("no event tree for mode id ") + godot::String::num_int64(mode_id);
        result["ok"] = false;
        result["error"] = last_error_;
        return result;
    }

    // FUN_001F96B8 takes the starting node from the profile (FUN_001F5D00). Without a profile the
    // port starts on event 1, which is the only node that is unlocked in that state.
    cursor_ = selected->FindLeafByNumber_001FA840(1);
    last_error_ = "";
    result["ok"] = true;
    result["tree_name"] = godot::String::utf8(selected->name.c_str());
    result["tree_type"] = selected->type_0x08;
    result["node_count"] = static_cast<int>(selected->records.size());
    result["error"] = "";
    return result;
}

godot::Array Ps2EventTree::get_nodes() const {
    godot::Array nodes;
    const auto* selected = tree();
    if (selected == nullptr) return nodes;
    for (std::size_t i = 0; i < selected->leaves.size(); ++i) {
        const auto& leaf = selected->leaves[i];
        const auto& record = selected->records[i];
        const int status = selected->Status_002010B8(static_cast<int>(i));
        godot::Dictionary node;
        node["index"] = static_cast<int>(i);
        node["number"] = record.number_0x28;
        node["name"] = godot::String::utf8(record.name_0x08.c_str());
        node["x"] = leaf.x;
        node["y"] = leaf.y;
        node["depth"] = leaf.depth_0x1C;
        node["group"] = leaf.group;
        node["status"] = status;
        node["string_id"] = static_cast<std::int64_t>(record.string_id_0xB0);
        const char* panel =
            hp2::frontend::EventPanelTexture_001F75F0(selected->type_0x08, status);
        node["panel_texture"] = panel == nullptr ? godot::String() : godot::String::utf8(panel);
        node["selected"] = static_cast<int>(i) == cursor_;
        // FUN_001FA9C8 writes 2.0f into the node it moves to and 1.0f into the one it leaves.
        node["scale"] = static_cast<int>(i) == cursor_ ? 2.0f : 1.0f;
        godot::PackedInt32Array links;
        for (int other : leaf.outgoing) links.push_back(other);
        node["links"] = links;
        nodes.append(node);
    }
    return nodes;
}

bool Ps2EventTree::move_cursor(int direction) {
    const auto* selected = tree();
    if (selected == nullptr || cursor_ < 0) return false;
    const auto& leaf = selected->leaves[static_cast<std::size_t>(cursor_)];
    // FUN_001FA9C8: -1/+1 take the node's own link list, +2 the one the view threads forward and
    // -2 the one it threads back. The port models those three lists as the node's outgoing and
    // incoming edges; which concrete neighbour each ordinal picks within a list is the order the
    // view built them in.
    const std::vector<int>* list = nullptr;
    int step = 0;
    if (direction == -1 || direction == 1) {
        list = &leaf.outgoing;
        step = direction;
    } else if (direction == 2) {
        list = &leaf.outgoing;
        step = 1;
    } else if (direction == -2) {
        list = &leaf.incoming;
        step = 1;
    }
    if (list == nullptr || list->empty()) return false;

    int target = -1;
    if (direction == -1 || direction == 1) {
        // Walk the siblings that share this node's depth, in the order the list holds them.
        const int at = cursor_;
        std::vector<int> row;
        for (std::size_t i = 0; i < selected->leaves.size(); ++i) {
            if (selected->leaves[i].depth_0x1C == leaf.depth_0x1C) row.push_back(static_cast<int>(i));
        }
        for (std::size_t i = 0; i < row.size(); ++i) {
            if (row[i] != at) continue;
            const int next = static_cast<int>(i) + step;
            if (next >= 0 && next < static_cast<int>(row.size())) target = row[static_cast<std::size_t>(next)];
            break;
        }
    } else {
        target = (*list)[0];
    }
    if (target < 0 || target == cursor_) return false;
    cursor_ = target;
    return true;
}

godot::Dictionary Ps2EventTree::get_cursor_info() const {
    godot::Dictionary info;
    const auto* selected = tree();
    if (selected == nullptr || cursor_ < 0) return info;
    const auto& record = selected->records[static_cast<std::size_t>(cursor_)];
    const int status = selected->Status_002010B8(cursor_);
    info["index"] = cursor_;
    info["number"] = record.number_0x28;
    info["name"] = godot::String::utf8(record.name_0x08.c_str());
    info["status"] = status;
    info["string_id"] = static_cast<std::int64_t>(record.string_id_0xB0);
    const char* panel = hp2::frontend::EventPanelTexture_001F75F0(selected->type_0x08, status);
    info["panel_texture"] = panel == nullptr ? godot::String() : godot::String::utf8(panel);
    return info;
}

} // namespace port::godot_bridge
