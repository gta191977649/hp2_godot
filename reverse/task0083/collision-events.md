# TASK-0083 collision event and host audio lifecycle

E-0083-C01. Debug NFSHP2.ELF SHA256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b;
version/build independently unknown. Read-only captures local/task0083;
captures.json preserves private response hashes. Not Verified.

## Confirmed event chain

Contact::CollisionImpulse001D1F58 obtains the witness Collision before checking
incoming velocity. GetCollision001D2FB8 lazily constructs00219230 with initial
contact, zero cumulative impulse and both packed timestamps set to WorldTimer.
AddImpulse00219400 sums individual impulse magnitudes, updates LastContactTime,
CurrentContactPoint and CurrentScrapeSpeed; PlayScrape executes during this call.
The sound update001E1468 later retries CreateSample until a non-null sample is
allocated; that collision then never recreates its impact sample. This preserves
scrape RNG before the later impact RNG. Collision::Update00219650 compares signed
packed elapsed*325F6C strictly against CollisionLifetime328B1C. World0014D93C ends
expired collisions after collision resolution. HasExpired001D30F0 does not discard
a witness with a non-null Collision even after its bounding boxes stop overlapping.
GetTerrainType00219698 selects greatest TerrainTypeInfo.CollisionPriority+2C;
equal priority selects body1. Native static contact order binds car32 then polygon.

## Bounded native and host implementation

Port-owned CollisionAudioContact retains this state per static polygon; no guest
Collision allocation/vtable or full world event list ABI is claimed. StaticCollisions
creates state in the contact impulse callback, records applied impulses, retains
witnesses while an event is alive and expires events after resolution. It starts
scraping during accumulation, updates existing scrape volume once per committed
frame and retains released samples for the original stepped fade/free threshold.
Impact selector, RNG, tire mixer, contact records and fading voices are copied
with candidate Memory; rejected frames publish none of those changes.

The own-car fixture binds its car position as listener and closest-player context.
It does not implement camera azimuth/doppler, other-player distances or full replay
sound services. Contact proximity in this bounded car/static path is local; no
claim covers arbitrary world objects or distant multi-body contacts. One committed
frame equals one sound-service update here; original cadence equivalence remains
unverified. Physical impulse integration remains unchanged.

Ps2DrivingRuntime turns original PCM and native volume/pitch into streams, buffers
committed impact requests for one-time consumption and publishes persistent scrape
IDs and ten tire slots. driving_audio.gd owns playback only. Native gain contains
scrape fading; the host does not create a replacement Tween. Pause silences all
voices; restart discards queued impacts and native mixer history. Releasing stream
references on teardown avoids retained playback resources. These are explicit
host lifecycle bindings, not original SPU emulation. Both existing driving hosts
share this transport. Sample source remains the configured original asset tree.

## Validation and limits

Native controlled wall integration requires delayed impact publication and rejects
repeated restart of the same contact; rejected material contact preserves audio
state/RNG. Event tests cover accumulation, scrape-before-impact random state,
copy/retry, one impact per contact, stable scrape ownership, fade and expiry.
Selector/bank and tire tests separately cover tuning gates and original PCM.
Host tests combine480native driving frames for drift/handbrake with a port-side
queue fixture for impact consumption and persistent scrape start/gain/release.
The queue fixture does not pretend to be an original collision scenario; native
wall tests cover that producer. Headless Dummy audio tests playback state, not
speaker audibility or PS2 timing/numerical equivalence.

Remaining: multi-car/Smokeable collision ownership and resulting glass/foliage
routing; original spatial/voice-pool/SPU behavior; rolling noise, damaged-tire,
leaves/thumper/bottom-out/tire-impact branches. No omitted service is marked
recovered or Verified by these PC tests. Next original-runtime comparison should
capture equal collision/input/clock states and audio request/voice transitions.
