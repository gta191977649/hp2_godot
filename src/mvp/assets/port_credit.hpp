#pragma once

#include "indep/inc/Flags.hpp"

namespace hp2 {
inline constexpr char portCreditTitle[] = "The Porting of HP2";
// EA Credits 501000 and 501002 use the same gold for both Impact faces.
#define HP2_CREDIT_GOLD "$RGB(134,96,48)"
#define HP2_CREDIT_TITLE HP2_CREDIT_GOLD "$FONT(FONT_IMPACTBIG)"
#define HP2_CREDIT_HEADING HP2_CREDIT_GOLD "$FONT(FONT_IMPACTNORMAL)"
inline constexpr FrontendCreditsLine portCreditLines[]{
    {HP2_CREDIT_TITLE "The Porting of HP2"},
    {""},
    {HP2_CREDIT_HEADING "Developer"},
    {"Nurupo"},
    {"episodes27@gmail.com", 0.8f},
    {""},
    {HP2_CREDIT_HEADING "Third-Party Libraries"},
    {"Godot Engine and godot-cpp - MIT"},
    {"FFmpeg libraries"},
    {"libavcodec, libavformat, libavutil, libswresample"},
    {"MinGW-w64 - ZPL 2.1 and file-specific terms"},
    {"winpthreads - MIT and derived-code notices"},
    {"GCC runtimes - GPL 3.0 or later"},
    {"with the GCC Runtime Library Exception"},
    {"Dear ImGui, ImGui Console and csys - MIT"},
    {"stb headers in Dear ImGui - MIT"},
    {HP2_CREDIT_HEADING "Special Thanks to Project:"},
    {"Gabriel Gauci Maistre - nfshp2-recomp", 0.8f},
    {"Romalu86 - NFSHP2_Engine", 0.8f},
    {""},
    {"and you for playing this game!", 0.8f},
};
#undef HP2_CREDIT_HEADING
#undef HP2_CREDIT_TITLE
#undef HP2_CREDIT_GOLD
inline constexpr FrontendCreditsContent portCredit{
    portCreditTitle, portCreditLines, sizeof(portCreditLines) / sizeof(*portCreditLines)};
}
