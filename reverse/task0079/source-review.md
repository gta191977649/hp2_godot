# TASK-0079 source preservation successor

E-0079-S01 permits exactly two identity replacements: the historical throwing
CarState::BeginTimestep template becomes the Debug001BD938 four-terrain reset;
NativeBackend::advance invokes it once before original movement and collision
retry processing. See [driving evidence](driving-chain.md). This changes recovered
behavior intentionally and does not claim preservation or original equivalence
for those two bodies. No other function slot, signature or stub changes are
allowed. Debug SHA-256 is
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b;
revision/build date remains unknown.

The immutable TASK0076 review/scope and TASK0077 review derive the prior source
state. TASK0077 accepts explicit historical identity/auxiliary snapshots; only its
default live call delegates to TASK0079. Its historical pins and checks remain
active. The inherited auxiliary pins remain unchanged except the explicitly
reviewed TASK0077 dispatcher. The new audit/test/review note and current CarState
header/backend test file receive explicit whole-file pins. Source identities
continue excluding tests as before; their whole-file pin protects this test change.

The successor rejects unrelated identity mutations, scope widening, changes to
predecessor pins and unexpected stub transitions. No historical JSON is rewritten.
