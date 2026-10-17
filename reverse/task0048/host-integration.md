# E-0048-H01/H02 - Track host integration

The port-side TrackViewer uses immutable native scene data, a CPU worker and bounded GPU upload slices. Geometry, original texture mip pixels and GS MODULATE/copy/source-over/additive metadata are shared by the viewer and driving presentation. Host derivative-based LOD/raster sampling is explicitly not PS2 GS numerical equivalence. World-lit strips use the recovered world-identity context and original material blocks; facade angles use the separately SHA-bound Debug math lookup adapter. The omitted environment reflection pass is reported in Recovery details.

E-0048-H02 publishes the existing native Wheel ground point at +0x130 and terrain plane at +0x140, alongside stored terrain/grip fields. These offsets are directly assigned by the already recovered Wheel::SetState and FindDrivingSurface in indep/src/wheel.cpp. The host reports native-axis normal components and elevation from those stored values, not a new ray cast, geometry sample, material guess, or replacement physics computation.

Opaque depth-writing repeated solids can share a host MultiMesh. Transparent/deferred/depth-dependent draws and facades retain individual ordering. This is a host rendering optimization, not a recovered world visibility or scenery LOD algorithm. Missing original models remain absent and are named in diagnostics; no substitute mesh is installed.

Loaded asset bytes, exact original mip images, ELF images and screenshots remain outside Git. No Ghidra database mutation is used. The completed TASK-0048 file records reproducible validation and remaining limits.
