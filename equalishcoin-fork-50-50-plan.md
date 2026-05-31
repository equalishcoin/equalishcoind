# EqualishCoin Fork + 50/50 Issuance Execution Plan

Status: In Progress
Owner: TBD
Last updated: 2026-05-30

## Objective

Fork/rebrand the current codebase into EqualishCoin and deliver a working, consensus-enforced long-term 50/50 issuance split between PoW and PoS, with minimal architecture changes and strong test coverage.

## Working Rules

- Keep changes minimal and reviewable.
- One concern per commit whenever practical.
- Treat all reward split logic as consensus-critical.
- Add tests for every consensus rule change.
- Do not add unrelated features.

## Definition of Done

- [ ] Node starts with new chain identity and genesis values.
- [ ] Rebrand constants are complete for main/test/regtest.
- [ ] Consensus enforces issuance split rules for both PoW and PoS.
- [ ] Local hybrid network can produce valid PoW and PoS blocks.
- [ ] Split behavior is validated by deterministic tests.
- [ ] Mining RPC path (`getblocktemplate` / `submitblock`) remains operational.

---

## Phase 0: Baseline and Safety Net

Goal: Freeze baseline behavior and create a measurable starting point.

Tasks

- [ ] Build current tree from clean checkout.
- [ ] Run unit + functional test subsets relevant to mining/validation.
- [ ] Capture baseline issuance behavior (PoW mint vs PoS mint over controlled run).
- [ ] Record known assumptions and uncertainties.

Deliverables

- [ ] Baseline test log.
- [ ] Baseline issuance note (short markdown summary).

Validation Gate

- [ ] No regressions before fork/rebrand work begins.

Suggested commit

- [ ] `chore: baseline validation and issuance snapshot`

---

## Phase 1: Fork/Rebrand (Non-Consensus First)

Goal: Apply naming and identity changes while avoiding reward logic changes.

Primary files

- `src/kernel/chainparams.cpp`
- `src/kernel/chainparams.h`
- `src/chainparamsbase.cpp`
- `src/chainparamsseeds.h`
- `src/Makefile.am`

Tasks

- [ ] Rename chain/product identifiers to EqualishCoin.
- [ ] Update message start bytes for all networks.
- [ ] Update P2P/RPC/default ports for all networks.
- [ ] Update base58 address prefixes and bech32 HRP.
- [x] Replace seed nodes and fixed seeds placeholders.
- [x] Update binary/help strings where needed.

Deliverables

- [x] Rebranded binaries start without touching reward logic.

Validation Gate

- [ ] Main/test/regtest each start with expected chain label and network magic.

Suggested commits

- [ ] `refactor: rebrand names and binary labels`
- [ ] `refactor: update network identity constants`
- [ ] `refactor: update seed configuration`

---

## Phase 2: New Genesis Activation

Goal: Replace genesis definitions and activate new chain roots.

Primary files

- `src/kernel/chainparams.cpp`
- `src/bitcoin-util.cpp` (grind flow)

Tasks

- [ ] Set new genesis timestamp text and tx parameters.
- [ ] Choose initial nTime/nBits/nNonce candidates for main/test/regtest.
- [ ] Grind header nonces with `peercoin-util grind`.
- [ ] Update asserted genesis hash + merkle root constants.
- [ ] Verify startup assertions pass for all networks.

Deliverables

- [ ] Deterministic genesis constants checked into source.

Validation Gate

- [ ] Fresh datadir starts and syncs from block 0 on all configured networks.

Suggested commit

- [ ] `consensus: replace genesis blocks and hash assertions`

---

## Phase 3: Introduce 50/50 Issuance Budget Primitives

Goal: Add deterministic consensus primitives for split accounting.

Recommended model

- Separate PoW and PoS issuance budgets per epoch (50/50 each).

Primary files

- `src/consensus/params.h`
- `src/validation.cpp`

Tasks

- [ ] Define epoch length and issuance parameters in consensus params.
- [ ] Implement deterministic budget computation helpers.
- [ ] Define rounding behavior and carry-forward rules (if any).
- [ ] Ensure logic is chain-derived and reorg-safe.

Deliverables

- [ ] Helper functions with unit tests for boundaries.

Validation Gate

- [ ] Helper tests pass for epoch transitions and rounding edge cases.

Suggested commit

- [ ] `consensus: add epoch issuance budget primitives`

---

## Phase 4: Enforce Budget in PoW Path

Goal: Enforce PoW-side split constraints in block validation.

Primary files

- `src/validation.cpp`

Tasks

- [ ] Wire PoW coinbase reward checks to PoW epoch budget constraints.
- [ ] Preserve existing fee handling semantics unless explicitly changed.
- [ ] Add rejection reasons that are deterministic and testable.

Deliverables

- [ ] Over-budget PoW blocks rejected by consensus.

Validation Gate

- [ ] New tests pass for valid/invalid PoW rewards around epoch edges.

Suggested commit

- [ ] `consensus: enforce PoW issuance budget in block validation`

---

## Phase 5: Enforce Budget in PoS Path

Goal: Enforce PoS-side split constraints in transaction validation.

Primary files

- `src/consensus/tx_verify.cpp`
- `src/validation.cpp` (if helper plumbing required)

Tasks

- [ ] Wire coinstake reward checks to PoS epoch budget constraints.
- [ ] Keep staking kernel/signature checks unchanged unless required.
- [ ] Add explicit rejection path for over-budget PoS reward.

Deliverables

- [ ] Over-budget coinstake transactions rejected by consensus.

Validation Gate

- [ ] New tests pass for valid/invalid PoS rewards around epoch edges.

Suggested commit

- [ ] `consensus: enforce PoS issuance budget in tx validation`

---

## Phase 6: Align Block Creation to New Consensus Rules

Goal: Ensure miner/staker template generation produces acceptable rewards.

Primary files

- `src/node/miner.cpp`
- `src/wallet/wallet.cpp`
- `src/validation.cpp`

Tasks

- [ ] Update PoW template reward proposal to match new acceptance rules.
- [ ] Update coinstake construction expectations if needed.
- [ ] Verify no wallet-only assumptions conflict with consensus paths.

Deliverables

- [ ] Templates from PoW and PoS paths are consistently accepted.

Validation Gate

- [ ] Hybrid block production works without manual reward patching.

Suggested commit

- [ ] `consensus: align block template reward proposals with 50/50 rules`

---

## Phase 7: End-to-End Hybrid Validation

Goal: Prove operational correctness on local multi-node network.

Tasks

- [ ] Launch at least two local nodes (PoW producer + PoS staker).
- [ ] Generate mature stakeable UTXOs and unlock wallet mint-only.
- [ ] Produce both PoW and PoS blocks.
- [ ] Confirm acceptance under normal conditions.
- [ ] Run deliberate over-budget negative tests.

Deliverables

- [ ] Reproducible local network script/runbook.
- [ ] Short issuance split validation report.

Validation Gate

- [ ] Both normal and negative-path checks pass.

Suggested commit

- [ ] `test: hybrid local network validation for 50/50 issuance`

---

## Phase 8: Mining Compatibility Smoke Tests

Goal: Validate ASIC/pool-facing compatibility assumptions.

Primary files

- `src/rpc/mining.cpp`
- `src/primitives/block.h`
- `src/primitives/block.cpp`
- `src/hash.h`

Tasks

- [ ] Confirm `getblocktemplate` fields are usable by pool software.
- [ ] Confirm `submitblock` path works end-to-end.
- [ ] Verify PoW header hash compatibility expectations (SHA-256d path).
- [ ] Document adaptation notes for ckpool/public-pool/GoPool.

Deliverables

- [ ] Compatibility test note and known gaps list.

Validation Gate

- [ ] At least one external-style submit flow succeeds.

Suggested commit

- [ ] `docs: mining compatibility and pool integration notes`

---

## Test Matrix (Must Pass Before Testnet)

Consensus

- [ ] PoW reward boundary tests.
- [ ] PoS reward boundary tests.
- [ ] Epoch rollover reward tests.
- [ ] Reorg across epoch boundary tests.

Integration

- [ ] PoW block generation acceptance.
- [ ] PoS block generation acceptance.
- [ ] Mixed-chain progression with stable validation.

RPC

- [ ] `getblocktemplate` smoke test.
- [ ] `submitblock` smoke test.

---

## Risk Register

High

- [ ] Consensus divergence from budget accounting bugs.
- [ ] Reorg accounting inconsistency.
- [ ] Rounding/carry edge case causing supply drift.

Medium

- [ ] Pool integration mismatches.
- [ ] Staking usability friction from current txindex dependency.

Low

- [ ] Rebrand constant omissions.
- [ ] Documentation lag.

Mitigations

- [ ] Consensus changes isolated into small commits.
- [ ] Mandatory negative tests per consensus commit.
- [ ] Reorg-specific tests before phase completion.

---

## Commit Plan (Tracking)

- [ ] C1 Baseline and instrumentation
- [ ] C2 Rebrand identifiers
- [ ] C3 Network constants and ports
- [ ] C4 Genesis replacement
- [ ] C5 Budget primitives
- [ ] C6 PoW enforcement
- [ ] C7 PoS enforcement
- [ ] C8 Template alignment
- [ ] C9 Hybrid validation tests
- [ ] C10 Mining compatibility docs

---

## Progress Log

- 2026-05-28: Plan document created.
- 2026-05-30: Broad non-consensus rebrand pass completed across docs/UI/help/manpages and runtime-facing links.
- 2026-05-30: Runtime check confirmed rebranded daemon/container startup and healthy RPC path (`equalishcoin-cli` with explicit datadir/user).
- 2026-05-30: Docker architecture cleanup in progress; compose no longer forces amd64, but fresh no-cache image build still failing in builder stage object-path generation.

## Notes / Open Questions

- [ ] Final epoch size and emission schedule constants.
- [ ] Exact rounding and carry policy (strict cap vs rollover).
- [ ] Whether legacy reward functions remain as bounded inputs or are fully replaced.
- [ ] Minimum acceptable split tolerance within an epoch (if any) vs strict endpoint equality.
