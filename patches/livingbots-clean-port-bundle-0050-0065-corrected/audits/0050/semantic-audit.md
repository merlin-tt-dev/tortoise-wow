# Semantic audit — 0050 split ownership

0050 is intentionally split into two independently visible patch files:

1. `patches/core/0050-core-native-login-query-holder-extract.patch`
2. `patches/mod/0050-mod-native-playerbot-login-query-holder.patch`

The core patch must be applied first. The two files together are byte-for-byte equivalent to the former combined 0050 result, but core and Playerbot ownership are now explicit.
