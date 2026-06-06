#include <xrpl/ledger/helpers/AccountRootHelpers.h>

#include <xrpl/basics/Expected.h>
#include <xrpl/basics/Log.h>
#include <xrpl/basics/base_uint.h>
#include <xrpl/basics/contract.h>
#include <xrpl/beast/utility/Journal.h>
#include <xrpl/beast/utility/Zero.h>
#include <xrpl/beast/utility/instrumentation.h>
#include <xrpl/ledger/ApplyView.h>
#include <xrpl/ledger/ReadView.h>
#include <xrpl/ledger/helpers/SponsorHelpers.h>
#include <xrpl/protocol/AccountID.h>
#include <xrpl/protocol/Feature.h>
#include <xrpl/protocol/Indexes.h>
#include <xrpl/protocol/LedgerFormats.h>
#include <xrpl/protocol/Rate.h>
#include <xrpl/protocol/SField.h>
#include <xrpl/protocol/STLedgerEntry.h>
#include <xrpl/protocol/STTx.h>
#include <xrpl/protocol/TER.h>
#include <xrpl/protocol/XRPAmount.h>
#include <xrpl/protocol/digest.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

namespace xrpl {

bool
isGlobalFrozen(ReadView const& view, AccountID const& issuer)
{
    if (isXRP(issuer))
        return false;
    if (auto const sle = view.read(keylet::account(issuer)))
        return sle->isFlag(lsfGlobalFreeze);
    return false;
}

// An owner count cannot be negative. If ownerCountAdj would cause a negative
// owner count, clamp the owner count at 0. Similarly for overflow. This
// ownerCountAdj allows the ownerCount to be adjusted up or down in multiple steps.
// If id != std::nullopt, then do error reporting.
//
// Returns adjusted owner count.
static std::uint32_t
confineOwnerCount(
    std::uint32_t current,
    std::int32_t ownerCountAdj,
    std::optional<AccountID> const& id = std::nullopt,
    beast::Journal j = beast::Journal{beast::Journal::getNullSink()})
{
    std::uint32_t adjusted{current + ownerCountAdj};
    if (ownerCountAdj > 0)
    {
        // Overflow is well defined on unsigned
        if (adjusted < current)
        {
            if (id)
            {
                JLOG(j.fatal()) << "Account " << *id << " owner count exceeds max!";
            }
            adjusted = std::numeric_limits<std::uint32_t>::max();
        }
    }
    else
    {
        // Underflow is well defined on unsigned
        if (adjusted > current)
        {
            if (id)
            {
                JLOG(j.fatal()) << "Account " << *id << " owner count set below 0!";
            }
            adjusted = 0;
            XRPL_ASSERT(!id, "xrpl::confineOwnerCount : id is not set");
        }
    }
    return adjusted;
}

static std::uint32_t
ownerCountHlp(
    ReadView const& view,
    SLE::const_ref sle,
    std::int32_t ownerCountAdj,
    bool reportConfine,
    beast::Journal j)
{
    AccountID const id = sle->at(sfAccount);
    std::uint32_t const savedCount = sle->at(sfOwnerCount);
    std::uint32_t const hookedCount = view.ownerCountHook(id, savedCount);

    std::uint32_t const sponsoredCount = sle->at(sfSponsoredOwnerCount);
    std::uint32_t const sponsoringCount = sle->at(sfSponsoringOwnerCount);

    if (hookedCount < sponsoredCount)
    {
        Throw<std::logic_error>(
            "xrpl::ownerCountHlp : OwnerCount must be greater than or equal to "
            "SponsoredOwnerCount");
    }

    std::int64_t deltaCount =
        static_cast<std::int64_t>(ownerCountAdj) - sponsoredCount + sponsoringCount;
    if (deltaCount > std::numeric_limits<std::int32_t>::max())
    {
        deltaCount = std::numeric_limits<std::int32_t>::max();
        JLOG(j.error()) << "Account " << id << " adjustment exceeds max, "
                        << "adjustment: " << ownerCountAdj << ", sponsoredCount: " << sponsoredCount
                        << ", sponsoringOwnerCount: " << sponsoringCount;
    }
    else if (deltaCount < std::numeric_limits<std::int32_t>::min())
    {
        deltaCount = std::numeric_limits<std::int32_t>::min();
        JLOG(j.fatal()) << "Account " << id << " adjustment exceeds min, "
                        << "adjustment: " << ownerCountAdj << ", sponsoredCount: " << sponsoredCount
                        << ", sponsoringCount: " << sponsoringCount;
    }

    std::uint32_t const confinedCount = reportConfine
        ? confineOwnerCount(hookedCount, deltaCount, id, j)
        : confineOwnerCount(hookedCount, deltaCount);

    return confinedCount;
}

static std::uint32_t
reserveCountHlp(SLE::const_ref sle, std::int32_t reserveCountAdj, beast::Journal j)
{
    AccountID const id = sle->at(sfAccount);
    bool const isSponsored = sle->isFieldPresent(sfSponsor);
    std::int64_t const sponsoringCount = sle->at(sfSponsoringAccountCount);
    std::int64_t const reserveCount = (isSponsored ? 0 : 1) + sponsoringCount;

    // Fix like confineOwnerCount
    std::int64_t adjusted = reserveCount + reserveCountAdj;
    if (adjusted > std::numeric_limits<std::uint32_t>::max())
    {
        JLOG(j.error()) << "Account " << id << " reserve count exceeds max, "
                        << "adjustment: " << reserveCountAdj
                        << ", sponsoringCount: " << sponsoringCount
                        << ", reserveCount: " << reserveCount;
        adjusted = std::numeric_limits<std::uint32_t>::max();
    }
    else if (adjusted < 0)
    {
        JLOG(j.fatal()) << "Account " << id << " reserve count below 0, "
                        << "adjustment: " << reserveCountAdj
                        << ", sponsoringCount: " << sponsoringCount
                        << ", reserveCount: " << reserveCount;
        adjusted = 0;
    }

    return static_cast<std::uint32_t>(adjusted);
}

static inline XRPAmount
baseReserveHlp(ReadView const& view, std::uint32_t ownerCount, std::uint32_t reserveCount)
{
    auto const& fees = view.fees();
    return (fees.reserve * reserveCount) + (fees.increment * ownerCount);
}

static XRPAmount
reserveHlp(
    ReadView const& view,
    SLE::const_ref sle,
    std::uint32_t ownerCount,
    std::uint32_t reserveCount)
{
    // Pseudo-accounts have no reserve requirement
    if (isPseudoAccount(sle))
        return XRPAmount(0);

    auto const reserve = baseReserveHlp(view, ownerCount, reserveCount);
    return reserve;
}

std::uint32_t
ownerCount(
    ReadView const& view,
    SLE::const_ref accSle,
    beast::Journal j,
    std::int32_t ownerCountAdj)
{
    auto const sleType = accSle->getType();
    bool const validType = sleType == ltLOAN_BROKER || sleType == ltACCOUNT_ROOT;
    if (!validType)
        Throw<std::logic_error>("xrpl::ownerCount: valid sle type");

    return ownerCountHlp(view, accSle, ownerCountAdj, true, j);
}

static STAmount
xrpLiquidHlp(
    ReadView const& view,
    AccountID const& id,
    SLE::const_ref accSle,
    std::int32_t ownerCountAdj,
    std::int32_t reserveCountAdj,
    beast::Journal j)
{
    if (!accSle)
        return beast::kZero;

    std::uint32_t const ownerCount = ownerCountHlp(view, accSle, ownerCountAdj, false, j);
    std::uint32_t const reserveCount = reserveCountHlp(accSle, reserveCountAdj, j);
    auto const reserve = reserveHlp(view, accSle, ownerCount, reserveCount);

    auto const fullBalance = accSle->at(sfBalance);
    auto const balance = view.balanceHookIOU(id, xrpAccount(), fullBalance);

    STAmount const amount = balance - reserve;

    JLOG(j.trace()) << "accountHolds:" << " account=" << to_string(id)
                    << " amount=" << amount.getFullText()
                    << " fullBalance=" << fullBalance.getFullText()
                    << " balance=" << balance.getFullText() << " reserve=" << reserve
                    << " ownerCount=" << ownerCount << " ownerCountAdj=" << ownerCountAdj;

    return amount;
}

XRPAmount
xrpLiquid(ReadView const& view, AccountID const& id, std::int32_t ownerCountAdj, beast::Journal j)
{
    auto const accSle = view.read(keylet::account(id));
    if (!accSle)
        return beast::kZero;

    auto const x = xrpLiquidHlp(view, id, accSle, ownerCountAdj, 0, j);
    return x.negative() ? XRPAmount(beast::kZero) : x.xrp();
}

Rate
transferRate(ReadView const& view, AccountID const& issuer)
{
    auto const sle = view.read(keylet::account(issuer));

    if (sle && sle->isFieldPresent(sfTransferRate))
        return Rate{sle->getFieldU32(sfTransferRate)};

    return kParityRate;
}

static void
adjustOwnerCountHlp(
    ApplyView& view,
    SLE::ref sle,
    SF_UINT32 const& sfield,
    AccountID const& accID,
    std::int32_t ownerCountAdj,
    beast::Journal j,
    bool callHook = true)
{
    std::uint32_t const current = sle->at(sfield);
    std::uint32_t const adjusted = confineOwnerCount(current, ownerCountAdj, accID, j);
    if (callHook)
        view.adjustOwnerCountHook(accID, current, adjusted);
    sle->at(sfield) = adjusted;
    view.update(sle);
}

void
adjustOwnerCount(
    ApplyView& view,
    SLE::ref accountSle,
    SLE::ref sponsorSle,
    std::int32_t ownerCountAdj,
    beast::Journal j)
{
    if (!accountSle)
        Throw<std::runtime_error>("xrpl::adjustOwnerCount : valid account sle");

    auto const sleType = accountSle->getType();
    bool const validType = sponsorSle ? sleType == ltACCOUNT_ROOT
                                      : sleType == ltLOAN_BROKER || sleType == ltACCOUNT_ROOT;
    if (!validType)
        Throw<std::logic_error>("xrpl::adjustOwnerCount : valid account sle type");

    XRPL_ASSERT(ownerCountAdj, "xrpl::adjustOwnerCount : nonzero adjustment input");
    if (ownerCountAdj == 0)
        return;

    auto const accountID = accountSle->getAccountID(sfAccount);
    if (sponsorSle)
    {
        if (sponsorSle->getType() != ltACCOUNT_ROOT)
            Throw<std::logic_error>("xrpl::adjustOwnerCount : valid sponsor sle type");
        auto const sponsorID = sponsorSle->getAccountID(sfAccount);

        adjustOwnerCountHlp(view, accountSle, sfSponsoredOwnerCount, accountID, ownerCountAdj, j);
        adjustOwnerCountHlp(view, sponsorSle, sfSponsoringOwnerCount, sponsorID, ownerCountAdj, j);

        auto sponsorshipSle = view.peek(keylet::sponsor(sponsorID, accountID));
        if (sponsorshipSle && ownerCountAdj > 0)
        {
            // update the pre-funded ReserveCount on Sponsorship ledger object
            // Reserve count moves opposite to adjustment: +adjustment => consume reserve (-),
            adjustOwnerCountHlp(
                view, sponsorshipSle, sfReserveCount, sponsorID, -ownerCountAdj, j, false);
        }
    }
    adjustOwnerCountHlp(view, accountSle, sfOwnerCount, accountID, ownerCountAdj, j);
}

void
adjustOwnerCountObj(
    ApplyView& view,
    SLE::ref accountSle,
    SLE::ref objectSle,
    std::int32_t ownerCountAdj,
    beast::Journal j)
{
    if (!objectSle)
        Throw<std::runtime_error>("xrpl::adjustOwnerCount : valid object sle");
    if (objectSle->getType() == ltACCOUNT_ROOT)
        Throw<std::logic_error>("xrpl::adjustOwnerCount : valid object sle type");

    SLE::ref sponsorSle = getLedgerEntryReserveSponsor(view, objectSle);
    adjustOwnerCount(view, accountSle, sponsorSle, ownerCountAdj, j);
}

XRPAmount
accountReserve(
    ReadView const& view,
    SLE::const_ref sle,
    beast::Journal j,
    std::int32_t ownerCountAdj,
    std::int32_t reserveCountAdj)
{
    if (!sle)
        Throw<std::runtime_error>("xrpl::accountReserve : valid sle");
    if (sle->getType() != ltACCOUNT_ROOT)
        Throw<std::logic_error>("xrpl::accountReserve : valid sle type");

    std::uint32_t const ownerCount = ownerCountHlp(view, sle, ownerCountAdj, true, j);
    std::uint32_t const reserveCount = reserveCountHlp(sle, reserveCountAdj, j);

    return reserveHlp(view, sle, ownerCount, reserveCount);
}

XRPAmount
baseAccountReserve(ReadView const& view, std::int32_t ownerCount)
{
    auto const reserve = baseReserveHlp(view, ownerCount, 1);
    return reserve;
}

TER
checkInsufficientReserve(
    ReadView const& view,
    STTx const& tx,
    SLE::const_ref accSle,
    STAmount const& accBalance,
    SLE::const_ref sponsorSle,
    std::int32_t ownerCountAdj,
    std::int32_t reserveCountAdj,
    beast::Journal j)
{
    if (sponsorSle)
    {
        auto const isCoSigning = isSponsorReserveCoSigning(tx);
        auto const sponsorshipSle =
            view.read(keylet::sponsor(sponsorSle->at(sfAccount), accSle->at(sfAccount)));

        // prefunded sponsor should have a sponsorship entry
        if (!isCoSigning && !sponsorshipSle)
            return tecINTERNAL;  // LCOV_EXCL_LINE

        // If sponsorship doesn't have enough ownerCount, we still can try fallback directly to
        // sponsor if tx reserve co-signed.
        if (sponsorshipSle)  // && !isCoSigning ??
        {
            auto const remainingOwnerCount = sponsorshipSle->at(sfReserveCount);
            if ((remainingOwnerCount < ownerCountAdj))
                return tecINSUFFICIENT_RESERVE;
        }

        auto const sponsorBalance = sponsorSle->getFieldAmount(sfBalance);
        STAmount const sponsorReserve =
            accountReserve(view, sponsorSle, j, ownerCountAdj, reserveCountAdj);

        if (sponsorBalance < sponsorReserve)
            return tecINSUFFICIENT_RESERVE;
    }
    else
    {
        STAmount const reserve = accountReserve(view, accSle, j, ownerCountAdj, reserveCountAdj);
        if (accBalance < reserve)
            return tecINSUFFICIENT_RESERVE;
    }
    return tesSUCCESS;
}

// ----------------------------------------------------

AccountID
pseudoAccountAddress(ReadView const& view, uint256 const& pseudoOwnerKey)
{
    // This number must not be changed without an amendment
    static constexpr std::uint16_t kMaxAccountAttempts = 256;
    for (std::uint16_t i = 0; i < kMaxAccountAttempts; ++i)
    {
        RipeshaHasher rsh;
        auto const hash = sha512Half(i, view.header().parentHash, pseudoOwnerKey);
        rsh(hash.data(), hash.size());
        AccountID const ret = AccountID::fromRaw(static_cast<RipeshaHasher::result_type>(rsh));
        if (!view.read(keylet::account(ret)))
            return ret;
    }
    return beast::kZero;
}

// Pseudo-account designator fields MUST be maintained by including the
// SField::sMD_PseudoAccount flag in the SField definition. (Don't forget to
// "| SField::sMD_Default"!) The fields do NOT need to be amendment-gated,
// since a non-active amendment will not set any field, by definition.
// Specific properties of a pseudo-account are NOT checked here, that's what
// InvariantCheck is for.
[[nodiscard]] std::vector<SField const*> const&
getPseudoAccountFields()
{
    static std::vector<SField const*> const kPseudoFields = []() {
        auto const ar = LedgerFormats::getInstance().findByType(ltACCOUNT_ROOT);
        if (!ar)
        {
            // LCOV_EXCL_START
            Throw<std::logic_error>(
                "xrpl::getPseudoAccountFields : unable to find account root "
                "ledger format");
            // LCOV_EXCL_STOP
        }
        auto const& soTemplate = ar->getSOTemplate();

        std::vector<SField const*> pseudoFields;
        for (auto const& field : soTemplate)
        {
            if (field.sField().shouldMeta(SField::kSmdPseudoAccount))
                pseudoFields.emplace_back(&field.sField());
        }
        return pseudoFields;
    }();
    return kPseudoFields;
}

[[nodiscard]] bool
isPseudoAccount(SLE::const_ref sleAcct, std::set<SField const*> const& pseudoFieldFilter)
{
    auto const& fields = getPseudoAccountFields();

    // Intentionally use defensive coding here because it's cheap and makes the
    // semantics of true return value clean.
    return sleAcct && sleAcct->getType() == ltACCOUNT_ROOT &&
        std::count_if(
            fields.begin(), fields.end(), [&sleAcct, &pseudoFieldFilter](SField const* sf) -> bool {
                return sleAcct->isFieldPresent(*sf) &&
                    (pseudoFieldFilter.empty() || pseudoFieldFilter.contains(sf));
            }) > 0;
}

Expected<SLE::pointer, TER>
createPseudoAccount(ApplyView& view, uint256 const& pseudoOwnerKey, SField const& ownerField)
{
    [[maybe_unused]]
    auto const& fields = getPseudoAccountFields();
    XRPL_ASSERT(
        std::count_if(
            fields.begin(),
            fields.end(),
            [&ownerField](SField const* sf) -> bool { return *sf == ownerField; }) == 1,
        "xrpl::createPseudoAccount : valid owner field");

    auto const accountId = pseudoAccountAddress(view, pseudoOwnerKey);
    if (accountId == beast::kZero)
        return Unexpected(tecDUPLICATE);

    // Create pseudo-account.
    auto account = std::make_shared<SLE>(keylet::account(accountId));
    account->setAccountID(sfAccount, accountId);
    account->setFieldAmount(sfBalance, STAmount{});

    // Pseudo-accounts can't submit transactions, so set the sequence number
    // to 0 to make them easier to spot and verify, and add an extra level
    // of protection.
    std::uint32_t const seqno =                           //
        view.rules().enabled(featureSingleAssetVault) ||  //
            view.rules().enabled(featureLendingProtocol)  //
        ? 0                                               //
        : view.seq();
    account->setFieldU32(sfSequence, seqno);
    // Ignore reserves requirement, disable the master key, allow default
    // rippling, and enable deposit authorization to prevent payments into
    // pseudo-account.
    account->setFieldU32(sfFlags, lsfDisableMaster | lsfDefaultRipple | lsfDepositAuth);
    // Link the pseudo-account with its owner object.
    account->setFieldH256(ownerField, pseudoOwnerKey);

    view.insert(account);

    return account;
}

[[nodiscard]] TER
checkDestinationAndTag(SLE::const_ref toSle, bool hasDestinationTag)
{
    if (toSle == nullptr)
        return tecNO_DST;

    // The tag is basically account-specific information we don't
    // understand, but we can require someone to fill it in.
    if (toSle->isFlag(lsfRequireDestTag) && !hasDestinationTag)
        return tecDST_TAG_NEEDED;  // Cannot send without a tag

    return tesSUCCESS;
}

}  // namespace xrpl
