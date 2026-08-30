# Follow-up 0084: PCH / compatibility shim

0084 is intentionally excluded from bundle 0066-0083.

During an additional `botpch.h` probe outside the official `USE_PCH=OFF` configuration, older compatibility-shim structure problems were exposed, including an unmatched preprocessor terminator and invalid `WorldSession::WorldSessionState` aliases. A small LFG-shim cleanup was started but not frozen because it must be evaluated together with those pre-existing PCH/header problems.

The uncommitted 0084 work was saved separately before the 0066-0083 freeze. It is not required for the validated `USE_PCH=OFF` source state in this bundle.
