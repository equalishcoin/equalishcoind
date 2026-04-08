# EqualishCoin Handoff Plan

## Mission

Continue turning the current Peercoin-derived codebase into a clean, working EqualishCoin fork:
- Honest meme branding
- Hybrid PoW/PoS preserved
- Maintainable diff against upstream
- Working renamed binaries and operator surfaces
- Unique chain identity before any real launch

This is not starting from zero. A substantial first pass is already complete and partially verified.

---

## Current State Summary

### Already completed and committed

#### Commit 1
- `7da53f3c8`
- Message: `Fork bootstrap: rebrand as EqualishCoin and set network identity`

#### Commit 2
- `f9cf1e825`
- Message: `Consensus: activate softfork features at height 1`

### Already implemented

#### Branding / identity
- Project renamed to EqualishCoin in key metadata and docs
- Ticker/default unit changed to `EQLS`
- Config file default changed to `equalishcoin.conf`
- Datadir changed to `.equalishcoin`
- Message signing magic changed to `EqualishCoin Signed Message:\n`
- Client/version strings updated toward EqualishCoin
- Network magic / ports / prefixes / bech32 HRPs changed in chain params
- Main/test placeholders for DNS seeds added
- Fixed seeds disabled on main/test to avoid accidental Peercoin overlap

#### Consensus / params
- Main/test softfork-style activations moved to block 1
- Mainnet stake min age reduced to 14 days
- Mainnet coinbase maturity reduced to 240
- Genesis/checkpoints remain mostly inherited from Peercoin for now
- Main/test checkpoints reduced to genesis-only
- `chainTxData` zeroed where inherited Peercoin values were misleading

#### Docs already added/updated
- [README.md](README.md)
- [docs/chain-params.md](docs/chain-params.md)
- [docs/fork-notes.md](docs/fork-notes.md)
- [docs/launch-plan.md](docs/launch-plan.md)
- [share/examples/equalishcoin.conf](share/examples/equalishcoin.conf)

#### Build/runtime verification already done
- `autogen.sh` works
- `configure --without-gui --disable-tests` works
- Renamed targets `equalishcoind` and `equalishcoin-cli` build successfully
- Regtest smoke test already passed:
  - daemon start
  - wallet creation
  - address generation
  - mining 3 blocks
  - `getblockchaininfo`
  - `getbalances`

---

## Current Uncommitted Changes

At the time of handoff, the following work exists but has **not** been committed yet:

### Modified files
- [.gitignore](.gitignore)
- [src/Makefile.am](src/Makefile.am)
- [doc/man/Makefile.am](doc/man/Makefile.am)

### New files
- [contrib/Dockerfile.nogui](contrib/Dockerfile.nogui)
- [contrib/docker-compose.yml](contrib/docker-compose.yml)
- [contrib/docker-entrypoint.sh](contrib/docker-entrypoint.sh)
- [contrib/init/equalishcoind.conf](contrib/init/equalishcoind.conf)
- [contrib/init/equalishcoind.openrc](contrib/init/equalishcoind.openrc)
- [contrib/init/equalishcoind.service](contrib/init/equalishcoind.service)
- [doc/man/equalishcoind.1](doc/man/equalishcoind.1)
- [doc/man/equalishcoin-cli.1](doc/man/equalishcoin-cli.1)
- [doc/man/equalishcoin-qt.1](doc/man/equalishcoin-qt.1)
- [doc/man/equalishcoin-tx.1](doc/man/equalishcoin-tx.1)
- [doc/man/equalishcoin-util.1](doc/man/equalishcoin-util.1)
- [doc/man/equalishcoin-wallet.1](doc/man/equalishcoin-wallet.1)

These changes are important because they make the renamed binaries real in the build system and align Docker/init packaging.

---

## Highest Priority Next Actions

### 1. Commit the currently uncommitted rename/build/operator pass
Create a separate commit for:
- renamed build targets in [src/Makefile.am](src/Makefile.am)
- manpage manifest updates in [doc/man/Makefile.am](doc/man/Makefile.am)
- new EqualishCoin manpage files
- Docker files
- EqualishCoin init/service files
- `.gitignore` updates for built Equalish binaries

Suggested commit message:
- `Build: rename binaries and operator artifacts to EqualishCoin`

### 2. Verify the remaining renamed binaries
Build and smoke-check:
- `equalishcoin-wallet`
- `equalishcoin-tx`
- `equalishcoin-util`

Expected command pattern:
```bash
pushd /opt/equalishcoin >/dev/null
./autogen.sh
./configure --without-gui --disable-tests
make -j"$(nproc)" \
  src/equalishcoind \
  src/equalishcoin-cli \
  src/equalishcoin-wallet \
  src/equalishcoin-tx \
  src/equalishcoin-util
popd >/dev/null
```

### 3. Replace inherited genesis with unique EqualishCoin genesis
This is the most important remaining protocol identity gap.

Current issue:
- [src/kernel/chainparams.cpp](src/kernel/chainparams.cpp) still uses inherited Peercoin genesis values and asserts

What must be done:
- choose final EqualishCoin genesis message
- generate a unique genesis block for at least mainnet
- ideally generate unique testnet genesis too
- update:
  - genesis creation timestamp text
  - nonce/time if needed
  - asserted genesis hash
  - asserted merkle root
  - checkpoint height 0 hashes
  - any chainTxData / assumptions impacted

Suggested genesis message candidates:
- `EqualishCoin: perfectly balanced, probably.`
- `This seemed fair at the time.`

Recommendation:
- prefer `EqualishCoin: perfectly balanced, probably.` unless generation constraints suggest otherwise

Do **not** leave inherited Peercoin genesis in place if the goal is a serious independent public fork.

### 4. Finish user-facing rename sweep
There are still likely Peercoin remnants outside the critical daemon/CLI path.

Search and clean:
- Qt strings/resources
- init docs
- bash completions
- service names
- docs under `doc/`
- remaining user-agent/help/manpage text
- any `peercoin:` URI usage if GUI/payment handling is still enabled later

Likely places:
- [doc/init.md](doc/init.md)
- [contrib/init](contrib/init)
- [contrib/peercoin-cli.bash-completion](contrib/peercoin-cli.bash-completion)
- [contrib/peercoin-tx.bash-completion](contrib/peercoin-tx.bash-completion)
- [contrib/peercoind.bash-completion](contrib/peercoind.bash-completion)
- Qt sources/resources under `src/qt/`
- remaining docs under `doc/`

### 5. Decide and document final chain economics more explicitly
Current docs are serviceable but not fully explicit yet.

Need to confirm/document:
- reward schedule inherited from Peercoin
- whether supply is effectively uncapped / tail-emission-like
- practical explanation of why EqualishCoin is “50/50 in spirit” rather than protocol-perfect
- whether any further maturity/min-age tuning is desirable before launch

Update:
- [docs/chain-params.md](docs/chain-params.md)
- [docs/fork-notes.md](docs/fork-notes.md)
- [README.md](README.md)

---

## Secondary Tasks

### 6. Validate no network identity collisions remain
Check that EqualishCoin no longer collides with Peercoin on:
- magic bytes
- ports
- address prefixes
- bech32 HRPs
- default datadir
- default conf file
- binary names
- service names

This is partly done already, but should be revalidated after genesis replacement.

### 7. Review signet defaults
Current signet params still include upstream signet seed/domain behavior in [src/kernel/chainparams.cpp](src/kernel/chainparams.cpp).

Decide whether to:
- leave signet as mostly upstream-compatible dev functionality, or
- fully EqualishCoin-fork it as well

Document whichever choice is made.

### 8. Add or update tests where feasible
Good candidates:
- chain params identity assertions
- renamed startup/basic CLI expectations
- regtest basic smoke scripts if functional tests are available

Do not overbuild this if the framework is awkward; prioritize useful, cheap validation.

### 9. Improve mining/staking operator docs
Need explicit simple usage examples for:
- run node
- create wallet
- mine a few blocks
- enable staking / minting where applicable
- explain that mining/staking split is “equal-ish”

Potential additions:
- README usage section refinement
- maybe a short devnet/regtest quickstart doc

---

## Verified Commands Already Used Successfully

### Build
```bash
pushd /opt/equalishcoin >/dev/null
./autogen.sh
./configure --without-gui --disable-tests
make -j"$(nproc)" src/equalishcoind src/equalishcoin-cli
popd >/dev/null
```

### Regtest smoke test
```bash
pushd /opt/equalishcoin >/dev/null
rm -rf /tmp/eqls-regtest
mkdir -p /tmp/eqls-regtest

./src/equalishcoind -regtest -daemonwait -server -datadir=/tmp/eqls-regtest

./src/equalishcoin-cli -regtest -datadir=/tmp/eqls-regtest createwallet dev
ADDR=$(./src/equalishcoin-cli -regtest -datadir=/tmp/eqls-regtest getnewaddress)
./src/equalishcoin-cli -regtest -datadir=/tmp/eqls-regtest generatetoaddress 3 "$ADDR"
./src/equalishcoin-cli -regtest -datadir=/tmp/eqls-regtest getblockchaininfo
./src/equalishcoin-cli -regtest -datadir=/tmp/eqls-regtest getbalances
./src/equalishcoin-cli -regtest -datadir=/tmp/eqls-regtest stop

popd >/dev/null
```

Observed result:
- node started
- wallet created
- 3 blocks mined
- chain/balance queries successful
- immature balance showed `150.000000`

---

## Recommended Execution Order For Next Agent

### Phase 1: Stabilize current work
1. Review `git status`
2. Confirm renamed binary/operator changes are correct
3. Commit that pending work

### Phase 2: Complete binary/runtime validation
1. Build all renamed non-Qt binaries
2. Run CLI help/version checks
3. Re-run regtest smoke test
4. Optionally validate `equalishcoin-wallet` basic behavior

### Phase 3: Fix deep chain identity
1. Generate unique EqualishCoin genesis
2. Patch [src/kernel/chainparams.cpp](src/kernel/chainparams.cpp)
3. Rebuild
4. Re-run regtest smoke test
5. Update docs with final genesis message and hashes

### Phase 4: Rename hygiene sweep
1. Search for remaining `Peercoin`, `peercoin`, `PPC`, `ppcoin`
2. Fix user-facing/operator-facing leftovers first
3. Skip massive locale churn unless clearly needed
4. Keep changes reviewable and minimal

### Phase 5: Documentation and launch clarity
1. Finalize [README.md](README.md)
2. Refine [docs/chain-params.md](docs/chain-params.md)
3. Update [docs/fork-notes.md](docs/fork-notes.md)
4. Tighten [docs/launch-plan.md](docs/launch-plan.md)

---

## Specific Files Most Important To Inspect Next

### Consensus / chain identity
- [src/kernel/chainparams.cpp](src/kernel/chainparams.cpp)
- [src/chainparamsbase.cpp](src/chainparamsbase.cpp)
- [src/consensus/amount.h](src/consensus/amount.h)

### Build / output naming
- [configure.ac](configure.ac)
- [src/Makefile.am](src/Makefile.am)
- [doc/man/Makefile.am](doc/man/Makefile.am)
- [.gitignore](.gitignore)

### Runtime / defaults
- [src/util/system.cpp](src/util/system.cpp)
- [src/init.cpp](src/init.cpp)
- [src/clientversion.cpp](src/clientversion.cpp)
- [src/util/message.cpp](src/util/message.cpp)

### Operator surfaces
- [contrib/docker-compose.yml](contrib/docker-compose.yml)
- [contrib/Dockerfile.nogui](contrib/Dockerfile.nogui)
- [contrib/docker-entrypoint.sh](contrib/docker-entrypoint.sh)
- [contrib/init/equalishcoind.service](contrib/init/equalishcoind.service)
- [contrib/init/equalishcoind.openrc](contrib/init/equalishcoind.openrc)
- [contrib/init/equalishcoind.conf](contrib/init/equalishcoind.conf)

### Docs
- [README.md](README.md)
- [docs/chain-params.md](docs/chain-params.md)
- [docs/fork-notes.md](docs/fork-notes.md)
- [docs/launch-plan.md](docs/launch-plan.md)
- [share/examples/equalishcoin.conf](share/examples/equalishcoin.conf)

---

## Important Constraints / Guidance For Next Agent

- Preserve Peercoin hybrid PoW/PoS model unless there is a strong reason to change consensus behavior
- Prefer stable boring values over clever symmetry gimmicks
- Do not add any premine, hidden allocation, founder tax, admin key, or central control path
- Keep the joke dry and honest, not loud
- Keep diffs focused and reviewable
- Do not revert prior committed work unless clearly broken
- Be especially careful around genesis, checkpoints, and network identity changes
- The current code already runs on regtest with renamed daemon/CLI, so avoid destabilizing that path

---

## Definition Of “Good Next Stopping Point”

The next agent should aim to stop only after:
- pending rename/operator changes are committed
- all renamed non-Qt binaries build
- regtest smoke test still passes
- unique EqualishCoin genesis is implemented and documented
- remaining major Peercoin user-facing naming leaks are reduced materially
- final report clearly distinguishes verified functionality from remaining gaps
