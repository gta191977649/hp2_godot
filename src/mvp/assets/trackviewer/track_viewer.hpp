#pragma once

#include <cstdint>
#include <memory>

namespace godot {
class InputEvent;
class Node3D;
template<class T> class Ref;
}

namespace hp2 {
struct MainMenuScreenServices;
struct RaceEffectOptions;
class GameResources;

class TrackViewer {
public:
    enum class Action { None, Open, Load, Close };
    struct Request { Action action{}; int track{}; };

    explicit TrackViewer(godot::Node3D& host);
    ~TrackViewer();
    void open(bool inRace, int currentTrack, int language);
    void show();
    Request take_request();
    void begin_load(int track, const GameResources& files);
    void update_scene(double delta, const RaceEffectOptions& options);
    void error(const char* message);
    void close();
    bool active() const;
    bool modal() const;
    bool captured() const;
    bool input(const godot::Ref<godot::InputEvent>& event);
    void draw(float width, float height, float dpi, float fps, bool blocked, RaceEffectOptions& effects);
    void advance(double delta, bool blocked);
    void detach();
    int track() const;
    std::uint64_t generation() const;
    std::uint32_t step() const;
    std::size_t placement_count() const;
    std::size_t scenery_section_count() const;
    int entity_count() const;
    int catalogue_size() const;

    static void return_to_main_menu(MainMenuScreenServices& menus, const void*);

private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};
}
