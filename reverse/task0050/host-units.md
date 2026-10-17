# E-0050-H01 - Host-unit and projection audit

Actual Windows Compatibility renderer, Elise onTrack11, recorded in ignored
local/task0050/runtime-units.log (prior projection) and
runtime-units-projection-fixed.log (corrected projection). The local body bounds are
host(-0.886459,-0.160586,-1.944788), size(1.772919,1.14403,3.838248), including attachments.
Both car and track global basis scales are(1,1,1) before/after driving publication.
The fixed camera pose is unchanged; only horizontal projection policy changes.

The ordinary curve-sign audit independently compared transformed source vertices to
stored signed-integer original SceneryInstance AABBs in every one of30 decompressed
TRACKB resources. All1388 inspected instances fit their original scene bounds, with
zero outward overflow. This bounded family audit is not a proof for every object type,
or exact screenshot object identification. Probe source/output remain ignored at
local/task0050/all_track_units.cpp and all-track-units.log; transform/format authority
and supporting addresses are retained in render-scale.md. No empirical dimensions used.

Projection correction uses Debug OUTSIDE bAngle0x32DC and original horizontal-axis
semantics from E0050U02. GDScript saves/restores Camera3D.keep_aspect as well as fov.
Runtime output retains71.52099609375 degrees and unit basis scales. The actual fixed
Elise window was inspected; image is local/task0050/elise-track11-projection-fixed.png.
Original scene framing is not certified because follow position remains tool-owned.
