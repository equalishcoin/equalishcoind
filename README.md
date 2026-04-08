# EqualishCoin (EQLS)

Fair... give or take.

EqualishCoin is a self-aware hybrid Proof-of-Work + Proof-of-Stake chain forked from Peercoin.
It is intentionally meme-aware but technically serious:

- Not a scam
- Not a pump-and-dump narrative
- Not pretending to be perfect or revolutionary
- Built to be understandable, runnable, and maintainable

The protocol identity target is roughly 50/50 between mining and staking in spirit. In practice, participation is expected to be "equal-ish" instead of mathematically exact.

## What this repo contains

- Full node daemon: `equalishcoind`
- CLI client: `equalishcoin-cli`
- Wallet tool: `equalishcoin-wallet`
- Hybrid PoW/PoS consensus inherited from Peercoin with conservative fork changes

## Build (Unix)

```bash
./autogen.sh
./configure
make -j"$(nproc)"
```

Main binaries are produced under `src/`.

## Run a node

```bash
./src/equalishcoind -daemon
./src/equalishcoin-cli getblockchaininfo
```

Default data dir:

- Linux: `~/.equalishcoin`
- macOS: `~/Library/Application Support/EqualishCoin`
- Windows: `%APPDATA%\EqualishCoin`

## Minimal regtest workflow

```bash
./src/equalishcoind -regtest -daemon -fallbackfee=0.001
./src/equalishcoin-cli -regtest createwallet dev
ADDR=$(./src/equalishcoin-cli -regtest getnewaddress)
./src/equalishcoin-cli -regtest generatetoaddress 5 "$ADDR"
./src/equalishcoin-cli -regtest getbalance
```

## Mining and staking notes

- CPU mining fallback is available via RPC mining calls (`generatetoaddress`) and `-generate` CLI behavior where supported.
- Staking is inherited from Peercoin wallet minting behavior.
- Use a local config (see `share/examples/equalishcoin.conf`) and ensure wallet is unlocked for minting in controlled environments.

## Chain identity summary

See:

- `docs/chain-params.md`
- `docs/fork-notes.md`
- `docs/launch-plan.md`

## Philosophy

EqualishCoin is "consensus, but approximate": practical engineering first, honest tone second, hype last.


