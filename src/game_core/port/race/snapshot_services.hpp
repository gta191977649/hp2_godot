#pragma once
// TASK-0099 E-0099-W74. Port resource/primitive operands for recovered snapshots.
// No guest field or recovered ABI is added. The numerical lane is already the
// explicit caller policy in PhysicsConstructionServices; snapshots never invent it.
namespace port::track {struct RouteData;}
namespace port::race {
struct PhysicsConstructionServices;
struct SnapshotServices {
 const ::port::track::RouteData* routes=nullptr;
 PhysicsConstructionServices* physics=nullptr;
};
}
