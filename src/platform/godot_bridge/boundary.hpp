#pragma once
#include <exception>
#include <string>
#include <utility>

// TASK-0020 follow-up: the GDExtension ABI boundary.
//
// godot-cpp is built with -fno-exceptions, so an exception that reaches Godot's own frames is
// undefined behaviour -- it does not produce a catchable error, it produces a crash with no
// diagnostic. CMakeLists already states the rule ("port adapters catch recovered NotImplemented
// locally"); this makes it systematic rather than something each method has to remember.
//
// `hp2::guard` is not enough on its own: it catches hp2::NotImplemented, which is the *expected*
// outcome we want to report. Anything else -- a std::bad_alloc, a parse error from an original
// asset, a std::out_of_range -- would still cross. Every bound method runs its body through
// `catch_all`, so a bug in the port becomes a reported error instead of a silent crash of the
// editor.
//
// gta-reversed solves the same problem one layer lower, with a single process-wide
// SetUnhandledExceptionFilter that logs the exception code, address and registers (its
// source/app/app_debug.cpp). It can do that because it is a DLL inside the original game and
// terminating is acceptable there; we are the whole program and a host that can still report the
// failure is strictly more useful, so the net sits at the boundary where context still exists.
namespace port::godot_bridge {

// Runs `body` and returns an empty string, or a description of whatever escaped it.
template <class Body>
std::string catch_all(Body&& body) {
    try {
        std::forward<Body>(body)();
    } catch (const std::exception& error) {
        return error.what();
    } catch (...) {
        return "unknown exception crossed the GDExtension boundary";
    }
    return {};
}

} // namespace port::godot_bridge
