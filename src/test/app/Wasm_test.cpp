//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012, 2013 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#include <test/jtx.h>
#include <xrpld/app/misc/WasmVM.h>

namespace ripple {
namespace test {

extern std::string const p0Hex;
extern std::string const p1Hex;
extern std::string const p2Hex;
extern std::string const p4Hex;
extern std::string const p5Hex;

struct Wasm_test : public beast::unit_test::suite
{
    void
    testEscrowWasmP0()
    {
        testcase("escrow wasm P0 test");

        auto const wasmStr = boost::algorithm::unhex(p0Hex);
        std::vector<uint8_t> const wasm(wasmStr.begin(), wasmStr.end());
        std::string const funcName("mock_escrow");

        auto re = runEscrowWasm(wasm, funcName, 15);
        if (BEAST_EXPECT(re.has_value()))
            BEAST_EXPECT(re.value());

        re = runEscrowWasm(wasm, funcName, 11);
        if (BEAST_EXPECT(re.has_value()))
            BEAST_EXPECT(!re.value());
    }

    void
    testBadWasm()
    {
        testcase("bad wasm test");
        std::string const wasmHex = "00000000";
        auto wasmStr = boost::algorithm::unhex(wasmHex);
        std::vector<uint8_t> wasm(wasmStr.begin(), wasmStr.end());
        std::string funcName("mock_escrow");
        auto re = runEscrowWasm(wasm, funcName, 15);
        BEAST_EXPECT(re.error());
    }

    void
    testEscrowWasmP1()
    {
        testcase("escrow wasm P1 test");

        auto const wasmStr = boost::algorithm::unhex(p1Hex);
        std::vector<uint8_t> const wasm(wasmStr.begin(), wasmStr.end());
        std::string const funcName("check_accountID");
        {
            std::string str = "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh";
            std::vector<uint8_t> data(str.begin(), str.end());
            auto re = runEscrowWasm(wasm, funcName, data);
            if (BEAST_EXPECT(re.has_value()))
                BEAST_EXPECT(re.value());
        }
        {
            std::string str = "rHb9CJAWyB4rj91VRWn96DkukG4bwdty00";
            std::vector<uint8_t> data(str.begin(), str.end());
            auto re = runEscrowWasm(wasm, funcName, data);
            if (BEAST_EXPECT(re.has_value()))
                BEAST_EXPECT(!re.value());
        }
    }

    void
    testEscrowWasmP2P3()
    {
        testcase("escrow wasm P2 & P3 test");

        auto wasmStr = boost::algorithm::unhex(p2Hex);
        std::vector<uint8_t> wasm(wasmStr.begin(), wasmStr.end());
        std::string funcName("compare_accountID");

        std::string escrow_tx_json_str = R"({
           "Account" : "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh",
           "Fee" : "10",
           "Flags" : 2147483648,
           "OfferSequence" : 2,
           "Owner" : "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh",
           "Sequence" : 3,
           "SigningPubKey" : "0330E7FC9D56BB25D6893BA3F317AE5BCF33B3291BD63DB32654A313222F7FD020",
           "TransactionType" : "EscrowFinish",
           "TxnSignature" : "30450221008AD5EE48F7F1047813E79C174FE401D023A4B4A7B99AF826E081DB1DFF7B9C510220133F05B7FD3D7D7F163E8C77EE0A49D02619AB6C77CC3487D0095C9B34033C1C",
           "hash" : "74465121372813CBA4C77E31F12E137163F5B2509B16AC1703ECF0DA194B2DD4"
        })";

        std::vector<uint8_t> escrow_tx_json_data(
            escrow_tx_json_str.begin(), escrow_tx_json_str.end());
        {
            std::string escrow_lo_json_str = R"({
           "Account" : "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh",
           "Amount" : "100000",
           "CancelAfter" : 790297421,
           "Destination" : "rBYn44yhs8cf8G2t79XMUHYQpp2ayhqwcw",
           "DestinationNode" : "0",
           "FinishAfter" : 790297403,
           "FinishFunction" : "0061736D0100000001180460027F7F0060017F017F60027F7F017F60047F7F7F7F00030C0B01010200000000000003000405017001030305030100110619037F01418080C0000B7F0041DD85C0000B7F0041E085C0000B074205066D656D6F7279020008616C6C6F6361746500000F636865636B5F6163636F756E74494400020A5F5F646174615F656E6403010B5F5F686561705F6261736503020908010041010B02060A0AF5360B610002",
           "Flags" : 0,
           "LedgerEntryType" : "Escrow",
           "OwnerNode" : "0",
           "PreviousTxnID" : "CF25D1C6B8E637C7DAC61B586F820A16896A3090D9F6FBF9FA00D8B13A265647",
           "PreviousTxnLgrSeq" : 4,
           "index" : "9BC6631F3EC761CF9BD846D006560E2D57B0A5C91D4570AEB209645B189A702F"
        })";

            std::vector<uint8_t> const escrow_lo_json_data(
                escrow_lo_json_str.begin(), escrow_lo_json_str.end());
            auto re = runEscrowWasm(
                wasm, funcName, escrow_tx_json_data, escrow_lo_json_data);
            if (BEAST_EXPECT(re.has_value()))
                BEAST_EXPECT(re.value());
        }

        {
            std::string escrow_lo_json_str = R"({
           "Account" : "rHb9CJAWyB4rj91VRWn96DkukG4bwdty00",
           "Amount" : "100000",
           "CancelAfter" : 790297421,
           "Destination" : "rBYn44yhs8cf8G2t79XMUHYQpp2ayhqwcw",
           "DestinationNode" : "0",
           "FinishAfter" : 790297403,
           "FinishFunction" : "0061736D0100000001180460027F7F0060017F017F60027F7F017F60047F7F7F7F00030C0B01010200000000000003000405017001030305030100110619037F01418080C0000B7F0041DD85C0000B7F0041E085C0000B074205066D656D6F7279020008616C6C6F6361746500000F636865636B5F6163636F756E74494400020A5F5F646174615F656E6403010B5F5F686561705F6261736503020908010041010B02060A0AF5360B610002",
           "Flags" : 0,
           "LedgerEntryType" : "Escrow",
           "OwnerNode" : "0",
           "PreviousTxnID" : "CF25D1C6B8E637C7DAC61B586F820A16896A3090D9F6FBF9FA00D8B13A265647",
           "PreviousTxnLgrSeq" : 4,
           "index" : "9BC6631F3EC761CF9BD846D006560E2D57B0A5C91D4570AEB209645B189A702F"
        })";

            std::vector<uint8_t> escrow_lo_json_data(
                escrow_lo_json_str.begin(), escrow_lo_json_str.end());
            auto re = runEscrowWasm(
                wasm, funcName, escrow_tx_json_data, escrow_lo_json_data);
            if (BEAST_EXPECT(re.has_value()))
                BEAST_EXPECT(!re.value());
        }
    }

    void
    testEscrowWasmP4()
    {
        testcase("escrow wasm P4 test");

        auto wasmStr = boost::algorithm::unhex(p4Hex);
        std::vector<uint8_t> wasm(wasmStr.begin(), wasmStr.end());
        std::string funcName("compare_accountID");

        std::string escrow_tx_json_str = R"({
           "Account" : "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh",
           "Fee" : "10",
           "Flags" : 2147483648,
           "OfferSequence" : 2,
           "Owner" : "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh",
           "Sequence" : 3,
           "SigningPubKey" : "0330E7FC9D56BB25D6893BA3F317AE5BCF33B3291BD63DB32654A313222F7FD020",
           "TransactionType" : "EscrowFinish",
           "TxnSignature" : "30450221008AD5EE48F7F1047813E79C174FE401D023A4B4A7B99AF826E081DB1DFF7B9C510220133F05B7FD3D7D7F163E8C77EE0A49D02619AB6C77CC3487D0095C9B34033C1C",
           "hash" : "74465121372813CBA4C77E31F12E137163F5B2509B16AC1703ECF0DA194B2DD4"
        })";

        std::vector<uint8_t> escrow_tx_json_data(
            escrow_tx_json_str.begin(), escrow_tx_json_str.end());

        {
            std::string escrow_lo_json_str = R"({
               "Account" : "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh",
               "Amount" : "100000",
               "CancelAfter" : 790297421,
               "Destination" : "rBYn44yhs8cf8G2t79XMUHYQpp2ayhqwcw",
               "DestinationNode" : "0",
               "FinishAfter" : 790297403,
               "FinishFunction" : "0061736D0100000001180460027F7F0060017F017F60027F7F017F60047F7F7F7F00030C0B01010200000000000003000405017001030305030100110619037F01418080C0000B7F0041DD85C0000B7F0041E085C0000B074205066D656D6F7279020008616C6C6F6361746500000F636865636B5F6163636F756E74494400020A5F5F646174615F656E6403010B5F5F686561705F6261736503020908010041010B02060A0AF5360B610002",
               "Flags" : 0,
               "LedgerEntryType" : "Escrow",
               "OwnerNode" : "0",
               "PreviousTxnID" : "CF25D1C6B8E637C7DAC61B586F820A16896A3090D9F6FBF9FA00D8B13A265647",
               "PreviousTxnLgrSeq" : 4,
               "index" : "9BC6631F3EC761CF9BD846D006560E2D57B0A5C91D4570AEB209645B189A702F",
               "Data" : "02"
            })";

            std::vector<uint8_t> const escrow_lo_json_data(
                escrow_lo_json_str.begin(), escrow_lo_json_str.end());

            auto re = runEscrowWasmP4(
                wasm, funcName, escrow_tx_json_data, escrow_lo_json_data);
            if (BEAST_EXPECT(re.has_value()))
            {
                auto reValue = re.value();
                //                std::cout << reValue.first << " " <<
                //                reValue.second
                //                          << std::endl;
                BEAST_EXPECT(!reValue.first);
                BEAST_EXPECT(reValue.second == "1");
            }
        }

        {
            std::string escrow_lo_json_str = R"({
               "Account" : "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh",
               "Amount" : "100000",
               "CancelAfter" : 790297421,
               "Destination" : "rBYn44yhs8cf8G2t79XMUHYQpp2ayhqwcw",
               "DestinationNode" : "0",
               "FinishAfter" : 790297403,
               "FinishFunction" : "0061736D0100000001180460027F7F0060017F017F60027F7F017F60047F7F7F7F00030C0B01010200000000000003000405017001030305030100110619037F01418080C0000B7F0041DD85C0000B7F0041E085C0000B074205066D656D6F7279020008616C6C6F6361746500000F636865636B5F6163636F756E74494400020A5F5F646174615F656E6403010B5F5F686561705F6261736503020908010041010B02060A0AF5360B610002",
               "Flags" : 0,
               "LedgerEntryType" : "Escrow",
               "OwnerNode" : "0",
               "PreviousTxnID" : "CF25D1C6B8E637C7DAC61B586F820A16896A3090D9F6FBF9FA00D8B13A265647",
               "PreviousTxnLgrSeq" : 4,
               "index" : "9BC6631F3EC761CF9BD846D006560E2D57B0A5C91D4570AEB209645B189A702F",
               "Data" : "1"
            })";

            std::vector<uint8_t> const escrow_lo_json_data(
                escrow_lo_json_str.begin(), escrow_lo_json_str.end());
            auto re = runEscrowWasmP4(
                wasm, funcName, escrow_tx_json_data, escrow_lo_json_data);
            if (BEAST_EXPECT(re.has_value()))
            {
                auto reValue = re.value();
                //                std::cout << reValue.first << " " <<
                //                reValue.second
                //                          << std::endl;
                BEAST_EXPECT(reValue.first);
                BEAST_EXPECT(reValue.second == "0");
            }
        }
    }

    void
    testEscrowWasmP5()
    {
        testcase("escrow wasm P5 test");

        auto wasmStr = boost::algorithm::unhex(p5Hex);
        std::vector<uint8_t> wasm(wasmStr.begin(), wasmStr.end());

        using namespace test::jtx;
        struct TestLedgerDataProvider : public LedgerDataProvider
        {
            Env* env;

        public:
            TestLedgerDataProvider(Env* env) : env(env)
            {
            }

            int32_t
            get_ledger_sqn() override
            {
                return (int32_t)env->current()->seq();
            }
        };
        Env env{*this};
        TestLedgerDataProvider ledgerDataProvider(&env);
        std::string const funcName("ready");

        auto re = runEscrowWasm(wasm, funcName, &ledgerDataProvider);
        if (BEAST_EXPECT(re.has_value()))
            BEAST_EXPECT(!re.value());

        env.close();
        env.close();
        env.close();
        env.close();

        re = runEscrowWasm(wasm, funcName, &ledgerDataProvider);
        if (BEAST_EXPECT(re.has_value()))
            BEAST_EXPECT(re.value());
    }

    void
    run() override
    {
        using namespace test::jtx;

        for (int i = 0; i < static_cast<int>(wasmEngines::END); ++i)
        {
            setWasmEngine(static_cast<wasmEngines>(i));
            std::cout << "===========\nEngine: "
                      << engineName(static_cast<wasmEngines>(i))
                      << "\n===========" << std::endl;
            testEscrowWasmP0();
            testBadWasm();
            testEscrowWasmP1();
            testEscrowWasmP2P3();
            testEscrowWasmP4();
            testEscrowWasmP5();
        }
    }
};

inline uint64_t
usecs()
{
    uint64_t x =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
            .count();
    return x;
}

class WasmPerf_test : public beast::unit_test::suite
{
    static const int TESTS_N = 20;
    static const int ENGINES_N = wasmEngines::END;
    static const int ADD_MOD_N = 1000;
    static const int FIB_N = 5;

    // std::vector<std::unique_ptr<WasmEngine>> engines;

    // testcase, engine, iteration
    std::vector<std::vector<std::vector<std::uint64_t>>> testTimes;

    // void
    // initEngines()
    // {
    //     for (int i = wasmEngines::Edge; i < wasmEngines::END; ++i)
    //     {
    //         setWasmEngine(static_cast<wasmEngines>(i));
    //         engines.emplace_back(WasmEngine::instance());
    //     }
    // }

    // return module idx, also create instance 0 (and return 0)
    int
    addModule(WasmEngine& e, vbytes const& binWasm)
    {
        return e.addModule(binWasm);
    }

    // return instance idx
    int
    addInstance(WasmEngine& e, int m)
    {
        return e.addInstance(m);
    }

    void
    ptest_0_AddModule(wasmEngines ei, WasmEngine& e)
    {
        auto const wasmStr = boost::algorithm::unhex(p4Hex);
        vbytes const wasm(wasmStr.begin(), wasmStr.end());

        std::cout << std::endl;
        testcase(
            std::string(engineName(static_cast<wasmEngines>(ei))) +
            " PerfTest 0, module load, size(" + std::to_string(wasm.size()) +
            ")");

        auto& times(testTimes[0][ei]);
        // times.resize(ADD_MOD_N + 1);

        times[0] = usecs();
        for (int i = 0; i < ADD_MOD_N; ++i)
        {
            e.addModule(wasm);
            times[i + 1] = usecs();
        }

        BEAST_EXPECT(times[ADD_MOD_N] > 0);
    }

    void
    ptest_1_AddInstance(wasmEngines ei, WasmEngine& e)
    {
        auto const wasmStr = boost::algorithm::unhex(p4Hex);
        vbytes const wasm(wasmStr.begin(), wasmStr.end());

        std::cout << std::endl;
        testcase(
            std::string(engineName(static_cast<wasmEngines>(ei))) +
            " PerfTest 1, Add instance, size(" + std::to_string(wasm.size()) +
            ")");

        auto& times(testTimes[1][ei]);
        // times.resize(ADD_MOD_N + 1);

        int k = 0;
        e.addModule(wasm);
        times[0] = usecs();
        for (int i = 0; i < ADD_MOD_N; ++i)
        {
            k = e.addInstance(0);
            times[i + 1] = usecs();
        }

        BEAST_EXPECT(times[ADD_MOD_N] > 0);
        BEAST_EXPECT(k == ADD_MOD_N + 1);
    }

    void
    ptest_2_RunP4(wasmEngines ei, WasmEngine& e)
    {
        auto const wasmStr = boost::algorithm::unhex(p4Hex);
        vbytes const wasm(wasmStr.begin(), wasmStr.end());

        std::string funcName("compare_accountID");
        std::string escrow_tx_json_str = R"({
           "Account" : "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh",
           "Fee" : "10",
           "Flags" : 2147483648,
           "OfferSequence" : 2,
           "Owner" : "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh",
           "Sequence" : 3,
           "SigningPubKey" : "0330E7FC9D56BB25D6893BA3F317AE5BCF33B3291BD63DB32654A313222F7FD020",
           "TransactionType" : "EscrowFinish",
           "TxnSignature" : "30450221008AD5EE48F7F1047813E79C174FE401D023A4B4A7B99AF826E081DB1DFF7B9C510220133F05B7FD3D7D7F163E8C77EE0A49D02619AB6C77CC3487D0095C9B34033C1C",
           "hash" : "74465121372813CBA4C77E31F12E137163F5B2509B16AC1703ECF0DA194B2DD4"
        })";
        std::string escrow_lo_json_str = R"({
               "Account" : "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh",
               "Amount" : "100000",
               "CancelAfter" : 790297421,
               "Destination" : "rBYn44yhs8cf8G2t79XMUHYQpp2ayhqwcw",
               "DestinationNode" : "0",
               "FinishAfter" : 790297403,
               "FinishFunction" : "0061736D0100000001180460027F7F0060017F017F60027F7F017F60047F7F7F7F00030C0B01010200000000000003000405017001030305030100110619037F01418080C0000B7F0041DD85C0000B7F0041E085C0000B074205066D656D6F7279020008616C6C6F6361746500000F636865636B5F6163636F756E74494400020A5F5F646174615F656E6403010B5F5F686561705F6261736503020908010041010B02060A0AF5360B610002",
               "Flags" : 0,
               "LedgerEntryType" : "Escrow",
               "OwnerNode" : "0",
               "PreviousTxnID" : "CF25D1C6B8E637C7DAC61B586F820A16896A3090D9F6FBF9FA00D8B13A265647",
               "PreviousTxnLgrSeq" : 4,
               "index" : "9BC6631F3EC761CF9BD846D006560E2D57B0A5C91D4570AEB209645B189A702F",
               "Data" : "02"
            })";

        std::vector<uint8_t> escrow_tx_json_data(
            escrow_tx_json_str.begin(), escrow_tx_json_str.end());
        std::vector<uint8_t> const escrow_lo_json_data(
            escrow_lo_json_str.begin(), escrow_lo_json_str.end());

        std::cout << std::endl;
        testcase(
            std::string(engineName(static_cast<wasmEngines>(ei))) +
            " PerfTest 2, runP4, size(" + std::to_string(wasm.size()) + ")");

        auto& times(testTimes[2][ei]);
        // times.resize(ADD_MOD_N + 1);

        times[0] = usecs();
        for (int i = 0; i < ADD_MOD_N; ++i)
        {
            auto const r = e.runP4(
                wasm, funcName, escrow_tx_json_data, escrow_lo_json_data);
            times[i + 1] = usecs();

            BEAST_EXPECT(r.value().second == "1");
        }

        BEAST_EXPECT(times[ADD_MOD_N] > 0);
    }

    void
    ptest_3_JustRunP4(wasmEngines ei, WasmEngine& e)
    {
        auto const wasmStr = boost::algorithm::unhex(p4Hex);
        vbytes const wasm(wasmStr.begin(), wasmStr.end());

        std::string funcName("compare_accountID");
        std::string escrow_tx_json_str = R"({
           "Account" : "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh",
           "Fee" : "10",
           "Flags" : 2147483648,
           "OfferSequence" : 2,
           "Owner" : "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh",
           "Sequence" : 3,
           "SigningPubKey" : "0330E7FC9D56BB25D6893BA3F317AE5BCF33B3291BD63DB32654A313222F7FD020",
           "TransactionType" : "EscrowFinish",
           "TxnSignature" : "30450221008AD5EE48F7F1047813E79C174FE401D023A4B4A7B99AF826E081DB1DFF7B9C510220133F05B7FD3D7D7F163E8C77EE0A49D02619AB6C77CC3487D0095C9B34033C1C",
           "hash" : "74465121372813CBA4C77E31F12E137163F5B2509B16AC1703ECF0DA194B2DD4"
        })";
        std::string escrow_lo_json_str = R"({
               "Account" : "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh",
               "Amount" : "100000",
               "CancelAfter" : 790297421,
               "Destination" : "rBYn44yhs8cf8G2t79XMUHYQpp2ayhqwcw",
               "DestinationNode" : "0",
               "FinishAfter" : 790297403,
               "FinishFunction" : "0061736D0100000001180460027F7F0060017F017F60027F7F017F60047F7F7F7F00030C0B01010200000000000003000405017001030305030100110619037F01418080C0000B7F0041DD85C0000B7F0041E085C0000B074205066D656D6F7279020008616C6C6F6361746500000F636865636B5F6163636F756E74494400020A5F5F646174615F656E6403010B5F5F686561705F6261736503020908010041010B02060A0AF5360B610002",
               "Flags" : 0,
               "LedgerEntryType" : "Escrow",
               "OwnerNode" : "0",
               "PreviousTxnID" : "CF25D1C6B8E637C7DAC61B586F820A16896A3090D9F6FBF9FA00D8B13A265647",
               "PreviousTxnLgrSeq" : 4,
               "index" : "9BC6631F3EC761CF9BD846D006560E2D57B0A5C91D4570AEB209645B189A702F",
               "Data" : "02"
            })";

        std::vector<uint8_t> escrow_tx_json_data(
            escrow_tx_json_str.begin(), escrow_tx_json_str.end());
        std::vector<uint8_t> const escrow_lo_json_data(
            escrow_lo_json_str.begin(), escrow_lo_json_str.end());

        std::cout << std::endl;
        testcase(
            std::string(engineName(static_cast<wasmEngines>(ei))) +
            " PerfTest 3, just runP4, size(" + std::to_string(wasm.size()) +
            ")");

        auto& times(testTimes[3][ei]);
        // times.resize(ADD_MOD_N + 1);
        e.addModule(wasm);

        times[0] = usecs();
        for (int i = 0; i < ADD_MOD_N; ++i)
        {
            auto const r = e.justRunP4(
                wasm, funcName, escrow_tx_json_data, escrow_lo_json_data);
            times[i + 1] = usecs();

            BEAST_EXPECT(r.value().second == "1");
        }

        BEAST_EXPECT(times[ADD_MOD_N] > 0);
    }

    void
    ptest_4_runFunc(wasmEngines ei, WasmEngine& e)
    {
        std::string const fibHex64 =
            "0061736d0100000001120460000060017f017e60017f0060017f017f0213"
            "0103656e760b73657454656d70526574300002030403000103071b02115f"
            "5f7761736d5f63616c6c5f63746f727300010366696200030a580302000b"
            "3f01017e200045044042000f0b2000410348044042010f0b200041026a21"
            "000340200041036b100220017c2101200041026b220041044a0d000b2001"
            "42017c0b1301017e200010022201422088a710002001a70b";

        std::string const fibHex =
            "0061736d0100000001090260000060017f017f0303020001071b02115f5f"
            "7761736d5f63616c6c5f63746f727300000366696200010a440202000b3f"
            "01017f200045044041000f0b2000410348044041010f0b200041026a2100"
            "0340200041036b100120016a2101200041026b220041044a0d000b200141"
            "016a0b";

        auto const wasmStr = boost::algorithm::unhex(fibHex);
        vbytes const wasm(wasmStr.begin(), wasmStr.end());
        std::string const funcName("fib");

        std::cout << std::endl;
        testcase(
            std::string(engineName(static_cast<wasmEngines>(ei))) +
            " PerfTest 4, run func, size(" + std::to_string(wasm.size()) + ")");

        auto& times(testTimes[4][ei]);
        e.addModule(wasm);

        times[0] = usecs();
        for (int i = 0; i < FIB_N; ++i)
        {
            auto const r = e.runFunc(funcName, 40);
            times[i + 1] = usecs();

            BEAST_EXPECT(r >= 0);
        }

        BEAST_EXPECT(times[FIB_N] > 0);
    }

    void
    ptest_Results()
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << std::endl;

        for (int i = 0; i < TESTS_N; ++i)
        {
            for (int j = 0; j < ENGINES_N; ++j)
            {
                auto const& vi(testTimes[i][j]);
                std::uint64_t avg;
                int imin, imax;
                avg = 0;
                imin = imax = 1;

                int k = 1;
                for (; (k < ADD_MOD_N + 1) && vi[k]; ++k)
                {
                    auto const x = vi[k];
                    auto const dn = x - vi[k - 1];
                    auto const dmin = vi[imin] - vi[imin - 1];
                    auto const dmax = vi[imax] - vi[imax - 1];
                    if (dmin > dn)
                        imin = k;
                    if (dmax < dn)
                        imax = k;
                    avg += dn;
                }

                if (--k <= 0)
                    continue;

                avg /= k;

                if (avg)
                    std::cout
                        << "TEST: " << i << ", " << std::setw(5) << k
                        << " reps, ENGINE: " << std::setw(10)
                        << engineName(static_cast<wasmEngines>(j))
                        << ", AVG: " << std::setw(9) << avg
                        << ", MIN: " << std::setw(9) << vi[imin] - vi[imin - 1]
                        << "(" << std::setw(4) << imin << ")"
                        << ", MAX: " << std::setw(9) << vi[imax] - vi[imax - 1]
                        << "(" << std::setw(4) << imax << ")"
                        << ", all: " << std::setw(9) << vi[k] - vi[0]
                        << std::endl;
            }
        }
    }

public:
    void
    run() override
    {
#ifdef __APPLE__
        std::cout
            << "MACOS doesn't allow runtime code generation, JIT will be "
               "disabled, the tests will be much slower than on Linux / Windows"
            << std::endl;
#endif

        using namespace test::jtx;

        // initEngines();
        testTimes.resize(TESTS_N);
        for (auto& ve : testTimes)
        {
            ve.resize(ENGINES_N);
            for (auto& vi : ve)
                vi.resize(ADD_MOD_N + 1);
        }

        for (int e = wasmEngines::Edge; e < wasmEngines::END; ++e)
        {
            // debug
            if ((e == wasmEngines::Edge)  //|| (e == wasmEngines::Time)
                                          // if ((e != wasmEngines::Wamr) && (e
                                          // != wasmEngines::Er) &&
                                          //     (e != wasmEngines::I) && (e !=
                                          //     wasmEngines::Time)
                                          //     // && (e != wasmEngines::Edge)
            )
                continue;

            setWasmEngine(static_cast<wasmEngines>(e));
            auto engine = WasmEngine::instance();
            // ptest_0_AddModule(static_cast<wasmEngines>(e), *engine);
            // ptest_1_AddInstance(static_cast<wasmEngines>(e), *engine);
            // ptest_2_RunP4(static_cast<wasmEngines>(e), *engine);
            // ptest_3_JustRunP4(static_cast<wasmEngines>(e), *engine);
            ptest_4_runFunc(static_cast<wasmEngines>(e), *engine);
        }

        ptest_Results();
    }
};

BEAST_DEFINE_TESTSUITE(Wasm, app, ripple);
BEAST_DEFINE_TESTSUITE(WasmPerf, app, ripple);

}  // namespace test
}  // namespace ripple
