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
extern std::string const bigHex;
extern std::string const sha512Hex;
extern std::string const fib32Hex;
extern std::string const fib64Hex;

extern std::string const tx_js;
extern std::string const lo_js;

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

static const std::string testNames[] = {
    "AddModule",
    "AddInstance",
    "Run P4",
    "JustRunP4",
    "RunHostFunc",
    "Fib x32",
    "Fib x64",
    "AddBigModule",
    "RunSha",
    "RunShaLarge",
    "JustRunP4 meter",
    "RunHostFunc meter",
    "Fib x64 meter",
    "RunShaLarge meter"};

class WasmPerf_test : public beast::unit_test::suite
{
    static const int TESTS_N = 20;
    static const int ENGINES_N = wasmEngines::END;
    static const int ADD_MOD_N = 1000;

    static const int FIB_N = 5;

#ifdef _DEBUG
    static const int ADD_MOD_SMALL_N = 10;
    static const int FIB_VAL_32 = 20;
    static const int FIB_VAL_64 = 30;
    static const int BIG_MOD_N = 10;
    static const int SHA_N = 30;
    static const int BIG_SHA_N = 3;
    static const int GAS_N = 50;
#else
    static const int ADD_MOD_SMALL_N = ADD_MOD_N;
    static const int FIB_VAL_32 = 35;
    static const int FIB_VAL_64 = 40;  // 48;
    static const int BIG_MOD_N = 30;
    static const int SHA_N = 500;
    static const int BIG_SHA_N = 20;
    static const int GAS_N = 500;
#endif

    // testcase, engine, iteration
    std::vector<std::vector<std::vector<std::uint64_t>>> testTimes;

    inline static std::string
    wname(wasmEngines ei)
    {
        return std::string(engineName(static_cast<wasmEngines>(ei)));
    }

    void
    ptest0(
        std::string name,
        int tnum,
        wasmEngines ei,
        WasmEngine& e,
        int inum,
        const std::string& modHex)
    {
        auto const ws = boost::algorithm::unhex(modHex);
        vbytes const wasm(ws.begin(), ws.end());

        std::cout << std::endl;
        std::string s = name + "(" + std::to_string(tnum) + ") " + wname(ei) +
            " mod size(" + std::to_string(wasm.size()) + "), cold";
        testcase(s);

        auto& times(testTimes[tnum][ei]);

        times[0] = usecs();
        for (int i = 0; i < inum; ++i)
        {
            // if (!(i % 50))
            //     e.clearModules();
            auto const midx = e.addModule(wasm);
            times[i + 1] = usecs();
            if (!BEAST_EXPECT(midx >= 0))
            {
                std::cout << "Error loading module " << i << std::endl;
                return;
            }
        }

        BEAST_EXPECT(times[inum] > 0);
    }

    // void ptest_1_AddInstance(wasmEngines ei, WasmEngine& e)
    void
    ptest1(
        std::string name,
        int tnum,
        wasmEngines ei,
        WasmEngine& e,
        int inum,
        const std::string& modHex)
    {
        auto const ws = boost::algorithm::unhex(modHex);
        vbytes const wasm(ws.begin(), ws.end());

        std::cout << std::endl;
        std::string s = name + "(" + std::to_string(tnum) + ") " + wname(ei) +
            " mod size(" + std::to_string(wasm.size()) + "), hot";
        testcase(s);

        auto& times(testTimes[tnum][ei]);

        int iidx = 0;
        int const midx = e.addModule(wasm);

        times[0] = usecs();
        for (int i = 0; i < inum; ++i)
        {
            iidx = e.addInstance(midx);
            if (!BEAST_EXPECT(iidx >= 0))
            {
                std::cout << "Error creating module " << i << std::endl;
                return;
            }
            times[i + 1] = usecs();
        }

        BEAST_EXPECT(times[inum] > 0);
        BEAST_EXPECT(iidx == inum);
    }

    void
    ptest2(
        std::string name,
        int tnum,
        wasmEngines ei,
        WasmEngine& e,
        int inum,
        const std::string& modHex,
        std::string_view fname,
        std::string const& d1,
        std::string const& d2)
    {
        auto const ws = boost::algorithm::unhex(modHex);
        vbytes const wasm(ws.begin(), ws.end());

        std::cout << std::endl;
        std::string s = name + "(" + std::to_string(tnum) + ") " + wname(ei) +
            " mod size(" + std::to_string(wasm.size()) + "), cold";
        testcase(s);

        auto& times(testTimes[tnum][ei]);

        std::vector<uint8_t> tx(d1.begin(), d1.end());
        std::vector<uint8_t> const lo(d2.begin(), d2.end());

        times[0] = usecs();
        for (int i = 0; i < inum; ++i)
        {
            auto const r = e.runP4(wasm, fname, tx, lo);
            times[i + 1] = usecs();

            if (!BEAST_EXPECT(r.has_value() && r.value().second == "1"))
                return;
        }

        BEAST_EXPECT(times[inum] > 0);
    }

    void
    ptest3(
        std::string name,
        int tnum,
        wasmEngines ei,
        WasmEngine& e,
        int inum,
        const std::string& modHex,
        std::string_view fname,
        std::string const& d1,
        std::string const& d2,
        bool meter = false)
    {
        auto const ws = boost::algorithm::unhex(modHex);
        vbytes const wasm(ws.begin(), ws.end());

        std::cout << std::endl;
        std::string s = name + "(" + std::to_string(tnum) + ") " + wname(ei) +
            " mod size(" + std::to_string(wasm.size()) + "), hot";
        testcase(s);

        auto& times(testTimes[tnum][ei]);

        std::vector<uint8_t> tx(d1.begin(), d1.end());
        std::vector<uint8_t> const lo(d2.begin(), d2.end());

        if (meter)
            e.setMeter();
        int midx = e.addModule(wasm);
        if (midx < 0)
        {
            std::cerr << "Failed to load module" << std::endl;
            return;
        }

        std::int64_t sgas = 0;
        if (meter)
            sgas = e.getRemainingGas(midx);

        times[0] = usecs();
        for (int i = 0; i < inum; ++i)
        {
            auto const r = e.justRunP4(fname, tx, lo, midx);
            times[i + 1] = usecs();

            BEAST_EXPECT(r.value().second == "1");
            // if (meter)
            // {
            //     auto const gas = e.getRemainingGas(midx);
            //     BEAST_EXPECT(gas > 100);
            // }
        }

        BEAST_EXPECT(times[inum] > 0);

        if (meter)
        {
            auto const egas = e.getRemainingGas(midx);
            std::cout << s << " Gas wasted: " << sgas - egas
                      << ", avg: " << (sgas - egas) / inum << std::endl;
        }
    }

    void
    ptest4(
        std::string name,
        int tnum,
        wasmEngines ei,
        WasmEngine& e,
        int inum,
        const std::string& modHex,
        std::string_view fname,
        bool meter = false)
    {
        auto const ws = boost::algorithm::unhex(modHex);
        vbytes const wasm(ws.begin(), ws.end());

        std::cout << std::endl;
        std::string s = name + "(" + std::to_string(tnum) + ") " + wname(ei) +
            " mod size(" + std::to_string(wasm.size()) + "), hot";
        testcase(s);

        auto& times(testTimes[tnum][ei]);

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

        if (meter)
            e.setMeter();
        auto re1 = e.preRun(wasm, &ledgerDataProvider);

        if (!re1.has_value() || re1.value() < 0)
        {
            std::cerr << "Failed to load module" << std::endl;
            return;
        }

        int midx = re1.value();
        std::int64_t sgas = 0;
        if (meter)
            sgas = e.getRemainingGas(midx);

        times[0] = usecs();
        for (int i = 0; i < inum; ++i)
        {
            auto const r = e.justRun(fname, &ledgerDataProvider, midx);
            times[i + 1] = usecs();

            BEAST_EXPECT(!r.value());
            // if (meter)
            // {
            //     auto const gas = e.getRemainingGas(midx);
            //     BEAST_EXPECT(gas > 100);
            // }
        }

        BEAST_EXPECT(times[inum] > 0);

        if (meter)
        {
            auto const egas = e.getRemainingGas(midx);
            std::cout << s << " Gas wasted: " << sgas - egas
                      << ", avg: " << (sgas - egas) / inum << std::endl;
        }

        // env.close();
        // env.close();
        // env.close();
        // env.close();

        // re = runEscrowWasm(wasm, funcName, &ledgerDataProvider);
        // if (BEAST_EXPECT(re.has_value()))
        //     BEAST_EXPECT(re.value());
    }

    void
    ptest5(
        std::string name,
        int tnum,
        wasmEngines ei,
        WasmEngine& e,
        int inum,
        const std::string& modHex,
        std::string_view fname,
        std::int32_t fibn)
    {
        auto const ws = boost::algorithm::unhex(modHex);
        vbytes const wasm(ws.begin(), ws.end());

        std::cout << std::endl;
        std::string s = name + "(" + std::to_string(tnum) + ") " + wname(ei) +
            " mod size(" + std::to_string(wasm.size()) +
            "), p: " + std::to_string(fibn) + ", cold";
        testcase(s);

        auto& times(testTimes[tnum][ei]);

        times[0] = usecs();
        for (int i = 0; i < inum; ++i)
        {
            int midx = e.addModule(wasm);
            if (midx < 0)
            {
                std::cerr << "Failed to load module" << std::endl;
                return;
            }

            auto const r = e.runFunc(fname, fibn, midx);
            times[i + 1] = usecs();

            BEAST_EXPECT(r >= 0);
        }

        BEAST_EXPECT(times[inum] > 0);
    }

    void
    ptest6(
        std::string name,
        int tnum,
        wasmEngines ei,
        WasmEngine& e,
        int inum,
        const std::string& modHex,
        std::string_view fname,
        std::int64_t fibn,
        bool meter = false)
    {
        auto const ws = boost::algorithm::unhex(modHex);
        vbytes const wasm(ws.begin(), ws.end());

        std::cout << std::endl;
        std::string s = name + "(" + std::to_string(tnum) + ") " + wname(ei) +
            " mod size(" + std::to_string(wasm.size()) +
            "), p: " + std::to_string(fibn) + ", hot";
        testcase(s);

        auto& times(testTimes[tnum][ei]);

        if (meter)
            e.setMeter();

        int midx = e.addModule(wasm);
        if (midx < 0)
        {
            std::cerr << "Failed to load module" << std::endl;
            return;
        }

        std::int64_t sgas = 0;
        if (meter)
            sgas = e.getRemainingGas(midx);

        times[0] = usecs();
        for (int i = 0; i < inum; ++i)
        {
            auto const r = e.runFunc64(fname, fibn, midx);
            times[i + 1] = usecs();

            BEAST_EXPECT(r >= 0);
        }

        BEAST_EXPECT(times[inum] > 0);

        if (meter)
        {
            auto const egas = e.getRemainingGas(midx);
            std::cout << s << " Gas wasted: " << sgas - egas
                      << ", avg: " << (sgas - egas) / inum << std::endl;
        }
    }

    void
    ptest7(
        std::string name,
        int tnum,
        wasmEngines ei,
        WasmEngine& e,
        int inum,
        const std::string& modHex,
        bool coldRun,
        std::string_view data,
        bool meter = false)
    {
        auto const ws = boost::algorithm::unhex(modHex);
        vbytes const wasm(ws.begin(), ws.end());

        std::cout << std::endl;
        std::string s = name + "(" + std::to_string(tnum) + ") " + wname(ei) +
            " mod size(" + std::to_string(wasm.size()) +
            "), p size: " + std::to_string(data.size()) +
            (coldRun ? ", cold" : ", hot");
        testcase(s);

        auto& times(testTimes[tnum][ei]);
        int midx = -1;

        if (meter)
            e.setMeter();

        if (!coldRun)
        {
            midx = e.addModule(wasm);
            if (midx < 0)
            {
                std::cerr << "Failed to load module" << std::endl;
                return;
            }
        }

        std::int64_t sgas = 0;
        if (meter)
            sgas = e.getRemainingGas(midx);

        times[0] = usecs();
        for (int i = 0; i < inum; ++i)
        {
            if (coldRun)
            {
                midx = e.addModule(wasm);
                if (midx < 0)
                {
                    std::cerr << "Failed to load module" << std::endl;
                    return;
                }
            }

            auto const r = e.runSha(data, midx);
            times[i + 1] = usecs();

            BEAST_EXPECT(r[0] > 0);
        }

        BEAST_EXPECT(times[inum] > 0);

        if (meter)
        {
            auto const egas = e.getRemainingGas(midx);
            std::cout << s << " Gas wasted: " << sgas - egas
                      << ", avg: " << (sgas - egas) / inum << std::endl;
        }
    }

    void
    ptest_Results()
    {
        std::cout << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << std::endl;

        for (int i = 0; i < TESTS_N; ++i)
        {
            for (int j = 0; j < ENGINES_N; ++j)
            {
                auto const& vi(testTimes[i][j]);
                std::uint64_t avg, all;
                int imin, imax;
                avg = all = 0;
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
                    all += dn;
                }

                if (--k <= 0)
                    continue;

                avg = all / k;

                std::cout << "TEST: " << std::setw(25) << testNames[i] << "("
                          << i << "), " << std::setw(5) << k
                          << " reps, ENGINE: " << std::setw(10)
                          << engineName(static_cast<wasmEngines>(j));
                if (all)
                    std::cout
                        << ", AVG: " << std::setw(9) << avg
                        << ", MIN: " << std::setw(9) << vi[imin] - vi[imin - 1]
                        << "(" << std::setw(4) << imin << ")"
                        << ", MAX: " << std::setw(9) << vi[imax] - vi[imax - 1]
                        << "(" << std::setw(4) << imax << ")"
                        << ", all: " << std::setw(9) << vi[k] - vi[0];
                else
                    std::cout << " SKIPPED";
                std::cout << std::endl;
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

        for (int e = 0; e < wasmEngines::END; ++e)
        {
            // clang-format off
            // debug
            if (
            //    (e != wasmEngines::Edge)
            //    (e != wasmEngines::Time)
                (e != wasmEngines::Wamr)
            //    (e != wasmEngines::Er)
            //    (e != wasmEngines::I)
            ) continue;
            // clang-format on

            setWasmEngine(static_cast<wasmEngines>(e));
            auto engine = WasmEngine::instance();

            // clang-format off

            if (engine->isImplemented(0)) ptest0("AddModule", 0, static_cast<wasmEngines>(e), *engine, ADD_MOD_SMALL_N, p4Hex); engine->clearModules();
            if (engine->isImplemented(1)) ptest1("AddInstance", 1, static_cast<wasmEngines>(e), *engine, ADD_MOD_N, p4Hex);     engine->clearModules();
            if (engine->isImplemented(2)) ptest2("Run P4", 2, static_cast<wasmEngines>(e), *engine, e ==wasmEngines::Wamr ? 50 : ADD_MOD_SMALL_N,p4Hex, "compare_accountID", tx_js, lo_js ); engine->clearModules();
            if (engine->isImplemented(3)) ptest3("JustRunP4", 3, static_cast<wasmEngines>(e), *engine, ADD_MOD_N, p4Hex, "compare_accountID", tx_js, lo_js); engine->clearModules();
            if (engine->isImplemented(4)) ptest4("RunHostFunc", 4, static_cast<wasmEngines>(e), *engine, ADD_MOD_N, p5Hex, "ready"); engine->clearModules();
            if (engine->isImplemented(5)) ptest5("Fib x32", 5, static_cast<wasmEngines>(e), *engine, 1, fib32Hex, "fib",FIB_VAL_32); engine->clearModules();
            if (engine->isImplemented(6)) ptest6("Fib x64", 6, static_cast<wasmEngines>(e), *engine, 1, fib64Hex, "fib", FIB_VAL_64); engine->clearModules();
            if (engine->isImplemented(7)) ptest0("AddBigModule", 7, static_cast<wasmEngines>(e), *engine, BIG_MOD_N, bigHex); engine->clearModules();

            // need add wasi support to engines.
            if (engine->isImplemented(8)) ptest7("RunSha", 8, static_cast<wasmEngines>(e), *engine, SHA_N, sha512Hex, true, p1Hex); engine->clearModules();
            if (engine->isImplemented(9)) ptest7("RunShaLarge", 9, static_cast<wasmEngines>(e), *engine, BIG_SHA_N, sha512Hex, false, bigHex);   engine->clearModules();

            if (engine->isImplemented(10)) ptest3("JustRunP4 meter", 10, static_cast<wasmEngines>(e), *engine, GAS_N, p4Hex, "compare_accountID", tx_js, lo_js, true ); engine->clearModules();
            if (engine->isImplemented(11)) ptest4("RunHostFunc meter", 11, static_cast<wasmEngines>(e), *engine, ADD_MOD_N, p5Hex, "ready", true); engine->clearModules();
            if (engine->isImplemented(12)) ptest6("Fib x64 meter", 12, static_cast<wasmEngines>(e), *engine, 1, fib64Hex, "fib", FIB_VAL_64, true); engine->clearModules();
            if (engine->isImplemented(13)) ptest7("RunShaLarge meter", 13, static_cast<wasmEngines>(e), *engine, BIG_SHA_N, sha512Hex, false, bigHex, true); engine->clearModules();

            // clang-format ON
        }

        ptest_Results();
    }
};

BEAST_DEFINE_TESTSUITE(Wasm, app, ripple);
BEAST_DEFINE_TESTSUITE(WasmPerf, app, ripple);

}  // namespace test
}  // namespace ripple
