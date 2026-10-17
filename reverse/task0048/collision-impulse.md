# E-0048-D04 - Debug static/car contact and impulse

Authority is Debug NFSHP2.ELF SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
Ghidra access was read-only. Raw decompilations/listing remain ignored under
`local/task0048/driving`; [driving-functions.json](driving-functions.json) records
response hashes and imported signatures. No original numerical comparison was made.

## Confirmed control and geometry

- World::DoTimestep0014D9A8 orders Drive, BeginMovement, DoTimestepMove,
  DoTimestepCollisions, FinishMovement. Collision processing follows the initial move.
- Original witness construction preserves carfirst/wafersecond (E-0048-T05).
  Contact::FindContactPointAndNormal001D1AE0 reads cached CP1/CP2, transforms each by
  the current body matrix/position, averages them and chooses the signed wafer normal.
  It does **not** rerun VClip after backup or displacement.
- FindWaferNormal001D15C0 uses face0. Listing001D1630 is `vsub.w`, clearing W only;
  the decompiler's apparent XYZ self-subtraction is counterevidence and is not used.
  If dot(normal,carPosition) <= planeDistance-WaferBias, it negates the normal.
  WaferBias is the original Debug3283A0 scalar. Carfirst then negates this wafer normal.
- Contact::FindContactTime001D1408 bisects backup time at most eight times. Disjoint
  samples reduce the upper bound; penetration increases the lower bound. It returns
  early when disjoint distance is below original ContactThreshold328398.
- A remaining penetration calls Contact::Crowbar001D1698. Its twenty iterations
  accumulate original325AA4, grow the increment below325AA8 by325AAC, then retry VClip.
  A successful query stores the displacement; exhausted attempts retain zero.
- Contact::Resolve001D18D8 performs backup, displacement, recompute, impulse, recompute,
  then movement for the backed time. The native binding requires an already-physics car
  and immovable static body, so the original mode-switch/static virtual branches are empty.
  Required services are checked before mutation.

The port ContactTime service stores actual car displacement: original Resolve subtracts
CrowbarVector for carfirst or adds it for carsecond. This sign normalization is an
explicit adapter operation, not a changed original crowbar algorithm. Both witness orders
are supported by the point/impulse bindings; default order is the actual carfirst path.

## Confirmed impulse branch

Contact::CollisionImpulse001D1F58 forms contact-point velocities using
GetContactPointVelocity001D1EA8 (linear + angular cross arm), then relative body2-body1
velocity. A separating normal component returns without applying momentum.
DenominatorTerm001D1ED8 computes the original angular contribution using inverse inertia,
arm cross normal, matrix multiplication, cross arm and dot normal.

The recovered static/car branch uses original coefficient getters: Car friction325B10,
bounce325B14, static friction325B18/bounce325B1C, mass-inverse cap325AFC and inertia
scale325B00. Static inverse mass, inertia and velocities are zero. Both original
ConsiderImpulse implementations return1, so this branch executes at most one iteration;
destructible/absorbing-body loop behavior is not exposed.

Normal impulse retains original MinimumContactImpulseFactor3283AC lower bound and
maximum bounciness. Tangential friction uses MaxFrictionScrapeSpeed3283B8 and the original
upright-car special branch (325AB8/325ABC, double thresholds2B0120/2B0128, friction325AC0).
The zero-scrape division is the actual EE `DIV.S` at001D2450; the existing finite/signed-zero
ISA projection supplies the hardware result rather than manufacturing a host NaN.
High-speed scrape adjustment retains original velocity constants325AC4/325AC8 and
order-specific arm factor325ACC/325AD0. No additional friction model is introduced.

CarCollisionBody::ApplyImpulse001D4F50 scales impulse by its mass-inverse cap325B04 and
dispatches to RigidBody::ApplyImpulseWithScaledInverseInertiaTensor0019A920 with325B08.
That leaf caps linear impulse at1000, angular impulse at10, then applies the original
angular scale to momentum. Calling the older unscaled ApplyImpulse would be incorrect.

The fixture implements exact null branches of AddDamage001D5008 and
AnimateCollisionReaction001D5188: pCarDamage844, pPlayer18, pAnimatedDriver4B4 and
pCopManager328C1C must be absent. Non-null dependencies report the original missing
entry instead of dropping effects. Contact impulse output is returned as a port observer;
the complete original collision event/audio/damage consumers are outside this binding.

## Placement and certification

The exact inline header marker `../indep/inc/Contact.hpp` occurs in Debug
CollisionBodyWitness::TestForContact001D3230 before ContactSlotPool allocation
(private raw `local/task0048/debug-001d3230.json`); Contact host bindings are
declared there. This does not claim the original implementation was inline: its original
translation unit is unresolved. CarCollisionBody and RigidBody methods remain in their
existing corresponding class files. Port callback/value storage asserts no original ABI.

Native unit scenarios cover point velocity, angular denominator, signed wafer normal,
cached-point average, normal impact, receding contact, zero-scrape EE arithmetic and
binary-search/crowbar dispatch. Runtime shape selection, swept candidate collection,
contact ordering and callback integration are separately recorded in the collision
geometry/static-runtime evidence. All results are port acceptance, not Verified.
