
#include "staking.h"
#include <chain.h>
#include <chainparams.h>
#include <interfaces/wallet.h>
#include <kernel.h>
#include <kernel/cs_main.h>
#include <kernelrecord.h>
#include <node/miner.h>
#include <pow.h>
#include <rpc/blockchain.h>
#include <rpc/protocol.h>
#include <rpc/server.h>
#include <timedata.h>
#include <txmempool.h>
#include <univalue.h>
#include <cmath>
#include <util/system.h>
#include <util/time.h>
#include <wallet/rpc/util.h>
#include <warnings.h>

using node::BlockAssembler;
using wallet::EnsureWalletContext;
using wallet::GetWalletForJSONRPCRequest;
using wallet::WalletContext;

RPCHelpMan getstakinginfo()
{
    return RPCHelpMan{
        "getstakinginfo",
        "Returns an object containing staking status.",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "Staking status object",
            {
                {RPCResult::Type::BOOL, "enabled", "If staking is enabled"},
                {RPCResult::Type::BOOL, "staking", "If staking is active"},
                {RPCResult::Type::STR, "errors", "Any staking errors"},
                {RPCResult::Type::NUM, "currentblocksize", "Current block size"},
                {RPCResult::Type::NUM, "currentblocktx", "Current block tx count"},
                {RPCResult::Type::NUM, "pooledtx", "Pooled tx count"},
                {RPCResult::Type::NUM, "difficulty", "Staking difficulty"},
                {RPCResult::Type::NUM, "search-interval", "Search interval"},
                {RPCResult::Type::NUM, "weight", "Wallet staking weight"},
                {RPCResult::Type::NUM, "effectiveweight", "Wallet effective staking weight (coin-time adjusted)"},
                {RPCResult::Type::NUM, "netstakeweight", "Network staking weight"},
                {RPCResult::Type::NUM, "expectedtime", "Expected time to stake"},
                {RPCResult::Type::NUM, "eligibleutxos", "Number of wallet outputs currently eligible for staking"},
                {RPCResult::Type::NUM, "coindayweight", "Sum of wallet coin-day weights used by kernel checks"},
                {RPCResult::Type::STR, "stakenextbits", "Compact target (hex) for the next proof-of-stake block"},
                {RPCResult::Type::NUM, "kernelrate", "Estimated kernel checks per second"},
                {RPCResult::Type::NUM, "expectedtimekernel", "Kernel-based expected time to stake (seconds)"},
                {RPCResult::Type::NUM, "probnohit1h", "Estimated probability of finding no stake within 1 hour"},
                {RPCResult::Type::NUM, "probnohit4h", "Estimated probability of finding no stake within 4 hours"},
                {RPCResult::Type::NUM, "probnohit8h", "Estimated probability of finding no stake within 8 hours"},
                {RPCResult::Type::NUM, "probnohit12h", "Estimated probability of finding no stake within 12 hours"},
            }
        },
        RPCExamples{
            "getstakinginfo"
        },
        [](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            WalletContext& context = EnsureWalletContext(request.context);
            if (!context.chain) {
                throw JSONRPCError(RPC_INTERNAL_ERROR, "Chain interface not found");
            }
            ChainstateManager& chainman = context.chain->chainman();

            std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
            CWallet* const pwallet = wallet.get();

            bool enabled = gArgs.GetBoolArg("-minting", true) && gArgs.GetBoolArg("-staking", true);
            int64_t wallet_weight{0};
            int64_t wallet_effective_weight{0};
            int64_t eligible_utxos{0};
            int64_t next_pos_bits{0};
            double wallet_coin_day_weight{0.0};
            bool wallet_unlocked{false};
            bool syncing{false};
            double difficulty{0.0};
            {
                LOCK2(pwallet->cs_wallet, cs_main);
                const CBlockIndex* const tip = chainman.ActiveChain().Tip();
                const CBlockIndex* const last_pos = GetLastBlockIndex(tip, true);
                difficulty = last_pos ? GetDifficulty(last_pos, tip) : 0.0;
                next_pos_bits = tip ? GetNextTargetRequired(tip, /*fProofOfStake=*/true, Params().GetConsensus()) : 0;
                wallet_unlocked = !pwallet->IsLocked();
                syncing = chainman.ActiveChainstate().IsInitialBlockDownload();

                const int64_t now = TicksSinceEpoch<std::chrono::seconds>(GetAdjustedTime());
                const int64_t stake_min_age = Params().GetConsensus().nStakeMinAge;
                const int64_t stake_max_age = Params().GetConsensus().nStakeMaxAge;
                const bool protocol_v03 = IsProtocolV03(now);

                std::unique_ptr<interfaces::Wallet> iwallet = interfaces::MakeWallet(context, wallet);
                const auto& wallet_txs = iwallet->getWalletTxs();
                for (const auto& wtx : wallet_txs) {
                    for (const auto& kr : KernelRecord::decomposeOutput(*iwallet, wtx)) {
                        if (!kr.spent && (now - kr.nTime) >= stake_min_age) {
                            ++eligible_utxos;
                            wallet_weight += kr.nValue;
                            const int64_t time_weight = std::max<int64_t>(0, (now - kr.nTime) - stake_min_age);
                            wallet_effective_weight += (kr.nValue * time_weight) / (24 * 60 * 60);

                            // Mirror kernel.cpp coin-day weight calculation so RPC reports realistic hit odds.
                            const int64_t kernel_time_weight = std::min<int64_t>(now - kr.nTime, stake_max_age) - (protocol_v03 ? stake_min_age : 0);
                            if (kernel_time_weight > 0) {
                                wallet_coin_day_weight += (static_cast<double>(kr.nValue) / static_cast<double>(COIN))
                                    * (static_cast<double>(kernel_time_weight) / static_cast<double>(24 * 60 * 60));
                            }
                        }
                    }
                }
            }

            const int64_t net_stake_weight = difficulty > 0.0 ? static_cast<int64_t>(difficulty * static_cast<double>(uint64_t{1} << 32)) : 0;
            const bool staking_active = enabled && wallet_unlocked && wallet_weight > 0 && !syncing;

            int64_t expected_time{0};
            if (staking_active && wallet_effective_weight > 0 && net_stake_weight > 0) {
                expected_time = static_cast<int64_t>((static_cast<double>(net_stake_weight) / static_cast<double>(wallet_effective_weight)) * Params().GetConsensus().nStakeTargetSpacing);
            }

            double kernel_rate{0.0};
            int64_t expected_time_kernel{0};
            double prob_nohit_1h{1.0};
            double prob_nohit_4h{1.0};
            double prob_nohit_8h{1.0};
            double prob_nohit_12h{1.0};

            if (staking_active && wallet_coin_day_weight > 0.0 && next_pos_bits > 0 && nLastCoinStakeSearchInterval > 0) {
                const uint32_t nbits_u32 = static_cast<uint32_t>(next_pos_bits);
                const uint32_t exponent = nbits_u32 >> 24;
                const uint32_t mantissa = nbits_u32 & 0x007fffff;
                const bool negative = (nbits_u32 & 0x00800000) != 0;
                if (!negative && mantissa > 0) {
                    const int shift = static_cast<int>(8 * (exponent - 3)) - 256;
                    const long double target_per_coin_day_prob = std::ldexp(static_cast<long double>(mantissa), shift);
                    const double search_interval = static_cast<double>(nLastCoinStakeSearchInterval);
                    const long double lambda = (target_per_coin_day_prob * static_cast<long double>(wallet_coin_day_weight)) / static_cast<long double>(search_interval);

                    kernel_rate = static_cast<double>(static_cast<long double>(eligible_utxos) / search_interval);
                    if (lambda > 0.0L) {
                        expected_time_kernel = static_cast<int64_t>(1.0L / lambda);
                        prob_nohit_1h = std::exp(-static_cast<double>(lambda * 3600.0L));
                        prob_nohit_4h = std::exp(-static_cast<double>(lambda * (4.0L * 3600.0L)));
                        prob_nohit_8h = std::exp(-static_cast<double>(lambda * (8.0L * 3600.0L)));
                        prob_nohit_12h = std::exp(-static_cast<double>(lambda * (12.0L * 3600.0L)));
                    }
                }
            }

            uint64_t pooled_tx{0};
            if (CTxMemPool* mempool = chainman.ActiveChainstate().GetMempool()) {
                LOCK(mempool->cs);
                pooled_tx = static_cast<uint64_t>(mempool->size());
            }

            UniValue obj(UniValue::VOBJ);
            obj.pushKV("enabled", enabled);
            obj.pushKV("staking", staking_active);
            obj.pushKV("errors", strMintWarning);
            obj.pushKV("currentblocksize", BlockAssembler::m_last_block_weight.value_or(0));
            obj.pushKV("currentblocktx", BlockAssembler::m_last_block_num_txs.value_or(0));
            obj.pushKV("pooledtx", pooled_tx);
            obj.pushKV("difficulty", difficulty);
            obj.pushKV("search-interval", static_cast<int>(nLastCoinStakeSearchInterval));
            obj.pushKV("weight", wallet_weight);
            obj.pushKV("effectiveweight", wallet_effective_weight);
            obj.pushKV("netstakeweight", net_stake_weight);
            obj.pushKV("expectedtime", expected_time);
            obj.pushKV("eligibleutxos", eligible_utxos);
            obj.pushKV("coindayweight", wallet_coin_day_weight);
            obj.pushKV("stakenextbits", strprintf("%08x", static_cast<uint32_t>(next_pos_bits)));
            obj.pushKV("kernelrate", kernel_rate);
            obj.pushKV("expectedtimekernel", expected_time_kernel);
            obj.pushKV("probnohit1h", prob_nohit_1h);
            obj.pushKV("probnohit4h", prob_nohit_4h);
            obj.pushKV("probnohit8h", prob_nohit_8h);
            obj.pushKV("probnohit12h", prob_nohit_12h);
            return obj;
        }
    };
}


