# 0092: Remove dead compatibility stubs and ConfigAccess

Deleted empty LFG/World/Spell compatibility stub headers and the unused ConfigAccess abstraction. Active config readers already use Penqle Config::GetRootSections()/GetKeys().

Ownership: **MOD**.
