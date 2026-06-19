#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test consistency between getnetworkhashps and getnetworkghps

This test mines a number of blocks, then verifies that
`getnetworkhashps()` (H/s) is consistent with `getnetworkghps()` (GH/s)
within a reasonable tolerance.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


class NetworkHashConsistencyTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        pass

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)

        # Mine some blocks so both RPCs have data to operate on
        self.generatetoaddress(node, 120, node.get_deterministic_priv_key().address)

        hps = node.getnetworkhashps()
        ghps = node.getnetworkghps()

        # Basic sanity
        assert hps is not None and ghps is not None
        assert hps > 0
        assert ghps > 0

        ghps_to_hps = ghps * 1e9

        # Allow method differences; require relative agreement within 20% (but at least 5% precision on small values)
        rel_tol = 0.20
        min_rel = 0.05
        diff = abs(hps - ghps_to_hps)
        max_allowed = rel_tol * max(hps, ghps_to_hps)
        min_allowed = min_rel * max(hps, ghps_to_hps)

        # Accept if within rel_tol, otherwise fail. Use min_allowed for small values.
        assert diff <= max(max_allowed, min_allowed), f"getnetworkhashps ({hps}) and getnetworkghps ({ghps} GH/s -> {ghps_to_hps} H/s) differ by {diff} > {max(max_allowed, min_allowed)}"


if __name__ == '__main__':
    NetworkHashConsistencyTest().main()
