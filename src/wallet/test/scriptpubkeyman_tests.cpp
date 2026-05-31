// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <key_io.h>
#include <script/descriptor.h>
#include <script/standard.h>
#include <test/util/setup_common.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/wallet.h>

#include <boost/test/unit_test.hpp>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(scriptpubkeyman_tests, BasicTestingSetup)

// Test LegacyScriptPubKeyMan::CanProvide behavior, making sure it returns true
// for recognized scripts even when keys may not be available for signing.
BOOST_AUTO_TEST_CASE(CanProvide)
{
    // Set up wallet and keyman variables.
    CWallet wallet(m_node.chain.get(), "", CreateDummyWalletDatabase());
    LegacyScriptPubKeyMan& keyman = *wallet.GetOrCreateLegacyScriptPubKeyMan();

    // Make a 1 of 2 multisig script
    std::vector<CKey> keys(2);
    std::vector<CPubKey> pubkeys;
    for (CKey& key : keys) {
        key.MakeNewKey(true);
        pubkeys.emplace_back(key.GetPubKey());
    }
    CScript multisig_script = GetScriptForMultisig(1, pubkeys);
    CScript p2sh_script = GetScriptForDestination(ScriptHash(multisig_script));
    SignatureData data;

    // Verify the p2sh(multisig) script is not recognized until the multisig
    // script is added to the keystore to make it solvable
    BOOST_CHECK(!keyman.CanProvide(p2sh_script, data));
    keyman.AddCScript(multisig_script);
    BOOST_CHECK(keyman.CanProvide(p2sh_script, data));
}

BOOST_AUTO_TEST_CASE(DescriptorMainnetCoinType1701)
{
    CWallet wallet(m_node.chain.get(), "", CreateDummyWalletDatabase());
    wallet.LoadWallet();

    LOCK(wallet.cs_wallet);
    wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    wallet.SetupDescriptorScriptPubKeyMans();

    auto* ext_legacy_spkm = dynamic_cast<DescriptorScriptPubKeyMan*>(wallet.GetScriptPubKeyMan(OutputType::LEGACY, /*internal=*/false));
    auto* int_legacy_spkm = dynamic_cast<DescriptorScriptPubKeyMan*>(wallet.GetScriptPubKeyMan(OutputType::LEGACY, /*internal=*/true));

    BOOST_REQUIRE(ext_legacy_spkm);
    BOOST_REQUIRE(int_legacy_spkm);

    std::string ext_desc;
    std::string int_desc;
    BOOST_REQUIRE(ext_legacy_spkm->GetDescriptorString(ext_desc, /*priv=*/false));
    BOOST_REQUIRE(int_legacy_spkm->GetDescriptorString(int_desc, /*priv=*/false));

    BOOST_CHECK_NE(ext_desc.find("/44'/1701'/0'/0/*"), std::string::npos);
    BOOST_CHECK_NE(int_desc.find("/44'/1701'/0'/1/*"), std::string::npos);
}

BOOST_AUTO_TEST_CASE(DescriptorCoinType1701DiffersFrom44h6h)
{
    CWallet wallet(m_node.chain.get(), "", CreateDummyWalletDatabase());
    wallet.LoadWallet();

    LOCK(wallet.cs_wallet);
    wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    wallet.SetupDescriptorScriptPubKeyMans();

    auto* ext_legacy_spkm = dynamic_cast<DescriptorScriptPubKeyMan*>(wallet.GetScriptPubKeyMan(OutputType::LEGACY, /*internal=*/false));
    BOOST_REQUIRE(ext_legacy_spkm);

    std::string desc_1701;
    BOOST_REQUIRE(ext_legacy_spkm->GetDescriptorString(desc_1701, /*priv=*/false));
    BOOST_REQUIRE_NE(desc_1701.find("/44'/1701'"), std::string::npos);

    std::string desc_6 = desc_1701;
    desc_6.replace(desc_6.find("/44'/1701'"), std::string("/44'/1701'").size(), "/44'/6'");

    FlatSigningProvider provider_1701;
    FlatSigningProvider provider_6;
    std::string error;
    std::unique_ptr<Descriptor> parsed_1701 = Parse(desc_1701, provider_1701, error, /*require_checksum=*/false);
    BOOST_REQUIRE(parsed_1701);
    std::unique_ptr<Descriptor> parsed_6 = Parse(desc_6, provider_6, error, /*require_checksum=*/false);
    BOOST_REQUIRE(parsed_6);

    std::vector<CScript> scripts_1701;
    std::vector<CScript> scripts_6;
    FlatSigningProvider out_1701;
    FlatSigningProvider out_6;
    BOOST_REQUIRE(parsed_1701->Expand(0, provider_1701, scripts_1701, out_1701));
    BOOST_REQUIRE(parsed_6->Expand(0, provider_6, scripts_6, out_6));
    BOOST_REQUIRE_EQUAL(scripts_1701.size(), 1U);
    BOOST_REQUIRE_EQUAL(scripts_6.size(), 1U);

    BOOST_CHECK(scripts_1701[0] != scripts_6[0]);

    CTxDestination dest_1701;
    CTxDestination dest_6;
    BOOST_REQUIRE(ExtractDestination(scripts_1701[0], dest_1701));
    BOOST_REQUIRE(ExtractDestination(scripts_6[0], dest_6));
    BOOST_CHECK(EncodeDestination(dest_1701) != EncodeDestination(dest_6));
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
