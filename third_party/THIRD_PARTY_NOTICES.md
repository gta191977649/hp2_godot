# Third-party notices for HP2 Godot

The staged Windows build uses the following components. The license texts are
in the adjacent `licenses/` directory in the staged project.

| Component | Use | License and source |
| --- | --- | --- |
| Godot Engine 4.7.2 | External game host | MIT. See `Godot-LICENSE.txt` and `Godot-COPYRIGHT.txt` for the engine and the third-party code and assets it includes. [Godot source](https://github.com/godotengine/godot/tree/4.7.2-stable). |
| godot-cpp | GDExtension bindings | MIT. See `godot-cpp-LICENSE.md`. [Pinned source](https://github.com/godotengine/godot-cpp/tree/e4b7c25e721ce3435a029087e3917a30aa73f06b). |
| FFmpeg | libavformat, libavcodec, libavutil and libswresample, as separate DLLs | The supplied BtbN SDK reports **LGPL 3.0 or later**. See `LGPL-3.0.txt`, `GPL-3.0.txt`, and the build details below. |
| GCC libgcc and libstdc++ | Statically included by the MinGW build | GPL 3.0 or later with the GCC Runtime Library Exception 3.1 for covered files. See `GPL-3.0.txt` and `GCC-RUNTIME-EXCEPTION.txt`. [GCC 13.2.0 source](https://github.com/gcc-mirror/gcc/tree/releases/gcc-13.2.0). |
| MinGW-w64 runtime | Windows compatibility code in the MinGW build | ZPL 2.1 and file-specific terms. See `MinGW-w64-COPYING.txt` and [MinGW-w64 11.0.1 source notices](https://github.com/mingw-w64/mingw-w64/tree/v11.0.1). |
| winpthreads | Thread support included by the MinGW build | MIT and the terms for derived portions in `winpthreads-COPYING.txt`. [Source](https://github.com/mingw-w64/mingw-w64/tree/v11.0.1/mingw-w64-libraries/winpthreads). |
| Dear ImGui 1.77 WIP, ImGui Console, csys | Debug overlay only | MIT. See `Dear-ImGui-LICENSE.txt`, `ImGui-Console-LICENSE.txt` and `csys-LICENSE.txt`. [Pinned source](https://github.com/rmxbalanque/imgui-console/tree/7840945b5a20ea9f4e83f98d1ad3d9379aede3ed). |
| stb_rect_pack, stb_textedit, stb_truetype | Dear ImGui internals, Debug only | MIT option of the bundled dual-licensed headers. See `stb-LICENSE.txt` and the headers in the pinned source above. |

XAudio2, COM and the Universal CRT are Windows system components; HP2 does not
bundle their DLLs. The projects under “Special Thanks” are credited separately
from the library notices.

## FFmpeg build and redistribution

The reference SDK is the [BtbN FFmpeg Builds](https://github.com/BtbN/FFmpeg-Builds)
Windows x64 8.1 `lgpl-shared` build, reporting
`n8.1.3-17-gc389840894-20261011`. The local archive SHA-256 is
`ec30bde63b8e36ebdc7ba5b37140a43a2d2afe884650269fadeb033a57098cfc`.
See the [matching FFmpeg source revision](https://github.com/FFmpeg/FFmpeg/tree/c3898408946f6cbfe8e244372c250366d5639d00)
and [FFmpeg license notes](https://ffmpeg.org/legal.html). The SDK's
`ffmpeg -L` and `ffmpeg -buildconf` report the license and build configuration
(recorded in `licenses/FFmpeg-BtbN-build.txt`):
this build enables `--enable-version3` and `--enable-shared`; it does not
enable `--enable-gpl` or `--enable-nonfree`.

For distribution, keep the LGPL and GPL texts and the library notices with the
DLLs. Preserve the ability to use compatible replacements for the LGPL DLLs
and to reverse engineer for debugging modifications to those libraries.
Provide the corresponding source for the actual DLLs under the applicable
terms, including build scripts, patches, and notices and source required by
components compiled into the SDK. The upstream FFmpeg revision alone does
not cover the SDK's bundled dependencies. A local Windows staging build
does not create that corresponding-source bundle. If `HP2_FFMPEG_SDK`
selects another SDK, verify its configuration, dependencies and licenses
and update this notice and the in-game credit accordingly.
