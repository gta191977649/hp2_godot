# TASK-0082 horn input and activation evidence

E-0082-H01. Debug NFSHP2.ELF SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b;
version/build independently unknown. Read-only captures: local/task0082/horn.
Recovered bounded implementation; not Verified against original runtime.

JoyHandler_HonkHorn 0013BF58 starts/stops the horn. Ordinary vehicles also set
Car.HeadlightsState830 to1 while held and0 on release. Police UsageType1 instead
toggles CopLights808 (0/2) and siren834 (0/1); BeACop3121EC manager operations
remain explicit NotImplemented. Non-null route editor3281D4 remains unsupported
at001C2088; the null editor branch of CancelTuningDrivingLines001C2F48 is empty.
RealDriver::StartHonkHorn0013C220 / StopHonkHorn0013C260 call the sound
object only for non-police usage and a non-null Car.pBaseCarSound850.
BaseCarSound Start0018B788, Stop0018B7C8 and IsHonking0018B808 require both
SoundEnabled326F40 and CarSoundEnabled327980. HornSound pointer is sound+34;
Sound3d::Activate002039C0 owns activated word0 and original Play/Stop calls.
HornSound Play0018D168 / Stop0018D190 perform original sample voice ownership;
Update0018D220 applies volume and DetunePitch*pitch-1. Sample selection0018BDA8
is Sound.Prefix + Horn in Sound.BaseName (see engine.md).

The native host fixture allocates an explicit minimal sound/activation proxy,
not an original constructor or emulated SPU voice. The host consumes this
activation using the original decoded sample, loop flags and gain/pitch tables.
H is a port-side keyboard binding. The playerless host fixture binds the human
input path; it does not run the AI scheduler UpdateHeadlightsAndHorn (which
returns early for a real non-null Player). No fake Player pointer is installed.

Native regressions cover gates, press/release, police state, unsupported manager,
and failed-frame rollback/retry. Host audio uses thirteen AudioStreamPlayers;
pause stops them, resume restarts requested streams, restart resets mixer state.
This is host lifecycle policy, not a claim about original sample phase during pause.
