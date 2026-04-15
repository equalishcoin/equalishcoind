


#include "staking.h"
#include <univalue.h>
#include <rpc/server.h>

static RPCHelpMan getstakinginfo()
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
                {RPCResult::Type::NUM, "netstakeweight", "Network staking weight"},
                {RPCResult::Type::NUM, "expectedtime", "Expected time to stake"},
            }
        },
        RPCExamples{
            "getstakinginfo"
        },
        [](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            UniValue obj(UniValue::VOBJ);
            obj.pushKV("enabled", true); // TODO: wire real status
            obj.pushKV("staking", false); // TODO: wire real status
            obj.pushKV("errors", "");
            obj.pushKV("currentblocksize", 0);
            obj.pushKV("currentblocktx", 0);
            obj.pushKV("pooledtx", 0);
            obj.pushKV("difficulty", 0);
            obj.pushKV("search-interval", 0);
            obj.pushKV("weight", 0);
            obj.pushKV("netstakeweight", 0);
            obj.pushKV("expectedtime", 0);
            return obj;
        }
    };
}

static const CRPCCommand commands[] = {
    CRPCCommand{"wallet", &getstakinginfo},
};

void RegisterStakingRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < std::size(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
