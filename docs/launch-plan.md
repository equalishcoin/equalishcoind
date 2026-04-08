# EqualishCoin Launch Plan

## Ready now

- Chain identity constants (ports/magic/prefixes/HRPs) are forked
- Core daemon/CLI build identity is forked to EqualishCoin names
- Config/datadir defaults are forked
- Hybrid PoW/PoS model remains intact
- Regtest/dev workflow remains available through normal RPCs

## Needs completion before public mainnet launch

- Generate and lock a unique mainnet genesis block (and testnet if desired)
- Replace placeholder DNS seeds with operated seed infrastructure
- Regenerate manpages and update service unit artifacts under `contrib/init`
- Complete broad UI text and locale cleanup for final branding consistency
- Run full CI and platform matrix build validation
- Produce release signing and reproducible build process docs

## Suggested staged rollout

1. Internal devnet
2. Public testnet
3. Hardened seed + monitoring setup
4. Mainnet launch candidate
5. Mainnet launch

## Public launch checklist

- [ ] Reproducible builds pass
- [ ] Main binaries start and stop cleanly
- [ ] Wallet create/load/send/receive verified
- [ ] Mining RPC flow validated
- [ ] Staking flow validated in controlled test
- [ ] No network-identity collisions with Peercoin
- [ ] Seed nodes live and monitored
- [ ] Docs synced with final parameters
