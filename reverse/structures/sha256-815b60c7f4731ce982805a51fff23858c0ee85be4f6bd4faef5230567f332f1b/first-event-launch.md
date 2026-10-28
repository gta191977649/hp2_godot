# First-event launch logical storage and asset adaptation

TASK-0098 E-0098-RP01..RP05 / TASK-0099 E-0099-W01/W02. Debug NFSHP2.ELF SHA-256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b; revision unknown. Ghidra data_types/inspect/listings in the task manifests are authoritative. Host decoded structs and service classes do NOT claim a PS2 C++ ABI.

| Original structure | Size hex | Relevant original offsets hex |
|---|---:|---|
|RaceParameters|310|track0,traffic10,opponentselect38,playerstart3C,objective50/54,mode254,event258,coplimit25C,police260..288,players28C/2BC,display2EC,driverptr304,count308,colorseed30C|
|DriverInfo|18|elim0,number1,character2,skill3,start4,padding5..7,type8,RideInfoC,characterptr14|
|cPlayerSettings|30|HUD0,rumble1,handling2,transmission3,POV4,alternate5,split6,gender7,spring8,lookbackC,features10,name14[12bytes],total20,lap24,hotkey28,scoring2C|
|RaceEvent|1E4|tree0,state4,old8[16bytes],prev18[16bytes],name28,reward48,number68,type6A,counts6E..76,bonus78,objective7C,cars84,tracksD4,rewardsF4,opponents134,results150,unlockmap1B0|
|EventTimeState|4|Time.PackedTime0|
|EventResult|C|rank0,time4,reason8|
|World|1730 allocation|CarList0,AIList8,collisions10,total1C,AIpos20,coppos24,trafficpos28,CarTable2C[14],RNG178,pause17C,waste180,scale184,timer188,callback18C,param190,teleporter194|
|Car accessed header|not allocation extent|CarIndex8,DriverNumberA,MyDriver14; these fixture fields are not a Car constructor|
|DriveTarget|210|NumPoints0,WhichTime1F8,AccelerationTime1FC,Length200,lastindex204(signedbyte)|
|TrafficTeleporter|58|TimeUntilTeleport0,LastCar4,ForwardTotals8[10],BackwardTotals30[10]|

Global RaceParameters00311F78 and DriverInfoTable00312288 are Debug-only. Never alias Retail object pointers/profile offsets into them. Persistent state/parsed tree/catalog must outlive RaceEvent views and resource callbacks. Actual profile/settings/record providers remain required; no default zero scores are declared original saved records.

Retail asset TrackInfo rowA4 vs DebugA0 is a reviewed field projection only. Original lookup is one-based row(number-1), Name-empty rejection and cleared track54. Reviewed Retail fields include TrackNumber41,Point2Point43,CopCharacterNameHash54; other row tails are not imported as Debug. Character chunk34300 uses44hex rows with name4/body24; lookup by index differs from CharacterNumber identity. Skin chunk34601 uses40hex rows; GetSkinInfo default is first row. CarTypeInfo compatible accessed geometry40 and five LocationProb values520 were checked against original Debug type/consumers. Native parsers reject truncated/unknown source extents.

World/registering drivers and lightweight header fixtures do not instantiate actual Car/controller/World/Race objects or certify gameplay. See reverse/task0098/first-event-launch.md and reverse/task0099/shared-world.md for precise scopes, source addresses and remaining required dependencies. No Verified claim.
