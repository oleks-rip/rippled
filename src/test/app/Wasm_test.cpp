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
extern std::string const zkProofHex;
extern std::string const zkAotHex;

extern std::string const sp1_wasm;
extern std::string const sp1_aot;

extern std::string const sha512Hex;
extern std::string const fib32Hex;
extern std::string const fib64Hex;

extern std::string sha512PureHex;
extern std::string b58Hex;

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

static std::string const testNames[] = {
    "AddModule", "AddInstance", "Json",        "Json",    "LgrSqn",
    "Fib x32",   "Fib x64",     "AddModule",   "Sha512",  "Sha512",
    "Json",      "LgrSqn",      "Fib x64",     "Sha512",  "ZKProof",
    "ZKProof",   "ZKProof aot", "ZKProof aot", "Fib x64", "Sha512",
    "B58",
};

class WasmPerf_test : public beast::unit_test::suite
{
    static int const TESTS_N = 30;
    static int const ENGINES_N = wasmEngines::END;
    static int const ADD_MOD_N = 1000;

    static int const FIB_N = 5;

#ifdef _DEBUG
    static const int ADD_MOD_SMALL_N = 10;
    static int const FIB_VAL_32 = 20;
    static int const FIB_VAL_64 = 30;
    static int const BIG_MOD_N = 10;
    static int const SHA_N = 30;
    static int const BIG_SHA_N = 3;
    static int const GAS_N = 50;
    static int const ZKP_N = 30;
#else
    static const int ADD_MOD_SMALL_N = ADD_MOD_N;
    static int const FIB_VAL_32 = 35;
    static int const FIB_VAL_64 = 40;  // 48;
    static int const BIG_MOD_N = 50;
    static int const SHA_N = 500;
    static int const BIG_SHA_N = 20;
    static int const GAS_N = 500;
    static int const ZKP_N = 200;
#endif

    static const int GAS_CHECK_N = 16;
    static int const FIB_VAL_GAS_CHECK = 10;

    // testcase, engine, iteration
    std::vector<std::vector<std::vector<std::uint64_t>>> testTimes;

    inline static std::string
    wname(wasmEngines ei)
    {
        return std::string(engineName(static_cast<wasmEngines>(ei)));
    }

    void
    pt0_add_module(
        int tnum,
        wasmEngines ei,
        WasmEngine& e,
        int inum,
        std::string const& modHex,
        bool meter = false)
    {
        auto const ws = boost::algorithm::unhex(modHex);
        vbytes const wasm(ws.begin(), ws.end());

        std::cout << std::endl;
        std::string s = testNames[tnum] + "(" + std::to_string(tnum) +
            "): " + wname(ei) + " mod size: " + std::to_string(wasm.size()) +
            ", inum: " + std::to_string(inum) +
            (meter ? ", meter" : ", no meter");
        testcase(s);

        auto& times(testTimes[tnum][ei]);

        if (meter)
            e.setMeter();

        times[0] = usecs();
        for (int i = 0; i < inum; ++i)
        {
            if (meter && ei == wasmEngines::Er)
                e.setMeter();
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
    pt1_add_instance(
        int tnum,
        wasmEngines ei,
        WasmEngine& e,
        int inum,
        std::string const& modHex)
    {
        auto const ws = boost::algorithm::unhex(modHex);
        vbytes const wasm(ws.begin(), ws.end());

        std::cout << std::endl;
        std::string s = testNames[tnum] + "(" + std::to_string(tnum) + ") " +
            wname(ei) + " mod size(" + std::to_string(wasm.size()) + "), hot";
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
                std::cout << "Error creating instance " << i << std::endl;
                return;
            }
            times[i + 1] = usecs();
        }

        BEAST_EXPECT(times[inum] > 0);
        BEAST_EXPECT(iidx == inum);
    }

    void
    pt2_json(
        int tnum,
        wasmEngines ei,
        WasmEngine& e,
        int inum,
        std::string const& modHex,
        std::string_view fname,
        std::string const& d1,
        std::string const& d2,
        bool coldRun = true,
        bool meter = false)
    {
        auto const ws = boost::algorithm::unhex(modHex);
        vbytes const wasm(ws.begin(), ws.end());

        std::cout << std::endl;
        std::string s = testNames[tnum] + "(" + std::to_string(tnum) +
            "): " + wname(ei) + " mod size: " + std::to_string(wasm.size()) +
            ", inum: " + std::to_string(inum) +
            ", fname: " + std::string(fname) + (coldRun ? ", cold" : ", hot") +
            (meter ? ", meter" : ", no meter");
        testcase(s);

        auto& times(testTimes[tnum][ei]);
        int midx = -1;
        std::int64_t sgas = 0;
        std::int64_t cgas = 0;

        std::vector<uint8_t> const tx(d1.begin(), d1.end());
        std::vector<uint8_t> const lo(d2.begin(), d2.end());

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

            if (meter)
                cgas = sgas = e.getRemainingGas(midx);
        }

        times[0] = usecs();
        for (int i = 0; i < inum; ++i)
        {
            if (coldRun)
            {
                if (meter && ei == wasmEngines::Er)
                    e.setMeter();
                midx = e.addModule(wasm);
                if (midx < 0)
                {
                    std::cerr << "Failed to load module" << std::endl;
                    return;
                }

                if (meter)
                    cgas = sgas = e.getRemainingGas(midx);
            }

            auto const r = e.justRunP4(fname, tx, lo, midx);
            times[i + 1] = usecs();

            if (!BEAST_EXPECT(r && r->second == "1"))
                return;

            if (meter && (sgas > 0) && inum == GAS_CHECK_N)
            {
                auto const egas = e.getRemainingGas(midx);
                std::cout << s << " Gas in cycle: " << cgas - egas << std::endl;
                cgas = egas;
                // BEAST_EXPECT(gas > 100);
            }
        }

        BEAST_EXPECT(times[inum] > 0);

        if (meter && (sgas > 0))
        {
            auto const egas = e.getRemainingGas(midx);
            std::cout << s << " Gas overall: " << sgas - egas
                      << ", avg: " << (sgas - egas) / inum << std::endl;
        }
    }

    void
    pt4_lsqn_hot(
        int tnum,
        wasmEngines ei,
        WasmEngine& e,
        int inum,
        std::string const& modHex,
        std::string_view fname,
        bool meter = false)
    {
        auto const ws = boost::algorithm::unhex(modHex);
        vbytes const wasm(ws.begin(), ws.end());

        std::cout << std::endl;
        std::string s = testNames[tnum] + "(" + std::to_string(tnum) + ") " +
            wname(ei) + " mod size(" + std::to_string(wasm.size()) + "), hot";
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
        std::int64_t cgas = 0;
        if (meter)
            cgas = sgas = e.getRemainingGas(midx);

        times[0] = usecs();
        for (int i = 0; i < inum; ++i)
        {
            auto const r = e.justRun(fname, &ledgerDataProvider, midx);
            times[i + 1] = usecs();

            BEAST_EXPECT(!r.value());
            if (meter && inum == GAS_CHECK_N)
            {
                auto const egas = e.getRemainingGas(midx);
                std::cout << s << " Gas in cycle: " << cgas - egas << std::endl;
                cgas = egas;
                // BEAST_EXPECT(gas > 100);
            }
        }

        BEAST_EXPECT(times[inum] > 0);

        if (meter)
        {
            auto const egas = e.getRemainingGas(midx);
            std::cout << s << " Gas overall: " << sgas - egas
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
    pt5_fn1(
        int tnum,
        wasmEngines ei,
        WasmEngine& e,
        int inum,
        std::string const& modHex,
        std::string_view fname,
        std::int32_t fibn)
    {
        auto const ws = boost::algorithm::unhex(modHex);
        vbytes const wasm(ws.begin(), ws.end());

        std::cout << std::endl;
        std::string s = testNames[tnum] + "(" + std::to_string(tnum) + ") " +
            wname(ei) + " mod size(" + std::to_string(wasm.size()) +
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
    pt6_fn1_x64(
        int tnum,
        wasmEngines ei,
        WasmEngine& e,
        int inum,
        std::string const& modHex,
        std::string_view fname,
        std::int64_t fibn,
        bool coldRun,
        bool meter = false)
    {
        auto const ws = boost::algorithm::unhex(modHex);
        vbytes const wasm(ws.begin(), ws.end());

        std::cout << std::endl;
        std::string s = testNames[tnum] + "(" + std::to_string(tnum) +
            "): " + wname(ei) + " mod size: " + std::to_string(wasm.size()) +
            ", inum: " + std::to_string(inum) +
            ", fname: " + std::string(fname) + (coldRun ? ", cold" : ", hot") +
            (meter ? ", meter" : ", no meter") + ", p: " + std::to_string(fibn);
        testcase(s);

        auto& times(testTimes[tnum][ei]);
        int midx = -1;
        std::int64_t sgas = 0;
        std::int64_t cgas = 0;

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

            if (meter)
                cgas = sgas = e.getRemainingGas(midx);
        }

        times[0] = usecs();
        for (int i = 0; i < inum; ++i)
        {
            if (coldRun)
            {
                if (meter && ei == wasmEngines::Er)
                    e.setMeter();
                midx = e.addModule(wasm);
                if (midx < 0)
                {
                    std::cerr << "Failed to load module" << std::endl;
                    return;
                }

                if (meter)
                    cgas = sgas = e.getRemainingGas(midx);
            }

            auto const r = e.runFunc64(fname, fibn, midx);
            times[i + 1] = usecs();

            BEAST_EXPECT(r >= 0);

            if (meter && (sgas > 0) && inum == GAS_CHECK_N)
            {
                auto const egas = e.getRemainingGas(midx);
                std::cout << s << " Gas in cycle: " << cgas - egas << std::endl;
                cgas = egas;
                // BEAST_EXPECT(gas > 100);
            }
        }

        BEAST_EXPECT(times[inum] > 0);

        if (meter && (sgas > 0))
        {
            auto const egas = e.getRemainingGas(midx);
            std::cout << s << " Gas overall: " << sgas - egas
                      << ", avg: " << (sgas - egas) / inum << std::endl;
        }
    }

    void
    pt7_fn1a(
        int tnum,
        wasmEngines ei,
        WasmEngine& e,
        int inum,
        std::string const& modHex,
        std::string_view fname,
        std::string_view data,
        bool coldRun,
        bool meter = false)
    {
        auto const ws = boost::algorithm::unhex(modHex);
        vbytes const wasm(ws.begin(), ws.end());

        std::cout << std::endl;
        std::string s = testNames[tnum] + "(" + std::to_string(tnum) +
            "): " + wname(ei) + " mod size: " + std::to_string(wasm.size()) +
            ", inum: " + std::to_string(inum) +
            ", fname: " + std::string(fname) + (coldRun ? ", cold" : ", hot") +
            (meter ? ", meter" : ", no meter");
        testcase(s);

        auto& times(testTimes[tnum][ei]);
        int midx = -1;
        std::int64_t sgas = 0;
        std::int64_t cgas = 0;

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
            if (meter)
                cgas = sgas = e.getRemainingGas(midx);
        }

        times[0] = usecs();
        for (int i = 0; i < inum; ++i)
        {
            if (coldRun)
            {
                if (meter && ei == wasmEngines::Er)
                    e.setMeter();
                midx = e.addModule(wasm);
                if (midx < 0)
                {
                    std::cerr << "Failed to load module" << std::endl;
                    return;
                }
                if (meter)
                    cgas = sgas = e.getRemainingGas(midx);
            }

            auto const r = e.runFunc(fname, data, midx);
            times[i + 1] = usecs();

            BEAST_EXPECT(r[0] > 0);

            if (meter && (sgas > 0) && inum <= GAS_CHECK_N)
            {
                auto const egas = e.getRemainingGas(midx);
                std::cout << s << " Gas in cycle: " << cgas - egas << std::endl;
                cgas = egas;
                // BEAST_EXPECT(gas > 100);
            }
        }

        BEAST_EXPECT(times[inum] > 0);

        if (meter && (sgas > 0))
        {
            auto const egas = e.getRemainingGas(midx);
            std::cout << s << " Gas overall: " << sgas - egas
                      << ", avg: " << (sgas - egas) / inum << std::endl;
        }
    }

    void
    pt8_fn0(
        int tnum,
        wasmEngines ei,
        WasmEngine& e,
        int inum,
        std::string const& modHex,
        std::string_view fname,
        bool coldRun,
        bool meter = false)
    {
        auto const ws = boost::algorithm::unhex(modHex);
        vbytes const wasm(ws.begin(), ws.end());

        std::cout << std::endl;
        std::string s = testNames[tnum] + "(" + std::to_string(tnum) +
            "): " + wname(ei) + " mod size: " + std::to_string(wasm.size()) +
            ", inum: " + std::to_string(inum) +
            ", fname: " + std::string(fname) + (coldRun ? ", cold" : ", hot") +
            (meter ? ", meter" : ", no meter");
        testcase(s);

        auto& times(testTimes[tnum][ei]);
        int midx = -1;
        std::int64_t sgas = 0;
        std::int64_t cgas = 0;

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
            if (meter)
                cgas = sgas = e.getRemainingGas(midx);
        }

        times[0] = usecs();
        for (int i = 0; i < inum; ++i)
        {
            if (coldRun)
            {
                if (meter && ei == wasmEngines::Er)
                    e.setMeter();
                midx = e.addModule(wasm);
                if (midx < 0)
                {
                    std::cerr << "Failed to load module" << std::endl;
                    return;
                }
                if (meter)
                    cgas = sgas = e.getRemainingGas(midx);
            }

            auto const r = e.justRun(fname, midx);
            times[i + 1] = usecs();

            BEAST_EXPECT(r && *r == 1);
            if (meter && (sgas > 0) && inum == GAS_CHECK_N)
            {
                auto const egas = e.getRemainingGas(midx);
                std::cout << s << " Gas in cycle: " << cgas - egas << std::endl;
                cgas = egas;
                // BEAST_EXPECT(gas > 100);
            }
        }

        BEAST_EXPECT(times[inum] > 0);

        if (meter && (sgas > 0))
        {
            auto const egas = e.getRemainingGas(midx);
            std::cout << s << " Gas overall: " << sgas - egas
                      << ", avg: " << (sgas - egas) / inum << std::endl;
        }
    }

    void
    pt9_fn2a(
        int tnum,
        wasmEngines ei,
        WasmEngine& e,
        int inum,
        std::string const& modHex,
        std::string_view fname,
        std::string_view data,
        std::string& sv_res,
        bool coldRun,
        bool meter = false)
    {
        auto const ws = boost::algorithm::unhex(modHex);
        vbytes const wasm(ws.begin(), ws.end());

        std::cout << std::endl;
        std::string s = testNames[tnum] + "(" + std::to_string(tnum) +
            "): " + wname(ei) + " mod size: " + std::to_string(wasm.size()) +
            ", inum: " + std::to_string(inum) +
            ", fname: " + std::string(fname) + (coldRun ? ", cold" : ", hot") +
            (meter ? ", meter" : ", no meter");
        testcase(s);

        auto& times(testTimes[tnum][ei]);
        int midx = -1;
        std::int64_t sgas = 0;
        std::int64_t cgas = 0;

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

            if (meter)
                cgas = sgas = e.getRemainingGas(midx);
        }

        times[0] = usecs();
        for (int i = 0; i < inum; ++i)
        {
            if (coldRun)
            {
                if (meter && ei == wasmEngines::Er)
                    e.setMeter();
                midx = e.addModule(wasm);
                if (midx < 0)
                {
                    std::cerr << "Failed to load module" << std::endl;
                    return;
                }

                if (meter)
                    cgas = sgas = e.getRemainingGas(midx);
            }

            auto const r = e.runFunc(fname, sv_res, data, midx);
            times[i + 1] = usecs();

            BEAST_EXPECT(r > 0);

            if (meter && (sgas > 0) && inum == GAS_CHECK_N)
            {
                auto const egas = e.getRemainingGas(midx);
                std::cout << s << " Gas in cycle: " << cgas - egas << std::endl;
                cgas = egas;
                // BEAST_EXPECT(gas > 100);
            }
        }

        BEAST_EXPECT(times[inum] > 0);

        if (meter && (sgas > 0))
        {
            auto const egas = e.getRemainingGas(midx);
            std::cout << s << " Gas overall: " << sgas - egas
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
                (e == wasmEngines::Edge)
          // || (e == wasmEngines::Time)
          // || (e == wasmEngines::Wamr)
             || (e == wasmEngines::Er)
          // || (e == wasmEngines::I)
            ) continue;
            // clang-format on

            setWasmEngine(static_cast<wasmEngines>(e));
            auto engine = WasmEngine::instance();
            std::string sv_res;

            // clang-format off

            // INTERP TEST
            if (engine->isImplemented(0)) pt0_add_module(0, static_cast<wasmEngines>(e), *engine, ADD_MOD_SMALL_N, fib64Hex); engine->clearModules();
            if (engine->isImplemented(7)) pt0_add_module(7, static_cast<wasmEngines>(e), *engine, BIG_MOD_N, zkProofHex); engine->clearModules();

            if (engine->isImplemented(2)) pt2_json(2, static_cast<wasmEngines>(e), *engine, ADD_MOD_SMALL_N,p4Hex, "compare_accountID", tx_js, lo_js, true, true ); engine->clearModules();
            if (engine->isImplemented(3)) pt2_json(3, static_cast<wasmEngines>(e), *engine, ADD_MOD_N, p4Hex, "compare_accountID", tx_js, lo_js, false, true); engine->clearModules();

            // COLD
            if (engine->isImplemented(6)) pt6_fn1_x64( 6, static_cast<wasmEngines>(e), *engine, 1, fib64Hex, "fib", FIB_VAL_64, true, true); engine->clearModules();
            if (engine->isImplemented(19)) pt7_fn1a(19, static_cast<wasmEngines>(e), *engine, SHA_N, sha512PureHex, "sha512_process", std::string_view(tx_js.data(), 128), true, true); engine->clearModules();
            if (engine->isImplemented(20)) pt9_fn2a(20, static_cast<wasmEngines>(e), *engine, SHA_N, b58Hex, "b58enco", std::string_view(tx_js.data(), 128), sv_res, true, true); engine->clearModules();

            // HOT (overwrite cold)
            // if (engine->isImplemented(6)) pt6_fn1_x64( 6, static_cast<wasmEngines>(e), *engine, 1, fib64Hex, "fib", FIB_VAL_64, false, true); engine->clearModules();
            // if (engine->isImplemented(19)) pt7_fn1a(19, static_cast<wasmEngines>(e), *engine, SHA_N, sha512PureHex, "sha512_process", std::string_view(tx_js.data(), 128), false, true); engine->clearModules();
            // if (engine->isImplemented(20)) pt9_fn2a(20, static_cast<wasmEngines>(e), *engine, SHA_N, b58Hex, "b58enco", std::string_view(tx_js.data(), 128), sv_res, false, true); engine->clearModules();


            //////////////////////////////////////////////

            // if (engine->isImplemented(0)) pt0_add_module( 0, static_cast<wasmEngines>(e), *engine, ADD_MOD_SMALL_N, p4Hex); engine->clearModules();
            // if (engine->isImplemented(1)) pt1_add_instance("AddInstance", 1, static_cast<wasmEngines>(e), *engine, ADD_MOD_N, p4Hex);     engine->clearModules();
            // if (engine->isImplemented(2)) pt2_json(2, static_cast<wasmEngines>(e), *engine, ADD_MOD_SMALL_N,p4Hex, "compare_accountID", tx_js, lo_js ); engine->clearModules();
            // if (engine->isImplemented(3)) pt2_json(3, static_cast<wasmEngines>(e), *engine, ADD_MOD_N, p4Hex, "compare_accountID", tx_js, lo_js, false); engine->clearModules();
            // if (engine->isImplemented(4)) pt4_lsqn_hot("RunHostFunc", 4, static_cast<wasmEngines>(e), *engine, ADD_MOD_N, p5Hex, "ready"); engine->clearModules();
            // if (engine->isImplemented(5)) pt5_fn1( 5, static_cast<wasmEngines>(e), *engine, 1, fib32Hex, "fib",FIB_VAL_32); engine->clearModules();
            // if (engine->isImplemented(6)) pt6_fn1_x64( 6, static_cast<wasmEngines>(e), *engine, 1, fib64Hex, "fib", FIB_VAL_64, false); engine->clearModules();
            // if (engine->isImplemented(18)) pt6_fn1_x64(18, static_cast<wasmEngines>(e), *engine, ADD_MOD_SMALL_N, fib64Hex, "fib", FIB_VAL_GAS_CHECK, false, true); engine->clearModules();
            // if (engine->isImplemented(7)) pt0_add_module(7, static_cast<wasmEngines>(e), *engine, BIG_MOD_N, zkProofHex); engine->clearModules();

            // // need add wasi support to engines.
            // if (engine->isImplemented(8)) pt7_fn1a(8, static_cast<wasmEngines>(e), *engine, SHA_N, sha512Hex, "sha512_process", p1Hex, true, false); engine->clearModules();
            // if (engine->isImplemented(9)) pt7_fn1a(9, static_cast<wasmEngines>(e), *engine, BIG_SHA_N, sha512Hex, zkProofHex, false, false);   engine->clearModules();

            // // PERF WITH GAS
            // if (engine->isImplemented(10)) pt2_json(10, static_cast<wasmEngines>(e), *engine, GAS_N, p4Hex, "compare_accountID", tx_js, lo_js, false, true); engine->clearModules();
            // //if (engine->isImplemented(11)) pt4_lsqn_hot("RunHostFunc meter", 11, static_cast<wasmEngines>(e), *engine, ADD_MOD_N, p5Hex, "ready", true); engine->clearModules();
            // if (engine->isImplemented(12)) pt6_fn1_x64( 12, static_cast<wasmEngines>(e), *engine, 1, fib64Hex, "fib", FIB_VAL_64, true); engine->clearModules();
            // if (engine->isImplemented(13)) pt7_fn1a(13, static_cast<wasmEngines>(e), *engine, BIG_SHA_N, sha512Hex, "sha512_process", zkProofHex, false, true); engine->clearModules();

            // JUST GAS CHECK
            // if (engine->isImplemented(10)) pt2_json(10, static_cast<wasmEngines>(e), *engine, GAS_CHECK_N, p4Hex, "compare_accountID", tx_js, lo_js, false, true ); engine->clearModules();
            // if (engine->isImplemented(12)) pt6_fn1_x64( 12, static_cast<wasmEngines>(e), *engine, GAS_CHECK_N, fib64Hex, "fib", FIB_VAL_GAS_CHECK, false, true); engine->clearModules();
            // if (engine->isImplemented(13)) pt7_fn1a(13, static_cast<wasmEngines>(e), *engine, GAS_CHECK_N, sha512Hex, "sha512_process", fib32Hex, false, true); engine->clearModules();


            // ZK PROOF CHECK
            // pt8_fn0("ZKProof", 14, static_cast<wasmEngines>(e), *engine, 30, zkProofHex, "bellman_groth16_test", false); engine->clearModules();
            // pt8_fn0("ZKProof", 15, static_cast<wasmEngines>(e), *engine, 20, zkProofHex, "bellman_groth16_test", true); engine->clearModules();
            // pt8_fn0("ZKProof aot", 16, static_cast<wasmEngines>(e), *engine, 50, zkAotHex, "bellman_groth16_test", false); engine->clearModules();
            // pt8_fn0("ZKProof aot", 17, static_cast<wasmEngines>(e), *engine, 50, zkAotHex, "bellman_groth16_test", true); engine->clearModules();

            // SP1 ZK
            // pt8_fn0("ZKProof", 14, static_cast<wasmEngines>(e), *engine, 10, sp1_wasm, "sp1_groth16_verifier", false); engine->clearModules();
            // pt8_fn0("ZKProof", 15, static_cast<wasmEngines>(e), *engine, 10, sp1_wasm, "sp1_groth16_verifier", true); engine->clearModules();
            // pt8_fn0("ZKProof aot", 16, static_cast<wasmEngines>(e), *engine, 100, sp1_aot, "sp1_groth16_verifier", false); engine->clearModules();
            // pt8_fn0("ZKProof aot", 17, static_cast<wasmEngines>(e), *engine, 100, sp1_aot, "sp1_groth16_verifier", true); engine->clearModules();


            // SMALL CODE/MODULES CHECK
            //if (engine->isImplemented(0)) pt0_add_module(0, static_cast<wasmEngines>(e), *engine, ADD_MOD_SMALL_N, b58Hex, true); engine->clearModules();

            // artificial memory "allocation"
            // if (engine->isImplemented(19)) pt7_fn1a(19, static_cast<wasmEngines>(e), *engine, GAS_CHECK_N, sha512PureHex, "sha512_process", std::string_view(tx_js.data(), 128), false, true); engine->clearModules();
            // if (engine->isImplemented(20)) pt9_fn2a(20, static_cast<wasmEngines>(e), *engine, GAS_CHECK_N, b58Hex, "b58enco", std::string_view(tx_js.data(), 128), sv_res, false, true); engine->clearModules();

            static_assert(sizeof(testNames)/sizeof(testNames[0]) >= 20);
            // clang-format ON
        }

        ptest_Results();
    }
};

BEAST_DEFINE_TESTSUITE(Wasm, app, ripple);
BEAST_DEFINE_TESTSUITE(WasmPerf, app, ripple);

}  // namespace test
}  // namespace ripple
