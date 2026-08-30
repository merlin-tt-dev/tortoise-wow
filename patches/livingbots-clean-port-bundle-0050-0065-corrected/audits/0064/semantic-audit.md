# Semantic audit — 0064

## Native security LFG and whisper APIs

- Uses global native `sLFGMgr` instead of the removed World queue accessor.
- Rebuilds the rejection whisper with the native chat packet builder and sends it to the target session, preserving `CHAT_MSG_WHISPER` and `LANG_UNIVERSAL` semantics.

## Scope

Only the files listed in `changed-files.txt` are part of this patch. The patch is ordered after the preceding numbered patch in `APPLY_ORDER.txt`.
