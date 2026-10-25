# TASK-0095 - NameEntry keyboard

Retail SLUS_203.62 US PS2, revision unknown, SHA-256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
Read-only MCP captures in ignored local/task0091/name-agent. Not Verified.

## Confirmed - E-0095-N01

0024D408 constructs an inactive editor and0024D2F0 snapshots12 bytes per name
plus flags. Player configurations are profile10D7C/1144C, names+14, flags+7.
00183830 initializes names from localized27165/27166;00187DE8 copies at most11.
0024D5C0 activates item1/player0/pad0 or item3/player1/pad1.0024C708 clears8 bytes,
copies7, resets case and name cursor while retaining grid row/column.

0024D088 maps messages6/7 to wrapping columns0..13,8/9 to rows0..2; Square10
toggles lower/upper;15/16 moves name cursor clamped0..6. These require matching
pad. Back17 has no pad gate, commits and closes (sound29).0024CC00 selects the
authored lowercase/uppercase character table at0031D750/780. '(' deletes at the
current cursor, shifts left, pads position6 with space, then moves cursor left;
')' commits; all other characters overwrite and advance. Sounds25/26/27/28/29
are enter/delete/case/grid/close.0024D660 toggles each gender flag only on its
matching pad. Outer Back compares strcmp names and flags before optional Autosave.

## Confirmed - E-0095-N02

0024C5D0 shows keyboard class670B6FDD, selected player letter class7B95E55C or
7DEB0DFD, caretC01086B5; hides corresponding normal name71450A81/71572302.
0024CF50 fills CHAR_%X%X.0024CD40 fills P1/P2+C%d seven slots and uses selected
RGBA128/64/0/128 versus128/128/128/128. Caret absoluteX is selected letterX;
Y is letterY+height+2.001E4638 translates the whole keyboard class relative to
panel35B2F256, restoring authored coordinates when hidden.

## Confirmed - E-0095-N03

Grid paths are independent scalar001DAEC8 calls via001DB000; duration0.1,
retarget preserves current velocity. Wrapping snaps both current axes and clears
states. Panel starts650, targets authoredX with0.5-second path. During entry,
grid is snapped to moving target coordinates.0024CA60 centers highlight using
height/2 for BOTH axes (00144760/001446E0). Caret pulse0..128,speed10,wrap only
when phase>floatbits40C90E56; alpha uses original bSin polynomial0025A5E0.

## Implementation, validation and unresolved

Portable state/actions and production Options/NameEntry routes are connected.
Native frontend_name_entry test passed1/1,1.74s: wrong-pad rejection, wrap, delete,
case, cursor clamp, Back/Done commit, grid preservation, live profile and widgets.
Godot extension builds. Window captures expose an unresolved equal-layer draw
ordering dependency: settled keyboard panels cover glyphs. Do not mark visual
acceptance complete. TASK-0039-Q07 already identified equal-layer construction
order as unknown; read-only investigation continues. Shared Help overlay and
host second-controller input adapter are also open dependencies. Existing legacy
Username0016DBD0 is intentionally separate and still untranslated.

## Confirmed - E-0095-N04: final render phases and equal-layer order

0013FBA0 walks chunks ascending, appends widget nodes at0013FCA4..0013FCB8.
001402A0 postload preserves order;00140E98 only stamps dirty5.00146600 compares
signed first.layer<=second.layer;00146510/00250550 stable sorts next links.
00145F30 traverses prev links, preparing reverse-authored equal-layer packets.
This alone does NOT establish final visual ordering:00145D38 appends to layer
buckets,00145E10 consumes bucket next links.00145AF0 initializes exactly29
sentinels.00112858 submits bucket ranges24..20 (00112F78),15..0 (00113B58),
19..16 (00113B98),28..25 (00113BE0), in that order. Viewport pair gates apply,
not frontend/gameflow state gates. Keyboard16..19 overlays ordinary frontend.

Counterevidence: initial numeric-descending submission, even with corrected equal
order, kept keyboard behind GRID_SHADOW layer8 alpha255. That inference is retired.
Final phase ordering and reverse-authored ties are now implemented with deterministic
frame tests. Unsupported visible bucket values explicitly throw at00145D38.
Original captures0013fc68.listing.json,00145af0.listing.json,00112f40.listing.json,
00113ac0.listing.json,00113b20.listing.json remain ignored in name-agent.

Godot Vulkan capture name-entry-phases-fixed.png now visibly contains complete
lowercase keyboard, selected character, delete/Done quads, player slots and caret.
Window capture exits0 without errors. Native keyboard/frame tests2/2 pass2.12s.
Original-runtime comparison and common Help overlay remain open. No Verified claim.
