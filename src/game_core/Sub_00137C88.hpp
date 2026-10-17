#pragma once
// TASK-0038, bounded native projection; original source path and parameter names unknown.
namespace port::driving {
// ELF 00137C88; E-0038-003; SLUS_203.62 revision unknown;
// SHA256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Explicit arguments replace f12..f17, a0 and *a1. Ordinary finite float scope.
float Sub_00137C88(float, float, float, float, float, float, int, float&);
}
