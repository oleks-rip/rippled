#include <test/jtx/AMM.h>
#include <test/jtx/Account.h>
#include <test/jtx/Env.h>
#include <test/jtx/Oracle.h>
#include <test/jtx/TestHelpers.h>
#include <test/jtx/acctdelete.h>
#include <test/jtx/amount.h>
#include <test/jtx/batch.h>
#include <test/jtx/check.h>
#include <test/jtx/credentials.h>
#include <test/jtx/delegate.h>
#include <test/jtx/deposit.h>
#include <test/jtx/did.h>
#include <test/jtx/envconfig.h>
#include <test/jtx/escrow.h>
#include <test/jtx/fee.h>
#include <test/jtx/flags.h>
#include <test/jtx/jtx_json.h>
#include <test/jtx/mpt.h>
#include <test/jtx/multisign.h>
#include <test/jtx/noop.h>
#include <test/jtx/offer.h>
#include <test/jtx/pay.h>
#include <test/jtx/permissioned_domains.h>
#include <test/jtx/seq.h>
#include <test/jtx/sig.h>
#include <test/jtx/sponsor.h>
#include <test/jtx/ter.h>
#include <test/jtx/ticket.h>
#include <test/jtx/token.h>
#include <test/jtx/trust.h>
#include <test/jtx/txflags.h>
#include <test/jtx/vault.h>
#include <test/jtx/xchain_bridge.h>

#include <xrpld/core/Config.h>

#include <xrpl/basics/Number.h>
#include <xrpl/basics/Slice.h>
#include <xrpl/basics/base_uint.h>
#include <xrpl/basics/chrono.h>
#include <xrpl/basics/strHex.h>
#include <xrpl/beast/unit_test/suite.h>
#include <xrpl/beast/utility/instrumentation.h>
#include <xrpl/core/ServiceRegistry.h>
#include <xrpl/json/json_value.h>
#include <xrpl/ledger/ApplyView.h>
#include <xrpl/ledger/OpenView.h>
#include <xrpl/ledger/helpers/AccountRootHelpers.h>
#include <xrpl/protocol/AccountID.h>
#include <xrpl/protocol/Asset.h>
#include <xrpl/protocol/Feature.h>
#include <xrpl/protocol/Indexes.h>
#include <xrpl/protocol/Issue.h>
#include <xrpl/protocol/LedgerFormats.h>
#include <xrpl/protocol/Protocol.h>
#include <xrpl/protocol/SField.h>
#include <xrpl/protocol/STAmount.h>
#include <xrpl/protocol/TER.h>
#include <xrpl/protocol/TxFlags.h>
#include <xrpl/protocol/UintTypes.h>
#include <xrpl/protocol/jss.h>
#include <xrpl/tx/apply.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace xrpl::test {

// static STAmount
// accountReserve(jtx::Env& env, std::uint32_t count = 1)
// {
//     return env.current()->fees().reserve * count;
// }

static XRPAmount
reserve(jtx::Env& env, std::uint32_t ownerCount)
{
    return baseAccountReserve(*env.current(), ownerCount);
}

static void
adjustAccountXRPBalance(jtx::Env& env, jtx::Account const& account, STAmount const& balanceTo)
{
    using namespace test::jtx;
    XRPL_ASSERT(isXRP(balanceTo), "adjustAccountXRPBalance: balanceTo must be XRP");
    auto const currentBalance = env.balance(account);
    if (currentBalance == balanceTo)
        return;

    auto const baseFee = env.current()->fees().base;
    if (currentBalance > balanceTo)
    {
        env(pay(account, env.master, currentBalance - (balanceTo)),
            Fee(XRP(1)),
            sponsor::As(env.master, spfSponsorFee),
            Sig(sfSponsorSignature, env.master));
    }
    else
    {
        env(pay(env.master, account, balanceTo - currentBalance), Fee(baseFee));
    }

    env.close();
}

class SponsorSherlock_test : public beast::unit_test::Suite, public test::jtx::XChainBridgeObjects
{
protected:
    void
    test168CoSignedBlockedWithFeeOnlySponsorship()
    {
        // https://github.com/sherlock-audit/2026-04-xrp-ledger-april-2026-judging/issues/168
        testcase("Co-signed SponsorshipTransfer blocked with fee-only Sponsorship");
        using namespace test::jtx;
        Env env(*this);

        auto const sponsor = Account("sponsor");
        auto const sponsee = Account("sponsee");

        env.fund(XRP(10000), sponsor);
        env.fund(XRP(1000), sponsee);
        env.close();

        // Create fee-only sponsorship (no ReserveCount)
        env(sponsor::set_fee(sponsor, 0, XRP(100)), sponsor::SponseeAcc(sponsee));
        env.close();

        // Sponsee creates a DID
        env(did::setValid(sponsee));
        env.close();

        auto const didKey = keylet::did(sponsee.id());

        // Sponsor tries to co-sign SponsorshipTransfer to sponsor the DID
        // This SHOULD work (sponsor has 10000 XRP) but FAILS with tecINSUFFICIENT_RESERVE

        // TEAM DECISION: considered as legit behavior, no fallbacks, sponsor should look after
        // their Sponsorship objects
        env(sponsor::transfer(sponsee, tfSponsorshipCreate, didKey.key),
            sponsor::As(sponsor, spfSponsorReserve),
            Sig(sfSponsorSignature, sponsor),
            Ter(tecINSUFFICIENT_RESERVE));
    }

    void
    test251AMMDepositRejectXRPDeposits()
    {
        // https://github.com/sherlock-audit/2026-04-xrp-ledger-april-2026-judging/issues/251
        // AMMDeposit::preclaim misroutes !sle as accountCountDelta and hardcodes ownerCountDelta=1,
        // over-charging reserve check by 1 increment (2 XRP) when LP trustline exists (!sle=0).
        testcase("AMMDeposit rejects XRP deposits with insufficient reserve");
        using namespace test::jtx;

        Env env(*this);

        auto const issuer = Account("issuer");
        auto const alice = Account("alice");

        // Fund accounts with initial amounts
        env.fund(XRP(1000), issuer);
        env.fund(XRP(1000), alice);
        env.close();

        // Step 1: Issuer sets DefaultRipple flag
        env(fset(issuer, asfDefaultRipple));
        env.close();

        // Step 2: Alice creates trustline for TKN
        auto const TKN = issuer["TKN"];
        env(trust(alice, TKN(1'000'000)));
        env.close();

        // Step 3: Issuer sends 100 TKN to Alice
        env(pay(issuer, alice, TKN(100)));
        env.close();

        // Verify Alice has ownerCount=1 (TKN trustline only)
        BEAST_EXPECT(env.ownerCount(alice) == 1);

        // Step 4: Alice creates AMM (100 TKN / 5 XRP)
        AMM amm(env, alice, TKN(100), XRP(5), CreateArg{.tfee = 500});

        // Verify Alice now has ownerCount=2 (TKN trustline + LP trustline)
        BEAST_EXPECT(env.ownerCount(alice) == 2);

        // Step 5: Drain Alice's balance down to approximately 15 XRP total
        // Target: Leave Alice with just above 15 XRP
        // With ownerCount=2, reserve requirement is base(10) + 2*inc(2) = 14 XRP
        // So 15 XRP + fees should be sufficient (15 >= 14)
        auto currentBalance = env.balance(alice);
        auto const targetBalance = drops(15'001'000);  // 15.001 XRP
        auto const baseFee = env.current()->fees().base;
        auto const reserve2 = reserve(env, 2);  // Reserve for ownerCount=2 (should be 14 XRP)

        // Calculate how much to drain
        // We want final balance = targetBalance
        // Payment equation: currentBalance - drainAmount - fee = targetBalance
        // So: drainAmount = currentBalance - targetBalance - fee
        if (currentBalance > targetBalance + baseFee)
        {
            auto const drainAmount = currentBalance - targetBalance - baseFee;

            // Sanity check: ensure Alice can actually send this
            // (must keep at least reserve2)
            if (currentBalance - drainAmount - baseFee >= reserve2)
            {
                env(pay(alice, issuer, drainAmount), Fee(baseFee));
                env.close();
            }
        }

        auto const aliceBalance = env.balance(alice);
        BEAST_EXPECT(env.ownerCount(alice) == 2);
        // Verify Alice has a reasonable balance above reserve but not too much
        // Reserve for ownerCount=2 is 14 XRP, so Alice should have >= 14 XRP
        BEAST_EXPECT(aliceBalance >= reserve2);

        // Step 6: Alice attempts AMMDeposit with 1 drop XRP
        // With ownerCount=2 and LP trustline exists (!sle=0 since she has LP trustline):
        //   Correct behavior: reserve_for(N=2, !sle=0) = base + 2*inc = 10 + 2*2 = 14 XRP
        //   Post-deposit balance ~= 15 XRP - 1 drop ≈ 15 XRP. 15 >= 14 -> should PASS.
        //
        // Bug (issue #251): reserve_for(N+1=3, accountCountDelta=0) = base + 3*inc = 10 + 6 = 16
        // XRP.
        //                   15 < 16 -> would fail with tecINSUF_RESERVE_LINE.
        //
        // This test verifies the bug is FIXED - the deposit should succeed.
        amm.deposit(alice, {}, drops(1), {}, {}, tfSingleAsset, {}, {});
    }

    void
    test1033SponsoredWitnessCanChargeDoorOwnedClaimObjectsToUnrelatedSponsor()
    {
        // https://github.com/sherlock-audit/2026-04-xrp-ledger-april-2026-judging/issues/1033

        testcase("xchain account-create attestation redirects witness sponsorship to the door");
        using namespace test::jtx;

        Env mcEnv{*this, envconfig(), features};
        Env scEnv{*this, envconfig(), features};

        Account const sponsor{"sponsor"};
        Account const submitter{"submitter"};
        auto const& witnessSigner = signers[0];

        STAmount const funds{XRP(10000)};
        mcEnv.fund(funds, mcDoor, mcAlice, mcBob, mcCarol, mcGw);
        scEnv.fund(funds, scDoor, scAlice, scBob, scCarol, scGw, sponsor, submitter);
        for (auto const& s : signers)
        {
            mcEnv.fund(funds, s.account);
            scEnv.fund(funds, s.account);
        }
        for (auto const& payee : payees)
            scEnv.fund(funds, payee);
        mcEnv.close();
        scEnv.close();

        auto const& door = scEnv.master;

        mcEnv(jtx::signers(mcDoor, quorum, signers));
        mcEnv(bridgeCreate(mcDoor, jvb, reward, XRP(20)));
        mcEnv.close();

        scEnv(jtx::signers(door, quorum, signers));
        scEnv(bridgeCreate(door, jvb, reward, XRP(20)));
        scEnv.close();

        scEnv(sponsor::set_reserve(sponsor, 0, 1), sponsor::SponseeAcc(submitter));
        scEnv.close();

        std::uint32_t constexpr redirectedClaims = 12;
        auto const baseReserve = reserve(scEnv, 1);
        auto const backedForRedirectedClaims = reserve(scEnv, 1 + redirectedClaims);

        adjustAccountXRPBalance(scEnv, sponsor, drops(backedForRedirectedClaims));

        auto sponsorObj = scEnv.le(keylet::sponsor(sponsor, submitter));
        BEAST_EXPECT(sponsorObj);
        BEAST_EXPECT(sponsorObj->getFieldU32(sfReserveCount) == 1);

        auto const noJournal = beast::Journal{beast::Journal::getNullSink()};
        auto const liquidBefore = xrpLiquid(*scEnv.current(), sponsor.id(), 0, noJournal);
        BEAST_EXPECT(liquidBefore == backedForRedirectedClaims - baseReserve);

        auto const submitterOwnerCountBefore = scEnv.ownerCount(submitter);
        auto const submitterSponsoredOwnerCountBefore = scEnv.sponsoredOwnerCount(submitter);
        auto const doorOwnerCountBefore = scEnv.ownerCount(door);
        auto const doorSponsoredOwnerCountBefore = scEnv.sponsoredOwnerCount(door);
        auto const sponsorSponsoringOwnerCountBefore = scEnv.sponsoringOwnerCount(sponsor);

        scEnv(
            createAccountAttestation(
                submitter,
                jvb,
                mcAlice,
                XRP(20),
                reward,
                payees[0],
                true,
                1,
                scuAlice,
                witnessSigner),
            sponsor::As(sponsor, spfSponsorReserve),
            Ter(tesSUCCESS));
        scEnv.close();

        auto const claim1 = scEnv.le(keylet::xChainCreateAccountClaimID(STXChainBridge(jvb), 1));
        sponsorObj = scEnv.le(keylet::sponsor(sponsor, submitter));
        BEAST_EXPECT(claim1);
        BEAST_EXPECT(sponsorObj);
        BEAST_EXPECT((*claim1)[sfAccount] == door.id());
        BEAST_EXPECT(!claim1->isFieldPresent(sfSponsor));
        BEAST_EXPECT(sponsorObj->getFieldU32(sfReserveCount) == 1);
        BEAST_EXPECT(scEnv.ownerCount(submitter) == submitterOwnerCountBefore);
        BEAST_EXPECT(scEnv.sponsoredOwnerCount(submitter) == submitterSponsoredOwnerCountBefore);
        BEAST_EXPECT(scEnv.ownerCount(door) == doorOwnerCountBefore + 1);
        BEAST_EXPECT(scEnv.sponsoredOwnerCount(door) == doorSponsoredOwnerCountBefore);
        BEAST_EXPECT(scEnv.sponsoringOwnerCount(sponsor) == sponsorSponsoringOwnerCountBefore);

        auto const liquidAfterFirst = xrpLiquid(*scEnv.current(), sponsor.id(), 0, noJournal);
        BEAST_EXPECT(liquidAfterFirst == backedForRedirectedClaims - reserve(scEnv, 1));

        for (std::uint32_t i = 2; i <= redirectedClaims; ++i)
        {
            scEnv(
                createAccountAttestation(
                    submitter,
                    jvb,
                    mcBob,
                    XRP(20),
                    reward,
                    payees[(i - 1) % payees.size()],
                    true,
                    i,
                    scuBob,
                    witnessSigner),
                sponsor::As(sponsor, spfSponsorReserve),
                Ter(tesSUCCESS));
            scEnv.close();

            auto const claim = scEnv.le(keylet::xChainCreateAccountClaimID(STXChainBridge(jvb), i));
            sponsorObj = scEnv.le(keylet::sponsor(sponsor, submitter));
            BEAST_EXPECT(claim);
            BEAST_EXPECT(sponsorObj);
            BEAST_EXPECT((*claim)[sfAccount] == door.id());
            BEAST_EXPECT(!claim->isFieldPresent(sfSponsor));
            BEAST_EXPECT(sponsorObj->getFieldU32(sfReserveCount) == 1);
        }

        BEAST_EXPECT(scEnv.ownerCount(door) == doorOwnerCountBefore + redirectedClaims);
        BEAST_EXPECT(scEnv.sponsoredOwnerCount(door) == doorSponsoredOwnerCountBefore);
        BEAST_EXPECT(scEnv.sponsoringOwnerCount(sponsor) == sponsorSponsoringOwnerCountBefore);

        auto const liquidAfterAll = xrpLiquid(*scEnv.current(), sponsor.id(), 0, noJournal);
        BEAST_EXPECT(liquidAfterAll > reserve(scEnv, 1));

        scEnv(pay(sponsor, scBob, drops(1)), Fee(scEnv.current()->fees().base));
        scEnv.close();
    }

    void
    test1186AMMCreateUsesPreFeeReserveBalance()
    {
        // https://github.com/sherlock-audit/2026-04-xrp-ledger-april-2026-judging/issues/1186

        testcase("AMMCreate creates undercollateralized LP-token trustline");

        using namespace jtx;

        auto const run = [&](bool exploitPath) {
            Env env{*this, testableAmendments()};
            auto const fee = env.current()->fees().base;
            auto const ownerIncrement = env.current()->fees().increment;

            Account const gw{"gw"};
            Account const issuer{"issuer"};
            Account const alice{"alice"};
            auto const USD = issuer["USD"];

            env.fund(XRP(100'000), gw, issuer, alice);
            env(fset(issuer, asfDefaultRipple));
            env.close();

            env.trust(USD(50'000), alice);
            env(pay(issuer, alice, USD(30'000)));
            env.close();

            MPTTester BTC(
                {.env = env,
                 .issuer = gw,
                 .holders = {alice},
                 .pay = 30'000,
                 .flags = kMptDexFlags});

            BEAST_EXPECT(ownerCount(env, alice) == 2);
            BEAST_EXPECT(env.le(keylet::line(alice, USD)) != nullptr);
            BEAST_EXPECT(env.le(keylet::mptoken(BTC.issuanceID(), alice)) != nullptr);

            STAmount const preTrimBalance = env.balance(alice, XRP);
            STAmount const targetBalance =
                exploitPath ? reserve(env, 3) : reserve(env, 3) - drops(1);
            STAmount const trimAmount = preTrimBalance - targetBalance - fee;

            BEAST_EXPECT(trimAmount > XRP(0));
            env(pay(alice, issuer, trimAmount));
            env.close();

            BEAST_EXPECT(env.balance(alice, XRP) == targetBalance);
            BEAST_EXPECT(ownerCount(env, alice) == 2);

            if (!exploitPath)
            {
                AMM noCreate(env, alice, USD(10'000), BTC(10'000), Ter(tecINSUF_RESERVE_LINE));
                BEAST_EXPECT(!noCreate.ammExists());
                BEAST_EXPECT(ownerCount(env, alice) == 2);
                BEAST_EXPECT(env.balance(alice, XRP) == targetBalance - ownerIncrement);
                return;
            }

            AMM amm(env, alice, USD(10'000), BTC(10'000));

            auto const lpLine = env.le(keylet::line(alice, amm.lptIssue()));
            // log << "ammcreate_balance=" << env.balance(alice, XRP)
            //     << " owners=" << ownerCount(env, alice) << " lpLineExists=" << (lpLine !=
            //     nullptr)
            //     << std::endl;

            BEAST_EXPECT(amm.ammExists());
            BEAST_EXPECT(lpLine != nullptr);
            BEAST_EXPECT(ownerCount(env, alice) == 3);
            BEAST_EXPECT(env.balance(alice, XRP) == targetBalance - ownerIncrement);
            BEAST_EXPECT(env.balance(alice, XRP) < reserve(env, 3));
        };

        run(false);
        run(true);
    }

    void
    test1186AMMDepositUsesPreFeeReserveBalance()
    {
        // https://github.com/sherlock-audit/2026-04-xrp-ledger-april-2026-judging/issues/1186
        // TEAM DECISION: considered as not a bug, see TicketCreate::doApply():
        //      "we want to allow dipping into the reserve to pay fees"
        //      As designed

        testcase("AMMDeposit creates undercollateralized LP-token trustline");

        using namespace jtx;

        auto const run = [&](bool exploitPath) {
            Env env{*this, testableAmendments()};
            auto const fee = env.current()->fees().base;

            Account const gw{"gw"};
            Account const issuer{"issuer"};
            Account const alice{"alice"};
            Account const bob{"bob"};
            auto const USD = issuer["USD"];

            env.fund(XRP(100'000), gw, issuer, alice, bob);
            env(fset(issuer, asfDefaultRipple));
            env.close();

            env.trust(USD(50'000), alice);
            env.trust(USD(50'000), bob);
            env(pay(issuer, alice, USD(30'000)));
            env(pay(issuer, bob, USD(30'000)));
            env.close();

            MPTTester BTC(
                {.env = env,
                 .issuer = gw,
                 .holders = {alice, bob},
                 .pay = 30'000,
                 .flags = kMptDexFlags});

            AMM amm(env, alice, USD(10'000), BTC(10'000));
            BEAST_EXPECT(amm.ammExists());

            BEAST_EXPECT(ownerCount(env, bob) == 2);
            BEAST_EXPECT(env.le(keylet::line(bob, amm.lptIssue())) == nullptr);

            STAmount const preTrimBalance = env.balance(bob, XRP);
            STAmount const targetBalance =
                exploitPath ? reserve(env, 3) : reserve(env, 3) - drops(1);
            STAmount const trimAmount = preTrimBalance - targetBalance - fee;

            BEAST_EXPECT(trimAmount > XRP(0));
            env(pay(bob, issuer, trimAmount));
            env.close();

            BEAST_EXPECT(env.balance(bob, XRP) == targetBalance);
            BEAST_EXPECT(ownerCount(env, bob) == 2);

            if (!exploitPath)
            {
                amm.deposit(
                    {.account = bob,
                     .asset1In = USD(1'000),
                     .asset2In = BTC(1'000),
                     .flags = tfTwoAsset,
                     .err = Ter(tecINSUF_RESERVE_LINE)});

                BEAST_EXPECT(env.le(keylet::line(bob, amm.lptIssue())) == nullptr);
                BEAST_EXPECT(ownerCount(env, bob) == 2);
                BEAST_EXPECT(env.balance(bob, XRP) == targetBalance - fee);
                return;
            }

            amm.deposit(
                {.account = bob,
                 .asset1In = USD(1'000),
                 .asset2In = BTC(1'000),
                 .flags = tfTwoAsset});

            auto const lpLine = env.le(keylet::line(bob, amm.lptIssue()));
            // log << "ammdeposit_balance=" << env.balance(bob, XRP)
            //     << " owners=" << ownerCount(env, bob) << " lpLineExists=" << (lpLine != nullptr)
            //     << " lpTokens=" << amm.getLPTokensBalance(bob.id()) << std::endl;

            BEAST_EXPECT(lpLine != nullptr);
            BEAST_EXPECT(ownerCount(env, bob) == 3);
            BEAST_EXPECT(amm.getLPTokensBalance(bob.id()) > beast::kZero);
            BEAST_EXPECT(env.balance(bob, XRP) == targetBalance - fee);
            BEAST_EXPECT(env.balance(bob, XRP) < reserve(env, 3));
        };

        run(false);
        run(true);
    }

    void
    test1350ReserveCountSilentWrap(FeatureBitset features)
    {
        // https://github.com/sherlock-audit/2026-04-xrp-ledger-april-2026-judging/issues/1350

        testcase(
            "adjustReserveCount uint32 wraparound silently erases pool quota at UINT32_MAX "
            "boundary");

        using namespace test::jtx;

        Env env{*this, features};
        Account const sponsor{"sponsor"};
        Account const sponsee{"sponsee"};
        env.fund(XRP(10000), sponsor, sponsee);
        env.close();

        std::uint32_t const uint32Max = std::numeric_limits<std::uint32_t>::max();

        // STEP 1: Sponsee creates a Check co-signed by sponsor BEFORE any sponsorship
        // exists. adjustOwnerCount in CheckCreate peeks keylet::sponsor(S, B)
        // — finds nothing — so the pool quota is NOT debited. The resulting
        // ltCHECK carries sfSponsor = sponsor.
        auto const sponseeSeq = env.seq(sponsee);
        uint256 const checkID = keylet::check(sponsee, sponseeSeq).key;
        if (!BEAST_EXPECT(checkID.isNonZero()))
            return;

        env(check::create(sponsee, sponsor, XRP(1)),
            sponsor::As(sponsor, spfSponsorReserve),
            Sig(sfSponsorSignature, sponsor));
        env.close();

        // STEP 2: Sponsor (or a SponsorReserve-narrowed delegate) creates the
        // ltSPONSORSHIP pool with sfReserveCount at the UINT32_MAX boundary.
        // Because the Check was already created before this pool, no deduction
        // has occurred — the pool starts at exactly UINT32_MAX.
        env(sponsor::set(sponsor, 0, uint32Max), sponsor::SponseeAcc(sponsee));
        env.close();

        // VERIFY 1: pool exists with sfReserveCount == UINT32_MAX
        {
            json::Value const poolEntry = sponsor::ledgerEntry(env, sponsor, sponsee);
            auto const& node = poolEntry[jss::result][jss::node];
            BEAST_EXPECT(node.isMember(sfReserveCount.jsonName)) &&
                BEAST_EXPECT(node[sfReserveCount.jsonName].asUInt() == uint32Max);
        }

        // STEP 3: Sponsee ends the sponsorship on the Check. The End's payback
        // at SponsorshipTransfer.cpp:520-528 calls adjustReserveCount(+1) with
        // sfReserveCount = UINT32_MAX. The uint32 addition wraps to 0, int32_t
        // conversion gives 0 (not negative — guard NOT triggered), and the
        // code calls makeFieldAbsent — silently erasing the entire quota.

        // Updated flow: sfReserveCount adjustment can only decrease. Freeing object doesn't reset
        // sfReserveCount
        env(sponsor::transfer(sponsee, tfSponsorshipEnd, checkID));
        env.close();

        // VERIFY 2: silent wrap — sfReserveCount is now ABSENT
        // Present after fix.

        {
            json::Value const poolEntry = sponsor::ledgerEntry(env, sponsor, sponsee);
            auto const& node = poolEntry[jss::result][jss::node];
            BEAST_EXPECT(node.isMember(sfReserveCount.jsonName)) &&
                BEAST_EXPECTS(
                    node[sfReserveCount.jsonName].asUInt() == uint32Max,
                    std::to_string(node[sfReserveCount.jsonName].asUInt()));
        }

        // VERIFY 3: pool is bricked — future drawdowns fail with
        // tecINSUFFICIENT_RESERVE (sfReserveCount absent = 0 < ownerCountDelta
        // = 1 at checkInsufficientReserve, preclaim). The Check is now
        // unsponsored (sfSponsor removed in STEP 3); attempt to re-sponsor it.

        // ALREADY FIXED
        env(sponsor::transfer(sponsee, tfSponsorshipCreate, checkID),
            sponsor::As(sponsor, spfSponsorReserve));
        env.close();
    }

    void
    test1364AmmWithdrawSponsoredMptBypass()
    {
        // https://github.com/sherlock-audit/2026-04-xrp-ledger-april-2026-judging/issues/1364

        // AMMWithdraw::equalWithdrawTokens() misses prefunded-sponsor reserve check,
        // allowing a sponsee to create a sponsored MPToken without consuming sfReserveCount
        // when the sponsor's owner count is below 2.
        testcase("AMMWithdraw bypasses prefunded sponsor ReserveCount for MPT creation");

        using namespace test::jtx;

        Env env{*this, testableAmendments()};

        Account const issuer{"issuer"};
        Account const alice{"alice"};
        Account const bob{"bob"};
        Account const sponsor{"sponsor"};

        // Fund all accounts with sufficient XRP
        env.fund(XRP(1000), issuer, alice, bob, sponsor);
        env.close();

        // Drain sponsor (just enough for a fee-only sponsorship but not reserve
        // sponsorship)
        adjustAccountXRPBalance(
            env, sponsor, reserve(env, 1) + XRP(1) + env.current()->fees().base);
        // auto const sponsorSle = env.le(sponsor);
        // log << "Sponsor balance: " << sponsorSle->at(sfBalance) << std::endl;

        // Get initial sponsor state
        auto const sponsoringOwnerCountBefore = env.sponsoringOwnerCount(sponsor);

        // Step 1: Create MPT with lsfMPTCanTrade | lsfMPTCanTransfer flags. Both are needed for AMM
        // trading. MPTokenIssuanceCreate
        MPTTester mpt(
            {.env = env, .issuer = issuer, .flags = kMptDexFlags, .maxAmt = 1000'000'000});
        env.close();

        // Step 2: MPTokenAuthorize and pay to alice
        mpt.authorize({.account = alice, .id = mpt.issuanceID()});
        env.close();
        env(pay(issuer, alice, mpt(20'000)));
        env.close();

        // Step 3: Alice creates an AMM pool with XRP and MPT, fee = 2XRP
        AMM amm(
            env, alice, XRP(10), mpt(10'000), false, 0, env.current()->fees().increment.drops());
        env.close();
        BEAST_EXPECT(amm.expectTradingFee(0));

        // Step 3: Bob deposits into AMM to get LP tokens, Bob needs LP tokens to be able to
        // withdraw
        auto const jv1 = amm.depositJv(
            {.account = bob, .asset1In = XRP(1), .flags = tfSingleAsset, .assets = {{XRP, mpt}}});
        // log << jv1.toStyledString() << std::endl;
        env(jv1);
        env.close();

        // Verify Bob does NOT have an MPToken yet
        BEAST_EXPECT(env.le(keylet::mptoken(mpt.issuanceID(), bob)) == nullptr);

        // Step 4: Sponsor creates a fee-only sponsorship for Bob (no ReserveCount)
        // log << "Sponsor balance: " << sponsorSle->at(sfBalance) << std::endl;
        env(sponsor::set_fee(sponsor, 0, drops(1'000'000)), sponsor::SponseeAcc(bob));
        env.close();

        // Verify sponsorship exists with FeeAmount but no ReserveCount
        {
            auto const sle = env.le(keylet::sponsor(sponsor, bob));
            BEAST_EXPECT(sle) && BEAST_EXPECT(sle->isFieldPresent(sfFeeAmount)) &&
                BEAST_EXPECT(!sle->isFieldPresent(sfReserveCount));
        }

        // Control: the regular MPTokenAuthorize path correctly treats a prefunded
        // sponsor as requiring ReserveCount even while the sponsor's owner count is
        // below the free-object threshold.
        env(mpt.authorizeJV({.account = bob, .id = mpt.issuanceID()}),
            sponsor::As(sponsor, spfSponsorReserve),
            Ter(tecINSUFFICIENT_RESERVE));
        env.close();

        // Step 6: Exploit - AMMWithdraw with prefunded sponsor succeeds
        // This should also fail with tecINSUFFICIENT_RESERVE but the bug allows it to succeed
        // Fixed, failed
        env(amm.withdrawJv(
                WithdrawArg{
                    .account = bob,
                    .asset1Out = mpt(1),
                    .flags = tfSingleAsset,
                    .assets = {{mpt, XRP}}}),
            sponsor::As(sponsor, spfSponsorReserve),
            Ter(tecINSUFFICIENT_RESERVE));
        env.close();

        // Verify the bug: MPToken was created without consuming ReserveCount
        // FIXED
        auto const mptokenAfter = env.le(keylet::mptoken(mpt.issuanceID(), bob));
        BEAST_EXPECT(!mptokenAfter);
        // && BEAST_EXPECT(mptokenAfter->isFieldPresent(sfSponsor)) &&
        // BEAST_EXPECT(mptokenAfter->getAccountID(sfSponsor) == sponsor.id());

        // Verify sponsorship still has no ReserveCount
        {
            auto const sle = env.le(keylet::sponsor(sponsor, bob));
            BEAST_EXPECT(sle) && BEAST_EXPECT(!sle->isFieldPresent(sfReserveCount));
        }

        // Verify Bob's SponsoredOwnerCount increased
        // fixed
        {
            auto const bobAfter = env.le(keylet::account(bob));
            BEAST_EXPECT(bobAfter) &&
                BEAST_EXPECT(bobAfter->getFieldU32(sfSponsoredOwnerCount) == 0);
        }

        // Verify sponsor's SponsoringOwnerCount increased
        {
            auto const sponsorAfter = env.le(keylet::account(sponsor));
            BEAST_EXPECT(sponsorAfter) &&
                BEAST_EXPECT(
                    sponsorAfter->getFieldU32(sfSponsoringOwnerCount) ==
                    sponsoringOwnerCountBefore);

            // Verify sponsor is not under-reserved
            // Sponsor should need: base(10 XRP) + sponsoringOwnerCount(1) * inc(2 XRP) = 12 XRP
            auto const sponsorBalance = sponsorAfter->getFieldAmount(sfBalance);
            auto const requiredReserve = accountReserve(*env.current(), sponsorAfter, env.journal);
            BEAST_EXPECT(sponsorBalance >= requiredReserve);
        }
    }

public:
    void
    run() override
    {
        using namespace test::jtx;

        test168CoSignedBlockedWithFeeOnlySponsorship();
        test251AMMDepositRejectXRPDeposits();
        test1033SponsoredWitnessCanChargeDoorOwnedClaimObjectsToUnrelatedSponsor();
        test1186AMMCreateUsesPreFeeReserveBalance();
        test1186AMMDepositUsesPreFeeReserveBalance();
        test1350ReserveCountSilentWrap(testableAmendments());
        test1364AmmWithdrawSponsoredMptBypass();
    }
};

BEAST_DEFINE_TESTSUITE(SponsorSherlock, app, xrpl);

}  // namespace xrpl::test
