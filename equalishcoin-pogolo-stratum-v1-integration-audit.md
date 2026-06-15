# EqualishCoin -> Pogolo Stratum V1 Integration Audit

## Executive Summary

EqualishCoin can be mined through a Bitcoin-style Stratum V1 pool only after the pool stops assuming Bitcoin block and transaction bytes.

The biggest confirmed blockers are:

- `BITCOIN DIFFERENCE` EqualishCoin block serialization appends `vchBlockSig` after the transaction vector, even for PoW blocks where the signature is currently empty.
  Source: `src/primitives/block.h` | type `CBlock` | function `SERIALIZE_METHODS(CBlock, obj)`
- `POOL INTEGRATION RISK` Pogolo submits a `btcd/wire.MsgBlock`, which does not include EqualishCoin's trailing `vchBlockSig` field, so raw `submitblock` payloads are not consensus-compatible.
  Source: `/tmp/pogolo/main.go` | type `blockSubmission` pipeline | function main submission loop calling `backend.SubmitBlock`
  Source: `/tmp/pogolo/jobtemplate.go` | type `MiningJob` | function `UpdateBlock`
- `BITCOIN DIFFERENCE` EqualishCoin transactions are not universally Bitcoin wire format. For transaction versions `< 3`, `nTime` is serialized immediately after `nVersion`.
  Source: `src/primitives/transaction.h` | type template serializer | functions `SerializeTransaction`, `UnserializeTransaction`
- `POOL INTEGRATION RISK` Pogolo constructs the coinbase with btcd's Bitcoin transaction type, version `1`, and `LockTime = height - 1`, while EqualishCoin's block assembler creates a version `3` coinbase and sets `scriptSig = CScript() << height << OP_0`.
  Source: `src/node/miner.cpp` | type `BlockAssembler` | function `CreateNewBlock`
  Source: `src/primitives/transaction.h` | type `CTransaction` | field `CURRENT_VERSION`
  Source: `/tmp/pogolo/util.go` | function `CreateEmptyCoinbase`
- `BITCOIN DIFFERENCE` EqualishCoin is hybrid PoW/PoS. Pools should generate only PoW blocks from `getblocktemplate`; PoS blocks are identified by a coinstake as transaction index `1` and follow different timestamp, reward, and signature rules.
  Source: `src/primitives/block.h` | type `CBlock` | functions `IsProofOfStake`, `IsProofOfWork`
  Source: `src/node/miner.cpp` | type `BlockAssembler` | function `CreateNewBlock`

Bottom line:

- Stratum V1 as a protocol is still usable.
- Pogolo cannot be reused unchanged.
- The integration is implementation-ready if the pool is taught EqualishCoin transaction/block serialization, EqualishCoin address formats, EqualishCoin coinbase/reward rules, and EqualishCoin full-block submission bytes.

## Chain Facts Table

| Topic | Value | Source |
| --- | --- | --- |
| Network name | `main`, `test`, `regtest` | `src/chainparamsbase.cpp` \| type `CBaseChainParams` \| function `CreateBaseChainParams` |
| Mainnet magic | `f7 c1 ee 50` | `src/kernel/chainparams.cpp` \| type `CMainParams` \| constructor `CMainParams()` |
| Testnet magic | `ce fa 51 10` | `src/kernel/chainparams.cpp` \| type `CTestNetParams` \| constructor `CTestNetParams()` |
| Regtest magic | `fa ce 51 1a` | `src/kernel/chainparams.cpp` \| type `CRegTestParams` \| constructor `CRegTestParams()` |
| Mainnet P2P port | `19901` | `src/kernel/chainparams.cpp` \| type `CMainParams` \| constructor `CMainParams()` |
| Testnet P2P port | `29901` | `src/kernel/chainparams.cpp` \| type `CTestNetParams` \| constructor `CTestNetParams()` |
| Regtest P2P port | `39901` | `src/kernel/chainparams.cpp` \| type `CRegTestParams` \| constructor `CRegTestParams()` |
| Mainnet RPC port | `19902` | `src/chainparamsbase.cpp` \| type `CBaseChainParams` \| function `CreateBaseChainParams` |
| Testnet RPC port | `29902` | `src/chainparamsbase.cpp` \| type `CBaseChainParams` \| function `CreateBaseChainParams` |
| Regtest RPC port | `39902` | `src/chainparamsbase.cpp` \| type `CBaseChainParams` \| function `CreateBaseChainParams` |
| Mainnet onion/listen port | `19903` | `src/chainparamsbase.cpp` \| type `CBaseChainParams` \| function `CreateBaseChainParams` |
| Testnet onion/listen port | `29903` | `src/chainparamsbase.cpp` \| type `CBaseChainParams` \| function `CreateBaseChainParams` |
| Regtest onion/listen port | `39903` | `src/chainparamsbase.cpp` \| type `CBaseChainParams` \| function `CreateBaseChainParams` |
| Mainnet pubkey prefix | `33` | `src/kernel/chainparams.cpp` \| type `CMainParams` \| constructor `CMainParams()` |
| Mainnet script prefix | `28` | `src/kernel/chainparams.cpp` \| type `CMainParams` \| constructor `CMainParams()` |
| Mainnet WIF prefix | `161` | `src/kernel/chainparams.cpp` \| type `CMainParams` \| constructor `CMainParams()` |
| Mainnet bech32 HRP | `eqls` | `src/kernel/chainparams.cpp` \| type `CMainParams` \| constructor `CMainParams()` |
| Testnet/regtest pubkey prefix | `65` | `src/kernel/chainparams.cpp` \| types `CTestNetParams`, `CRegTestParams` \| constructors |
| Testnet/regtest script prefix | `196` | `src/kernel/chainparams.cpp` \| types `CTestNetParams`, `CRegTestParams` \| constructors |
| Testnet/regtest WIF prefix | `239` | `src/kernel/chainparams.cpp` \| types `CTestNetParams`, `CRegTestParams` \| constructors |
| Testnet bech32 HRP | `teqls` | `src/kernel/chainparams.cpp` \| type `CTestNetParams` \| constructor `CTestNetParams()` |
| Regtest bech32 HRP | `reqls` | `src/kernel/chainparams.cpp` \| type `CRegTestParams` \| constructor `CRegTestParams()` |
| Mainnet genesis hash | `00000000bff3777281affbc6aa04d22330deb32aed40ece82fec0f476690ae9a` | `src/kernel/chainparams.cpp` \| type `CMainParams` \| constructor `CMainParams()` |
| Testnet genesis hash | `0000000ee94aba0c4c7e7852c00faa4eaa1e33f91a665cf02959b75d93676cb3` | `src/kernel/chainparams.cpp` \| type `CTestNetParams` \| constructor `CTestNetParams()` |
| Regtest genesis hash | `7a1056e04931dd1ea5d8ca7a7f05a9ab80a1b5b51d310b3b5be83ff9393db042` | `src/kernel/chainparams.cpp` \| type `CRegTestParams` \| constructor `CRegTestParams()` |
| Block header version | `6` default | `src/primitives/block.h` \| type `CBlockHeader` \| constant `CURRENT_VERSION` |
| Transaction version | `3` default | `src/primitives/transaction.h` \| type `CTransaction` \| constant `CURRENT_VERSION` |
| Overall target block spacing | `600s` | `src/kernel/chainparams.cpp` \| consensus field `nStakeTargetSpacing` |
| Pre-v14 PoW retarget cadence | Every block, hybrid moving-average formula | `src/pow.cpp` \| function `GetNextTargetRequired` |
| v14+ PoW difficulty algorithm | ASERT on PoW only | `src/pow.cpp` \| functions `GetNextASERTWorkRequired`, `CalculateASERT` |
| ASERT ideal PoW spacing | `nStakeTargetSpacing * 6 = 3600s` | `src/pow.cpp` \| function `GetNextASERTWorkRequired` |
| Main/test `powLimit` | `0000000fffffffffffffffffffffffffffffffffffffffffffffffffffffffff` | `src/kernel/chainparams.cpp` \| types `CMainParams`, `CTestNetParams` |
| Regtest `powLimit` | `7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff` | `src/kernel/chainparams.cpp` \| type `CRegTestParams` |
| SegWit | Active from height `1` on main/test, `0` on regtest | `src/kernel/chainparams.cpp` \| consensus fields `SegwitHeight` |
| Buried deployments | BIP34, BIP65, BIP66, CSV active from height `1` | `src/kernel/chainparams.cpp` \| consensus fields |
| Version bits deployments | `testdummy` bit `28`, `taproot` bit `2`; both always active on main/test, configurable on regtest | `src/kernel/chainparams.cpp` \| consensus `vDeployments`; `src/deploymentinfo.cpp` \| `VersionBitsDeploymentInfo` |
| Coinbase maturity | main `30`, test/regtest `60` | `src/kernel/chainparams.cpp` \| consensus field `nCoinbaseMaturity`; `src/consensus/tx_verify.cpp` \| spend maturity checks |
| Address support | Legacy Base58 and Bech32 confirmed live; wallet RPC also exposes Bech32m code path | `src/wallet/rpc/addresses.cpp` \| function `getnewaddress`; live RPC examples below |

## Proof-of-Work and Header Rules

1. EqualishCoin PoW hash is the block header hash returned by `CBlockHeader::GetHash()`.
   Source: `src/primitives/block.cpp` | type `CBlockHeader` | function `GetHash`

2. `CBlockHeader::GetHash()` uses `SerializeHash(tmp)` after zeroing `nFlags`.
   `BITCOIN DIFFERENCE` EqualishCoin carries an extra `nFlags` field in the header object, but it is explicitly excluded from the PoW hash and from normal header serialization unless the stream has `SER_POSMARKER`.
   Source: `src/primitives/block.h` | type `CBlockHeader` | function `SERIALIZE_METHODS(CBlockHeader, obj)`
   Source: `src/primitives/block.cpp` | type `CBlockHeader` | function `GetHash`

3. The hashed PoW header field order is still:
   `nVersion | hashPrevBlock | hashMerkleRoot | nTime | nBits | nNonce`
   and the normal hashed size is fixed at `80` bytes.
   Source: `src/primitives/block.h` | type `CBlockHeader` | fields and constant `NORMAL_SERIALIZE_SIZE`

4. `CheckProofOfWork` converts compact `nBits` into an `arith_uint256` target with `SetCompact`, range-checks it against `powLimit`, then compares `UintToArith256(hash) <= bnTarget`.
   Source: `src/pow.cpp` | function `CheckProofOfWork`

5. `BITCOIN DIFFERENCE` There is no separate `GetPoWHash` implementation. The proof hash for PoW is the normal block hash. RPC confirms this by returning `proofhash == hash` for PoW blocks.
   Source: `src/rpc/blockchain.cpp` | function `blockToJSON`
   Live example: current PoW tip returned `proofhash` equal to `hash` and `flags: "proof-of-work"`.

6. ASIC compatibility is therefore the standard 80-byte SHA256d Bitcoin/Peercoin mining path.
   Source: `src/primitives/block.cpp` | function `CBlockHeader::GetHash`
   Source: `src/pow.cpp` | function `CheckProofOfWork`

## Hybrid PoW / PoS Rules

1. A block is PoS if `vtx.size() > 1 && vtx[1]->IsCoinStake()`; otherwise it is PoW.
   Source: `src/primitives/block.h` | type `CBlock` | functions `IsProofOfStake`, `IsProofOfWork`

2. A coinstake transaction is any non-coinbase transaction with at least two outputs and an empty first output.
   Source: `src/primitives/transaction.h` | type `CTransaction` | function `IsCoinStake`

3. `CheckBlock` enforces:
   - only transaction index `0` may be coinbase
   - only transaction index `1` may be coinstake
   - PoS coinbase output `0` must be empty
   - PoS block/coinstake timestamps must satisfy `CheckCoinStakeTimestamp`
   Source: `src/validation.cpp` | function `CheckBlock`
   Source: `src/kernel.cpp` | function `CheckCoinStakeTimestamp`

4. Pools should only generate PoW blocks via `getblocktemplate`. `BlockAssembler::CreateNewBlock()` only attempts coinstake when a wallet is supplied; RPC GBT calls it with `pwallet == nullptr`, so returned templates are PoW-only.
   Source: `src/node/miner.cpp` | type `BlockAssembler` | function `CreateNewBlock`
   Source: `src/rpc/mining.cpp` | function `getblocktemplate`

5. `POOL INTEGRATION RISK` Difficulty adjustment is hybrid. `GetNextTargetRequired` walks the previous chain by PoW/PoS class using `GetLastBlockIndex`, and PoW ASERT height-diff subtracts stake heights.
   Source: `src/pow.cpp` | functions `GetNextTargetRequired`, `GetNextASERTWorkRequired`
   Source: `src/chain.cpp` | function `GetLastBlockIndex`

6. Reward rules differ by block type:
   - PoW reward comes from `GetProofOfWorkReward(nBits, nTime)`
   - PoW templates further restrict spendable reward to `SplitIssuanceBudget(...).pow_budget`
   - PoS reward uses `GetProofOfStakeReward`
   Source: `src/validation.cpp` | functions `GetProofOfWorkReward`, `GetProofOfStakeReward`, `SplitIssuanceBudget`
   Source: `src/node/miner.cpp` | function `CreateNewBlock`

## Mining RPC Audit

| RPC | Status | Notes | Source |
| --- | --- | --- | --- |
| `getblocktemplate` | Supported | BIP22/BIP23/BIP9/BIP145-shaped. Requires `rules:["segwit"]`. Longpoll supported. Returns `coinbasevalue`, not `coinbasetxn`. | `src/rpc/mining.cpp` \| function `getblocktemplate` |
| `submitblock` | Supported | Accepts full serialized block hex. Returns `null` on success or BIP22-style reject strings. | `src/rpc/mining.cpp` \| function `submitblock` |
| `getwork` | Not supported | Not registered; live RPC returns `Method not found`. | `src/rpc/mining.cpp` \| function `RegisterMiningRPCCommands`; live RPC |
| `getmininginfo` | Supported | Includes `difficulty`, `networkhashps`, custom `networkghps`, `currentblockweight`, `currentblocktx`. | `src/rpc/mining.cpp` \| function `getmininginfo` |
| `getnetworkhashps` | Supported | Uses chain trust deltas, with a source comment saying it should be redone for Peercoin. | `src/rpc/mining.cpp` \| functions `GetNetworkHashPS`, `getnetworkhashps` |
| `getblock` | Supported | Verbosity `0-3`. Includes EqualishCoin extras such as `flags`, `proofhash`, `blocksignature`. | `src/rpc/blockchain.cpp` \| function `getblock` |
| `getblockhash` | Supported | Standard height -> hash lookup. | `src/rpc/blockchain.cpp` \| function `getblockhash` |
| `validateaddress` | Supported | Uses EqualishCoin `DecodeDestination`/`EncodeDestination`. | `src/rpc/output_script.cpp` \| function `validateaddress` |
| `getnewaddress` | Supported | Wallet RPC. Supports `legacy`, `bech32`, `bech32m`; explicitly rejects `p2sh-segwit`. | `src/wallet/rpc/addresses.cpp` \| function `getnewaddress` |

Observed live responses on the running node:

- `getwork` -> `error code: -32601`, `Method not found`
- `getblockhash 1` -> `000000001c24a0cd308411eb8b76773fc842e78eda84bd055de26e749b82ca01`
- `getblock <tip> 1` on current PoW tip showed `version: 6`, `flags: "proof-of-work"`, `proofhash == hash`, `blocksignature: ""`
- `getnewaddress` on wallet `bootstrap` produced legacy `EcipqhJJdrG6pXtuZc1p2SgGvb8ftcAp6G`
- `getnewaddress "" bech32` produced `eqls1q6zculhlxx43fx0nqhaeft6tcdvs37t7p3nuz2m`

`POOL INTEGRATION RISK` On mainnet, `getblocktemplate` still refuses service when the node has no peers:

- live response: `error code: -9`, `EqualishCoin is not connected!`

Source: `src/rpc/mining.cpp` | function `getblocktemplate`

## Block Template Construction

Template generation path:

`getblocktemplate` RPC -> `BlockAssembler(...).CreateNewBlock(scriptDummy, nullptr, nullptr, &node)` -> `UpdateTime()` -> JSON conversion

Source: `src/rpc/mining.cpp` | function `getblocktemplate`
Source: `src/node/miner.cpp` | type `BlockAssembler` | function `CreateNewBlock`

Exact returned fields confirmed by source:

- `version`
- `rules`
- `capabilities`
- `previousblockhash`
- `transactions[]`
- `coinbaseaux`
- `coinbasevalue`
- `longpollid`
- `target`
- `mintime`
- `mutable`
- `noncerange`
- `sigoplimit`
- `sizelimit`
- optional `weightlimit`
- `curtime`
- `bits`
- `height`
- optional `signet_challenge`
- optional `default_witness_commitment`

Source: `src/rpc/mining.cpp` | function `getblocktemplate`

Important template behavior:

- `BITCOIN DIFFERENCE` EqualishCoin GBT requires the caller to advertise `segwit` support even on this Peercoin-derived chain.
  Source: `src/rpc/mining.cpp` | function `getblocktemplate`
- `coinbasetxn` is not returned; the pool must build the coinbase itself.
  Source: `src/rpc/mining.cpp` | function `getblocktemplate`
- The template is PoW-only because GBT does not supply a wallet/coinstake path.
  Source: `src/node/miner.cpp` | function `CreateNewBlock`
- Transaction entries expose `data`, `txid`, `hash`, `depends`, `fee`, `sigops`, `weight`.
  Source: `src/rpc/mining.cpp` | function `getblocktemplate`

## Coinbase Transaction Rules

1. EqualishCoin's mined coinbase is created inside `BlockAssembler::CreateNewBlock` as a `CMutableTransaction` defaulting to transaction version `3`.
   `BITCOIN DIFFERENCE` Pogolo currently creates a version `1` coinbase.
   Source: `src/node/miner.cpp` | function `CreateNewBlock`
   Source: `src/primitives/transaction.cpp` | constructor `CMutableTransaction::CMutableTransaction`
   Source: `/tmp/pogolo/util.go` | function `CreateEmptyCoinbase`

2. For transaction versions `< 3`, EqualishCoin serializes `nTime` immediately after `nVersion`. For version `3`, `nTime` is not serialized.
   `POOL INTEGRATION RISK` Any pool-side transaction builder must reproduce this exactly.
   Source: `src/primitives/transaction.h` | functions `SerializeTransaction`, `UnserializeTransaction`

3. EqualishCoin's PoW coinbase scriptSig is built as `CScript() << nHeight << OP_0`.
   Source: `src/node/miner.cpp` | function `CreateNewBlock`

4. Coinbase value is not Bitcoin subsidy-plus-fees. The assembled coinbase output is set to the PoW issuance split budget:

   `pow_budget = SplitIssuanceBudget(consensus, GetProofOfWorkReward(nBits, nTime)).pow_budget`

   Source: `src/node/miner.cpp` | function `CreateNewBlock`
   Source: `src/validation.cpp` | functions `GetProofOfWorkReward`, `SplitIssuanceBudget`

5. `BITCOIN DIFFERENCE` The PoW subsidy curve is difficulty-based, not a fixed halving schedule.
   Source: `src/validation.cpp` | function `GetProofOfWorkReward`

6. Fees are accounted for through `pblocktemplate->vTxFees[0] = -nFees`; `coinbasevalue` returned by GBT is the maximum allowed coinbase input and includes subsidy plus fees.
   Source: `src/node/miner.cpp` | function `CreateNewBlock`
   Source: `src/rpc/mining.cpp` | function `getblocktemplate`

7. Coinbase payout script can be any script the pool chooses when creating the block, but pre-BTC16 block signatures only work if the payout output is raw `PUBKEY`.
   `POOL INTEGRATION RISK` If anyone needs to mine historical pre-BTC16 PoW blocks, a P2PKH or Bech32 coinbase output cannot satisfy `CheckBlockSignature` because `Solver(...)` must return `TxoutType::PUBKEY`.
   Source: `src/validation.cpp` | functions `SignBlock`, `CheckBlockSignature`

## Block Serialization and Block Signatures

1. Full block serialization is:

   `CBlockHeader | vector<CTransaction> vtx | vector<unsigned char> vchBlockSig`

   Source: `src/primitives/block.h` | type `CBlock` | function `SERIALIZE_METHODS(CBlock, obj)`

2. `BITCOIN DIFFERENCE` Even when the block signature is empty, the serialized block still contains the CompactSize-encoded zero-length vector after the transactions.
   `POOL INTEGRATION RISK` A Bitcoin-serialized block byte stream is therefore incomplete for EqualishCoin `submitblock`.
   Source: `src/primitives/block.h` | type `CBlock` | function `SERIALIZE_METHODS(CBlock, obj)`
   Source: `src/rpc/mining.cpp` | function `submitblock`

3. Block signatures are generated and verified against the block hash, using the pubkey from:

   - PoS: `block.vtx[1]->vout[1]`
   - PoW: `block.vtx[0]->vout[0]`

   Source: `src/validation.cpp` | functions `SignBlock`, `CheckBlockSignature`

4. `Are block signatures required for PoW blocks?`

   - Before BTC16 activation time: yes, because `CheckBlock` verifies signatures for PoS blocks and for all blocks when `!IsBTC16BIPsEnabled(block.GetBlockTime())`.
   - After BTC16 activation time: current-source behavior does not verify PoW block signatures in `CheckBlock`.

   Source: `src/validation.cpp` | function `CheckBlock`
   Source: `src/kernel.cpp` | function `IsBTC16BIPsEnabled`
   Source: `src/kernel.cpp` | constants `nBTC16BIPsSwitchTime`, `nBTC16BIPsTestSwitchTime`

5. Live current-chain evidence matches the source: current PoW tip JSON shows `"blocksignature": ""`.
   Source: `src/rpc/blockchain.cpp` | function `blockToJSON`

## Stratum V1 Mapping for EqualishCoin

### `mining.subscribe`

- Can be reused as protocol.
- `extranonce1` and `extranonce2_size` remain pool-defined.
- No EqualishCoin consensus dependency here.

`POOL INTEGRATION RISK` Pogolo currently uses its `stratum.ID` as `extranonce1` and assumes its own coinbase layout. That layout must be regenerated against EqualishCoin bytes.
Source: `/tmp/pogolo/stratumclient.go` | function `Run`

### `mining.authorize`

- Can be reused as protocol.
- Username/address validation must use EqualishCoin address codecs, not btcd Bitcoin codecs.

`BITCOIN DIFFERENCE` Pogolo currently calls `btcutil.DecodeAddress(..., backendChainParams)`.
`POOL INTEGRATION RISK` The backend chain params must become EqualishCoin-specific and the payout script builder must match EqualishCoin accepted address/script types.
Source: `/tmp/pogolo/stratumclient.go` | function `Run`
Source: `src/wallet/rpc/addresses.cpp` | function `getnewaddress`
Source: `src/rpc/output_script.cpp` | function `validateaddress`

### `mining.notify`

Field mapping should be:

- `job_id`: pool-local identifier
- `prevhash`: EqualishCoin previous block hash from GBT, re-encoded into Stratum's header-oriented byte order expected by miners
- `coinb1`, `coinb2`: EqualishCoin coinbase prefix/suffix bytes, split around extranonce insertion, built from an EqualishCoin transaction serializer
- `merkle_branches`: hashes derived from EqualishCoin txids / witness rules
- `version`: template version from GBT, forwarded unchanged unless a consciously approved version-rolling mask is in use
- `nbits`: compact target bytes from GBT `bits`
- `ntime`: pool-selected timestamp within `mintime..maxtime/current rules`
- `clean_jobs`: normal Stratum semantic

`BITCOIN DIFFERENCE` The coinbase bytes cannot be produced by btcd's Bitcoin serializer because EqualishCoin block assembly and transaction serialization differ.
Source: `src/rpc/mining.cpp` | function `getblocktemplate`
Source: `src/node/miner.cpp` | function `CreateNewBlock`
Source: `src/primitives/transaction.h` | functions `SerializeTransaction`, `UnserializeTransaction`
Source: `/tmp/pogolo/stratumclient.go` | function `createJob`

### `mining.set_difficulty`

- Share difficulty protocol can be reused.
- Prefer exact target comparison rather than float-only difficulty math.

EqualishCoin difficulty display formula still uses the Bitcoin-style `0x0000ffff` baseline.
Source: `src/rpc/blockchain.cpp` | function `GetDifficulty`

### `mining.submit`

EqualishCoin share handling should be:

1. Validate job ID and version mask policy.
2. Rebuild the EqualishCoin coinbase transaction bytes.
3. Recompute merkle root from EqualishCoin txids.
4. Rebuild the 80-byte PoW header with `nVersion`, `hashPrevBlock`, `hashMerkleRoot`, `nTime`, `nBits`, `nNonce`.
5. Compute SHA256d block hash.
6. Compare against share target.
7. Compare against network target decoded from `nBits`.
8. If block candidate, build full EqualishCoin block bytes including trailing `vchBlockSig` vector and call `submitblock`.

## Share Validation Recipe

Recommended implementation recipe:

1. Receive `mining.submit(worker, job_id, extranonce2, ntime, nonce, [version_mask])`.
2. Rebuild coinbase as EqualishCoin bytes, not btcd Bitcoin bytes.
   Source: `src/node/miner.cpp` | function `CreateNewBlock`
   Source: `src/primitives/transaction.h` | function `SerializeTransaction`
3. Hash the rebuilt coinbase to get coinbase txid / witness hash as needed.
   Source: `src/primitives/transaction.cpp` | functions `ComputeHash`, `ComputeWitnessHash`
4. Recompute the merkle root using EqualishCoin txids in template order.
   Source: `src/consensus/merkle.h` and `src/rpc/mining.cpp` transaction ordering usage
5. Rebuild the 80-byte header and hash it with `CBlockHeader::GetHash` semantics.
   Source: `src/primitives/block.cpp` | function `CBlockHeader::GetHash`
6. Decode `nBits` with compact-target logic equivalent to `arith_uint256::SetCompact` and compare the header hash to the share target.
   Source: `src/pow.cpp` | function `CheckProofOfWork`
7. Detect a block candidate when `hash <= network_target`.
8. Reject any timestamp outside the block template's legal window. EqualishCoin additionally clamps time through `UpdateTime`, `mintime`, median-time-past, and transaction times.
   Source: `src/rpc/mining.cpp` | function `getblocktemplate`
   Source: `src/node/miner.cpp` | function `CreateNewBlock`

`POOL INTEGRATION RISK` Pogolo's current validator only checks a Bitcoin-style rebuilt header and target, and it never reasons about EqualishCoin's trailing block signature vector or transaction-format differences.
Source: `/tmp/pogolo/stratumclient.go` | function `validateShareSubmission`

## Found Block Submission Recipe

1. Start from the accepted candidate share.
2. Rebuild the full EqualishCoin coinbase transaction.
3. Assemble the final EqualishCoin block with:
   - EqualishCoin header
   - full transaction vector
   - trailing `vchBlockSig` vector
4. For current PoW blocks, serialize an empty `vchBlockSig` unless historical rules require otherwise.
5. Hex-encode the full block bytes.
6. Call `submitblock <hex>`.
7. Interpret results:
   - `null` => accepted
   - `duplicate`, `duplicate-invalid`, `inconclusive`, or reject reason string => rejected/not confirmed

Source: `src/rpc/mining.cpp` | function `submitblock`
Source: `src/primitives/block.h` | type `CBlock` | serializer
Source: `src/validation.cpp` | function `CheckBlock`

Common reject causes from source:

- bad PoW target / hash
- malformed block bytes
- bad coinbase amount
- bad coinstake position if block layout is wrong
- bad timestamp
- bad merkle root
- bad block signature on historical pre-BTC16 blocks

Source: `src/pow.cpp` | function `CheckProofOfWork`
Source: `src/validation.cpp` | function `CheckBlock`

## Pogolo Audit and Change List

| File | Function | Bitcoin assumption | Required modification | Risk |
| --- | --- | --- | --- | --- |
| `/tmp/pogolo/util.go` | `CreateEmptyCoinbase` | Bitcoin btcd tx format, tx version `1`, BIP54 locktime/sequence, Bitcoin script builder assumptions | Replace with EqualishCoin coinbase builder that matches `BlockAssembler::CreateNewBlock` bytes and reward rules | High |
| `/tmp/pogolo/util.go` | `SerializeCoinbaseTx` | Bitcoin `SerializeNoWitness` is sufficient | Replace with EqualishCoin transaction serialization, including version/time rules | High |
| `/tmp/pogolo/util.go` | `FillCoinbaseTx` | Bitcoin address and witness-commitment handling | Rebuild payout output and witness commitment according to EqualishCoin tx/block types | High |
| `/tmp/pogolo/jobtemplate.go` | `CreateJobTemplate` | btcd can decode all template tx bytes and build correct merkle tree | Audit every template tx against EqualishCoin tx serialization; at minimum stop assuming all txs are btcd-native Bitcoin txs | High |
| `/tmp/pogolo/jobtemplate.go` | `UpdateBlock` | Rebuilding only scriptSig is enough and resulting block bytes are Bitcoin-valid | Rebuild EqualishCoin coinbase and final block bytes, then recompute merkle root with EqualishCoin hashes | High |
| `/tmp/pogolo/stratumclient.go` | `createJob` | Coinbase split into `coinb1/coinb2` using btcd-serialized transaction bytes | Split EqualishCoin coinbase bytes instead | High |
| `/tmp/pogolo/stratumclient.go` | `validateShareSubmission` | Bitcoin-style rebuilt header/block is sufficient for share validation | Use EqualishCoin serialization and target logic; keep timestamp window checks | High |
| `/tmp/pogolo/main.go` | main submission loop | `backend.SubmitBlock` with btcd block bytes matches backend consensus | Submit EqualishCoin full-block hex including trailing `vchBlockSig` | High |
| `/tmp/pogolo/stratumclient.go` | `Run` authorize path | `btcutil.DecodeAddress` with backend chain params is enough | Implement EqualishCoin address parsing or obtain scriptPubKey from node-side helper/RPC | Medium |
| `/tmp/pogolo/util.go` | `CalcDifficulty`, `CalcNetworkDifficulty` | Bitcoin diff baseline | Can likely be reused for display only, but share acceptance should compare integer targets directly | Low |
| `/tmp/pogolo/main.go` | GBT request loop | Bitcoin-style `rules:["segwit"]`, `coinbasevalue`, `proposal` | Reusable as-is for EqualishCoin GBT request shape | Low |

## Bitcoin-Compatible vs Equalish-Specific Summary

Can be reused mostly unchanged:

- Stratum message flow: `mining.subscribe`, `mining.authorize`, `mining.notify`, `mining.set_difficulty`, `mining.submit`
- SHA256d header hashing logic
- Compact `nBits` target math conceptually
- GBT polling pattern and BIP22 `submitblock` result handling

Must be modified or abstracted:

- coinbase transaction construction
- transaction serialization and hashing assumptions
- full block serialization
- address decoding and payout script building
- block submission bytes
- historical block-signature handling
- reward calculation and issuance split

## Validation Plan

Suggested commands:

1. Start a local node with wallet and RPC.
   - Example mainnet container path already in repo: `docker compose -f /opt/equalishcoin/contrib/docker-compose.yml up -d`

2. Check mining RPC availability.
   - `equalishcoin-cli getmininginfo`
   - `equalishcoin-cli getblocktemplate '{"rules":["segwit"]}'`
   - `equalishcoin-cli getwork` expected `Method not found`

3. Confirm address formats for payout testing.
   - `equalishcoin-cli -rpcwallet=<wallet> getnewaddress`
   - `equalishcoin-cli -rpcwallet=<wallet> getnewaddress "" bech32`
   - `equalishcoin-cli validateaddress <address>`

4. Validate PoW candidate path.
   - Fetch GBT
   - Build coinbase with EqualishCoin serializer
   - Recompute merkle root
   - Build 80-byte header
   - Hash with SHA256d
   - Compare to share target and network target

5. Validate block submission bytes.
   - Serialize final block including trailing `vchBlockSig`
   - `equalishcoin-cli submitblock <hex>`

6. Regtest / low-difficulty testing.
   - Intended path: run `equalishcoind -regtest` and use `getblocktemplate`, `submitblock`, and low target shares
   - `POOL INTEGRATION RISK` In this workspace, local `equalishcoind -regtest` crashed with a segmentation fault before serving RPC, so regtest verification is still outstanding.

## Open Questions

1. `POOL INTEGRATION RISK` Regtest runtime is currently unresolved because local `equalishcoind -regtest` segfaulted in this workspace before the audit could capture a live regtest GBT.

2. `POOL INTEGRATION RISK` Historical pre-BTC16 PoW block-signature requirements need an explicit maintainer decision if the pool is expected to mine historical chains or replay old heights. Current-chain PoW does not appear to require a non-empty signature.

3. `POOL INTEGRATION RISK` Pogolo's tx decoding assumes btcd can decode every GBT transaction. This is probably safe for current version-3 mempool transactions but should still be runtime-verified against real EqualishCoin templates with non-coinbase transactions.

4. `POOL INTEGRATION RISK` Version rolling policy should be confirmed with maintainers. EqualishCoin exposes version bits deployments, but the pool should probably forward the node's template version unchanged until there is a confirmed mask policy.

5. Maintainer confirmation is advisable for the intended payout script policy for pools:
   - legacy P2PKH only
   - Bech32 v0 allowed
   - Taproot / Bech32m allowed

6. Repository note: `src/kernel/chainparams.cpp` sets regtest `consensus.hashGenesisBlock` to `7a1056...` but `checkpointData` height `0` to `00000001f757...`. That inconsistency should be confirmed before relying on regtest for integration work.

## Minimal Implementation Recipe

If the next task is implementation, the shortest viable path is:

1. Keep Pogolo's Stratum transport and client/session logic.
2. Replace Pogolo's coinbase builder with an EqualishCoin-native builder matching `BlockAssembler::CreateNewBlock`.
3. Replace Pogolo's transaction and block serializers with EqualishCoin-native serializers.
4. Keep SHA256d header hashing and target comparison.
5. Serialize full candidate blocks with trailing `vchBlockSig` and submit through `submitblock`.
6. Use EqualishCoin address parsing or node-provided scriptPubKey generation instead of btcd Bitcoin address handling.
