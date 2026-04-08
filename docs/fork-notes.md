# EqualishCoin Fork Notes

This file tracks major changes from Peercoin in this fork pass.

## Scope philosophy

- Prefer minimal, understandable changes
- Keep diff against upstream manageable
- Preserve battle-tested consensus behavior

## Major implemented changes

- Rebranded project/package identity to `EqualishCoin`
- Renamed binaries via build config:
  - `equalishcoind`
  - `equalishcoin-cli`
  - `equalishcoin-tx`
  - `equalishcoin-wallet`
  - `equalishcoin-util`
  - `equalishcoin-chainstate`
- Updated default config file name to `equalishcoin.conf`
- Updated default data directory paths to `EqualishCoin` naming
- Updated PID file default to `equalishcoind.pid`
- Updated signed-message domain separator to `EqualishCoin Signed Message:\n`
- Updated currency unit display constant from `PPC` to `EQLS`
- Assigned new network magics and ports for main/test/regtest
- Assigned new bech32 HRPs
- Replaced live seed domains with explicit placeholder seed domains
- Cleared inherited fixed seed lists for main/test
- Replaced inherited mainnet/testnet genesis with unique EqualishCoin genesis values
- Reduced checkpoint sets to genesis-only for clean new-network startup
- Reset inherited chain tx stats to zeros where historical values were not fork-appropriate
- Adjusted stake min age and maturity for hobbyist accessibility

## Not changed in this pass

- Core PoW/PoS consensus engine design
- Reward logic structure inherited from Peercoin
- Deep GUI localization/translation-wide string replacement
- Signet and regtest genesis values (currently inherited)

## Security posture

- No backdoors, privileged signing paths, or centralized controls introduced
- No presale/founder-tax logic introduced
- Conservative defaults preserved where possible
