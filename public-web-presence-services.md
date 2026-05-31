# Public Web Presence and Services Inventory

This note tracks public-facing web presence/services referenced by the repository and their migration status to EqualishCoin-owned endpoints.

## Runtime / User-Facing Endpoints (Highest Priority)

| Surface | Current endpoint | File references | Migration target | Status | Notes |
|---|---|---|---|---|---|
| Website (GUI) | https://equalishcoin.com | src/qt/bitcoingui.cpp | Keep | Done | Already migrated to `.com`. |
| Foundation/Donate (GUI) | https://www.equalishcoin.com/foundation | src/qt/bitcoingui.cpp | Keep | Done | Confirm `/foundation` route exists. |
| Forum action (GUI) | https://equalishcoin.com | src/qt/bitcoingui.cpp | Optional forum URL | Done (temporary) | Using main site until a dedicated forum URL is finalized. |
| Discord (GUI) | https://discord.gg/XPxfwtG | src/qt/bitcoingui.cpp | Keep or rotate invite | Active | Consider rotating to a non-expiring invite. |
| Release check feed | https://mirror.equalishcoin.com/latest_release.json | src/qt/clientmodel.cpp | Keep | Done | Migrated from `http://mirror.peercoin.net/latest_release.json`. |

## Project Hosting / Support Endpoints

| Surface | Current endpoint | File references | Migration target | Status | Notes |
|---|---|---|---|---|---|
| Source repository | https://github.com/equalishcoin/equalishcoin | multiple docs/scripts/manpages | Keep | Done | Looks consistent. |
| Issue tracker | https://github.com/equalishcoin/equalishcoin/issues | src/config/bitcoin-config.h, test/config.ini | Keep | Done | Looks consistent. |
| Detached signatures repo | https://github.com/equalishcoin/equalishcoin-detached-sigs | contrib/gitian-build.py, contrib/gitian-descriptors/* | Keep | Done | Looks consistent. |
| Gitian signatures repo | https://github.com/equalishcoin/gitian.sigs | contrib/gitian-build.py | Keep | Done | Verify ownership and permissions. |
| DNS seeder repo | https://github.com/equalishcoin/equalishcoin-seeder | doc/dnsseed-policy.md | Keep | Done | Verify repository exists and is maintained. |

## Translation / Community Services

| Surface | Current endpoint | File references | Migration target | Status | Notes |
|---|---|---|---|---|---|
| Transifex project links | https://www.transifex.com/bitcoin/bitcoin/ and related | doc/translation_process.md, doc/release-process.md, release notes | Decide EqualishCoin org/slug | Pending | Requires Transifex org/project setup decision. |
| Translator mailing list | https://groups.google.com/forum/#!forum/bitcoin-translators | doc/translation_process.md | Decide EqualishCoin channel | Pending | Could remain if still intentionally shared. |
| Developer mailing list refs | https://lists.linuxfoundation.org/mailman/listinfo/bitcoin-dev | doc/dnsseed-policy.md | Decide replacement | Pending | Likely historical/interop; keep or replace intentionally. |

## Legacy / Historical References to Review

- Qt locale source strings still contain `talk.peercoin.net` in multiple translation files under `src/qt/locale/`.
- Old release notes and historical docs include legacy third-party links (expected for archived content).
- Some comments and identifiers still contain `peercoin` for protocol/history reasons (not always web presence).

## Suggested Follow-up Sequence

1. Confirm `mirror.equalishcoin.com` serves `latest_release.json` over HTTPS in production.
2. Decide final forum URL policy (dedicated forum host vs main website route).
3. Decide whether to migrate or intentionally retain Transifex and mailing-list endpoints.
4. If desired, run a dedicated locale string refresh/regeneration pass after service URLs are finalized.
