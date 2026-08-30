# 0089: Native guild-share auction buyout

Migrated guild-share mailbox access to MasterPlayer and routed auction purchases through WorldSession::HandleAuctionPlaceBid(), preserving the native player transaction path for money, DB, auction removal and mail.

Ownership: **MOD**.
