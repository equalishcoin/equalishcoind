// Copyright (c) 2017-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <index/txindex.h>
#include <interfaces/chain.h>
#include <key_io.h>
#include <policy/packages.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/standard.h>
#include <test/util/setup_common.h>
#include <util/time.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_SUITE(txvalidation_tests)

/**
 * Ensure that the mempool won't accept coinbase transactions.
 */
BOOST_FIXTURE_TEST_CASE(tx_mempool_reject_coinbase, TestChain100Setup)
{
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    CMutableTransaction coinbaseTx;

    coinbaseTx.nVersion = 1;
    coinbaseTx.vin.resize(1);
    coinbaseTx.vout.resize(1);
    coinbaseTx.vin[0].scriptSig = CScript() << OP_11 << OP_EQUAL;
    coinbaseTx.vout[0].nValue = 10000 * CENT;
    coinbaseTx.vout[0].scriptPubKey = scriptPubKey;

    BOOST_CHECK(CTransaction(coinbaseTx).IsCoinBase());

    LOCK(cs_main);

    unsigned int initialPoolSize = m_node.mempool->size();
    const MempoolAcceptResult result = m_node.chainman->ProcessTransaction(MakeTransactionRef(coinbaseTx));

    BOOST_CHECK(result.m_result_type == MempoolAcceptResult::ResultType::INVALID);

    // Check that the transaction hasn't been added to mempool.
    BOOST_CHECK_EQUAL(m_node.mempool->size(), initialPoolSize);

    // Check that the validation state reflects the unsuccessful attempt.
    BOOST_CHECK(result.m_state.IsInvalid());
    BOOST_CHECK_EQUAL(result.m_state.GetRejectReason(), "coinbase");
    BOOST_CHECK(result.m_state.GetResult() == TxValidationResult::TX_CONSENSUS);
}

BOOST_FIXTURE_TEST_CASE(tx_validate_coinstake_reward_split, TestChain100Setup)
{
    BOOST_REQUIRE(!g_txindex);
    g_txindex = std::make_unique<TxIndex>(interfaces::MakeChain(m_node), 1 << 20, true);
    BOOST_REQUIRE(g_txindex->Start());

    constexpr int64_t timeout_ms = 10 * 1000;
    const int64_t time_start = GetTimeMillis();
    while (!g_txindex->BlockUntilSyncedToCurrentChain()) {
        BOOST_REQUIRE(time_start + timeout_ms > GetTimeMillis());
        UninterruptibleSleep(std::chrono::milliseconds{100});
    }

    LOCK(cs_main);

    const CBlockIndex* tip = m_node.chainman->ActiveChain().Tip();
    const CTransactionRef& prev_tx = m_coinbase_txns[0];

    CMutableTransaction stake_tx;
    stake_tx.nVersion = 1;
    stake_tx.nTime = prev_tx->nTime + Params().GetConsensus().nStakeMinAge + 86400;
    stake_tx.vin.resize(1);
    stake_tx.vin[0].prevout = COutPoint(prev_tx->GetHash(), 0);
    stake_tx.vin[0].scriptSig = CScript() << OP_0;
    stake_tx.vout.resize(2);
    stake_tx.vout[0].SetEmpty();
    stake_tx.vout[1].scriptPubKey = GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()));

    uint64_t coin_age{0};
    const CCoinsViewCache& coins_tip = m_node.chainman->ActiveChainstate().CoinsTip();
    BOOST_REQUIRE(GetCoinAge(CTransaction(stake_tx), coins_tip, coin_age, stake_tx.nTime));

    const CAmount money_supply = tip->nMoneySupply;
    const CAmount reward_limit = GetProofOfStakeReward(static_cast<int64_t>(coin_age), stake_tx.nTime, money_supply);
    const CAmount spendable_reward = SplitIssuanceBudget(Params().GetConsensus(), reward_limit).pos_budget;
    BOOST_REQUIRE(spendable_reward > 0);

    stake_tx.vout[1].nValue = prev_tx->vout[0].nValue + spendable_reward;

    CAmount tx_fee{};
    TxValidationState valid_state;
    BOOST_CHECK(Consensus::CheckTxInputs(CTransaction(stake_tx), valid_state, coins_tip, tip->nHeight + 1, tx_fee, Params().GetConsensus(), stake_tx.nTime, money_supply));
    BOOST_CHECK(valid_state.IsValid());
    BOOST_CHECK_EQUAL(tx_fee, 0);

    stake_tx.vout[1].nValue = prev_tx->vout[0].nValue + spendable_reward + 1;

    TxValidationState invalid_state;
    BOOST_CHECK(!Consensus::CheckTxInputs(CTransaction(stake_tx), invalid_state, coins_tip, tip->nHeight + 1, tx_fee, Params().GetConsensus(), stake_tx.nTime, money_supply));
    BOOST_CHECK_EQUAL(invalid_state.GetRejectReason(), "bad-txns-coinstake-too-large");

    SyncWithValidationInterfaceQueue();
    g_txindex->Stop();
    g_txindex.reset();
}
BOOST_AUTO_TEST_SUITE_END()
