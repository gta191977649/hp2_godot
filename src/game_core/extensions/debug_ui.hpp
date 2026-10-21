#pragma once
#include "port/render/frame.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace port::extensions {
// Port-side input/presentation contract shared by console and viewer windows.
enum class Key { Tab, Left, Right, Up, Down, PageUp, PageDown, Home, End,
    Insert, Delete, Backspace, Space, Enter, Escape, KeyPadEnter, A, C, V, X, Y, Z, Count };
struct Input {
    enum class Kind { Key, Text, Mouse, Button, Wheel, FocusLost, Toggle } kind;
    int code=0;bool pressed=false,repeat=false,ctrl=false,shift=false,alt=false,super=false;
    float x=0,y=0;unsigned unicode=0;
};
struct Vertex {render::Vec2 position,uv;render::Color color;};
struct Batch {render::Rect clip;std::vector<Vertex> vertices;std::vector<int> indices;};
Batch convert_batch(const std::vector<Vertex>& vertices,const std::vector<unsigned>& indices,
    std::size_t index_offset,std::size_t count,std::size_t vertex_offset,
    render::Rect clip,render::Vec2 origin,render::Vec2 scale,render::Vec2 viewport);
class DebugUiSession {
    struct Impl;std::unique_ptr<Impl> impl_;
public:
    explicit DebugUiSession(const std::string& settings);
    ~DebugUiSession();
    void current();
    void set_scale(float scale);
    float scale()const;
    void begin(float width,float height,float delta);
    std::vector<Batch> finish(float width,float height);
    bool input(const Input& input);
    void reset_input();
    void save_settings();
    const render::Bitmap& font()const;
    bool capture_mouse()const;
    bool capture_keyboard()const;
    void clipboard(const char* (*get)(void*),void (*set)(void*,const char*),void* user);
};
}
