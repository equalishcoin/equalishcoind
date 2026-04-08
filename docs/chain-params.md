# EqualishCoin Chain Parameters

This document describes the current EqualishCoin network identity and launch constants.

## Narrative goal

EqualishCoin targets a practical hybrid identity between PoW and PoS: 50/50 in spirit, equal-ish in practice.

## Consensus model

- Base model: inherited Peercoin hybrid PoW/PoS
- Approach: preserve stable behavior unless safety/operability requires a change
- No presale, founder tax, hidden allocation, or privileged mint path

## Network identity

### Mainnet

- Network magic (`pchMessageStart`): `f7 c1 ee 50`
- Default P2P port: `19901`
- Default RPC port: `19902`
- Onion target port: `19903`
- Bech32 HRP: `eqls`
- Base58 pubkey prefix: `33`
- Base58 script prefix: `28`
- Base58 secret prefix: `161`
- DNS seeds: `seed1.equalishcoin.invalid`, `seed2.equalishcoin.invalid` (placeholders)

### Testnet

- Network magic: `ce fa 51 10`
- Default P2P port: `29901`
- Default RPC port: `29902`
- Onion target port: `29903`
- Bech32 HRP: `teqls`
- Base58 pubkey prefix: `65`
- Base58 script prefix: `196`
- Base58 secret prefix: `239`
- DNS seeds: `testseed1.equalishcoin.invalid`, `testseed2.equalishcoin.invalid` (placeholders)

### Signet

- Dynamic message start from signet challenge (existing behavior)
- Default P2P port: `39333`
- Default RPC port: `39332`
- Onion target port: `39334`
- Bech32 HRP: `seqls`

### Regtest

- Network magic: `fa ce 51 1a`
- Default P2P port: `39901`
- Default RPC port: `39902`
- Onion target port: `39903`
- Bech32 HRP: `reqls`
- Base58 pubkey prefix: `65`
- Base58 script prefix: `196`
- Base58 secret prefix: `239`

## Economic and timing parameters

Current values remain close to Peercoin for stability:

- Stake target spacing: `10 minutes`
- PoW target spacing: `10 minutes`
- Mainnet stake min age: `14 days` (reduced from 30 days for hobbyist accessibility)
- Stake max age: `90 days`
- Mainnet coinbase maturity: `240` blocks
- Testnet/regtest coinbase maturity: `60` blocks

## Checkpoints and chain tx data

- Mainnet and testnet checkpoints were reduced to genesis-only for clean fork bootstrapping
- `chainTxData` set to zeros where inherited historical peer values were not meaningful for a new fork

## Genesis status

- Current fork keeps inherited Peercoin genesis blocks for main/test/regtest to preserve immediate operability
- A dedicated genesis-regeneration pass is still recommended before public mainnet launch
- Suggested genesis message theme: `EqualishCoin: perfectly balanced, probably.`

## Supply behavior

- Supply behavior is inherited from Peercoin's hybrid minting logic in this phase
- No hard-coded special allocation logic has been added
- No insider-only rules are present
