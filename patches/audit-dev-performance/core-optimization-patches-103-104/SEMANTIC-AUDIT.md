# Semantic audit notes 103-104

### 103 - Chat packet message views
`BuildChatPacket()` accepts `std::string_view` for message data and avoids the unconditional internal message copy when no GM color decoration is needed. All call sites were audited for lifetime/null requirements. The message-payload transformation was previously compared old-vs-new over 200,000 randomized messages and was byte-identical.

### 104 - Non-mutating WorldText line split
Replaces `strtok()`-based splitting with `std::string_view` slicing. This removes mutation of source text (including the historical `const_cast`-style path through string-store data) and avoids temporary line strings while preserving the old behavior of skipping empty delimiter runs. The split transformation was previously compared over 250,000 randomized cases.
