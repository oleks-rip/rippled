//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2022 Ripple Labs Inc.

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

#include <ripple/app/paths/Flow.h>
#include <ripple/app/tx/impl/SignerEntries.h>
#include <ripple/app/tx/impl/Transactor.h>
#include <ripple/app/tx/impl/XChainBridge.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/XRPAmount.h>
#include <ripple/basics/chrono.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/ledger/ApplyView.h>
#include <ripple/ledger/PaymentSandbox.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/AccountID.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/SField.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/protocol/STObject.h>
#include <ripple/protocol/STXChainAttestationBatch.h>
#include <ripple/protocol/STXChainBridge.h>
#include <ripple/protocol/TER.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/protocol/XChainAttestations.h>
#include <ripple/protocol/digest.h>
#include <ripple/protocol/st.h>

#include <unordered_map>
#include <unordered_set>

namespace ripple {

/*
   Bridges connect two independent ledgers: a "locking chain" and an "issuing
   chain". An asset can be moved from the locking chain to the issuing chain by
   putting it into trust on the locking chain, and issuing a "wrapped asset"
   that represents the locked asset on the issuing chain.

   Note that a bridge is not an exchange. There is no exchange rate: one wrapped
   asset on the issuing chain always represents one asset in trust on the
   locking chain. The bridge also does not exchange an asset on the locking
   chain for an asset on the issuing chain.

   A good model for thinking about bridges is a box that contains an infinite
   number of "wrapped tokens". When a token from the locking chain
   (locking-chain-token) is put into the box, a wrapped token is taken out of
   the box and put onto the issuing chain (issuing-chain-token). No one can use
   the locking-chain-token while it remains in the box. When an
   issuing-chain-token is returned to the box, one locking-chain-token is taken
   out of the box and put back onto the locking chain.

   This requires a way to put assets into trust on one chain (put a
   locking-chain-token into the box). A regular XRP account is used for this.
   This account is called a "door account". Much in the same way that a door is
   used to go from one room to another, a door account is used to move from one
   chain to another. This account will be jointly controlled by a set of witness
   servers by using the ledger's multi-signature support. The master key will be
   disabled. These witness servers are trusted in the sense that if a quorum of
   them collude, they can steal the funds put into trust.

   This also requires a way to prove that assets were put into the box - either
   a locking-chain-token on the locking chain or returning an
   issuing-chain-token on the issuing chain. A set of servers called "witness
   servers" fill this role. These servers watch the ledger for these
   transactions, and attest that the given events happened on the different
   chains by signing messages with the event information.

   There needs to be a way to prevent the attestations from the witness
   servers from being used more than once. "Claim ids" fill this role. A claim
   id must be acquired on the destination chain before the asset is "put into
   the box" on the source chain. This claim id has a unique id, and once it is
   destroyed it can never exist again (it's a simple counter). The attestations
   reference this claim id, and are accumulated on the claim id. Once a quorum
   is reached, funds can move. Once the funds move, the claim id is destroyed.

   Finally, a claim id requires that the sender has an account on the
   destination chain. For some chains, this can be a problem - especially if
   the wrapped asset represents XRP, and XRP is needed to create an account.
   There's a bootstrap problem. To address this, there is a special transaction
   used to create accounts. This transaction does not require a claim id.

   See the document "docs/bridge/spec.md" for a full description of how
   bridges and their transactions work.
*/

namespace {

enum class TransferHelperCanCreateDst { no, yes };

/** Transfer funds from the src account to the dst account

    @param psb The payment sandbox.
    @param src The source of funds.
    @param dst The destination for funds.
    @param dstTag Integer destination tag. Used to check if funds should be
           transferred to an account with a `RequireDstTag` flag set.
    @param claimOwner Owner of the claim ledger object.
    @param amt Amount to transfer from the src account to the dst account.
    @param canCreate Flag to determine if accounts may be created using this
   transfer.
    @param j Log

    @return tesSUCCESS if payment succeeds, otherwise the error code for the
            failure reason.
 */

TER
transferHelper(
    PaymentSandbox& psb,
    AccountID const& src,
    AccountID const& dst,
    std::optional<std::uint32_t> const& dstTag,
    std::optional<AccountID> const& claimOwner,
    STAmount const& amt,
    TransferHelperCanCreateDst canCreate,
    beast::Journal j)
{
    if (dst == src)
        return tesSUCCESS;

    auto const dstK = keylet::account(dst);
    if (auto sleDst = psb.read(dstK))
    {
        // Check dst tag and deposit auth

        if ((sleDst->getFlags() & lsfRequireDestTag) && !dstTag)
            return tecDST_TAG_NEEDED;

        if ((dst != claimOwner) && (sleDst->getFlags() & lsfDepositAuth) &&
            (!psb.exists(keylet::depositPreauth(dst, src))))
        {
            return tecNO_PERMISSION;
        }
    }
    else if (!amt.native() || canCreate == TransferHelperCanCreateDst::no)
    {
        return tecNO_DST;
    }

    if (amt.native())
    {
        auto const sleSrc = psb.peek(keylet::account(src));
        assert(sleSrc);
        if (!sleSrc)
            return tecINTERNAL;

        {
            auto const ownerCount = sleSrc->getFieldU32(sfOwnerCount);
            auto const reserve = psb.fees().accountReserve(ownerCount);

            if ((*sleSrc)[sfBalance] < amt + reserve)
            {
                return tecINSUFFICIENT_FUNDS;
            }
        }

        auto sleDst = psb.peek(dstK);
        if (!sleDst)
        {
            if (canCreate == TransferHelperCanCreateDst::no)
            {
                // Already checked, but OK to check again
                return tecNO_DST;
            }
            if (amt < psb.fees().accountReserve(0))
            {
                JLOG(j.trace()) << "Insufficient payment to create account.";
                return tecNO_DST_INSUF_XRP;
            }

            // Create the account.
            std::uint32_t const seqno{
                psb.rules().enabled(featureDeletableAccounts) ? psb.seq() : 1};

            sleDst = std::make_shared<SLE>(dstK);
            sleDst->setAccountID(sfAccount, dst);
            sleDst->setFieldU32(sfSequence, seqno);

            psb.insert(sleDst);
        }

        (*sleSrc)[sfBalance] = (*sleSrc)[sfBalance] - amt;
        (*sleDst)[sfBalance] = (*sleDst)[sfBalance] + amt;
        psb.update(sleSrc);
        psb.update(sleDst);

        return tesSUCCESS;
    }

    auto const result = flow(
        psb,
        amt,
        src,
        dst,
        STPathSet{},
        /*default path*/ true,
        /*partial payment*/ false,
        /*owner pays transfer fee*/ true,
        /*offer crossing*/ false,
        /*limit quality*/ std::nullopt,
        /*sendmax*/ std::nullopt,
        j);

    if (auto const r = result.result();
        isTesSuccess(r) || isTecClaim(r) || isTerRetry(r))
        return r;
    return tecXCHAIN_PAYMENT_FAILED;
}

/**  Action to take when the transfer from the door account to the dst fails

     @note This is useful to prevent a failed "create account" transaction from
           blocking subsequent "create account" transactions.
*/
enum class OnTransferFail {
    /** Remove the claim even if the transfer fails */
    removeClaim,
    /**  Keep the claim if the transfer fails */
    keepClaim
};

/** Transfer funds from the door account to the dst and distribute rewards

    @param psb The payment sandbox.
    @param bridgeSpc Bridge
    @param dst The destination for funds.
    @param dstTag Integer destination tag. Used to check if funds should be
           transferred to an account with a `RequireDstTag` flag set.
    @param claimOwner Owner of the claim ledger object.
    @param sendingAmount Amount that was committed on the source chain.
    @param rewardPoolSrc Source of the funds for the reward pool (claim owner).
    @param rewardPool Amount to split among the rewardAccounts.
    @param rewardAccounts Account to receive the reward pool.
    @param srcChain Chain where the commit event occurred.
    @param sleClaimID sle for the claim id (may be NULL or XChainClaimID or
           XChainCreateAccountClaimID). Don't read fields that aren't in common
           with those two types and always check for NULL. Remove on success (if
           not null). Remove on fail if the onTransferFail flag is removeClaim.
    @param onTransferFail Flag to determine if the claim is removed on transfer
           failure. This is used for create account transactions where claims
           are removed so they don't block future txns.
    @param j Log

    @return tesSUCCESS if payment succeeds, otherwise the error code for the
            failure reason. Note that failure to distribute rewards is still
            considered success.
 */
TER
finalizeClaimHelper(
    PaymentSandbox& psb,
    STXChainBridge const& bridgeSpec,
    AccountID const& dst,
    std::optional<std::uint32_t> const& dstTag,
    AccountID const& claimOwner,
    STAmount const& sendingAmount,
    AccountID const& rewardPoolSrc,
    STAmount const& rewardPool,
    std::vector<AccountID> const& rewardAccounts,
    STXChainBridge::ChainType const srcChain,
    std::shared_ptr<SLE> const& sleClaimID,
    OnTransferFail onTransferFail,
    beast::Journal j)
{
    STXChainBridge::ChainType const dstChain =
        STXChainBridge::otherChain(srcChain);
    STAmount const thisChainAmount = [&] {
        STAmount r = sendingAmount;
        r.setIssue(bridgeSpec.issue(dstChain));
        return r;
    }();
    auto const& thisDoor = bridgeSpec.door(dstChain);

    auto const thTer = transferHelper(
        psb,
        thisDoor,
        dst,
        dstTag,
        claimOwner,
        thisChainAmount,
        TransferHelperCanCreateDst::yes,
        j);

    if (!isTesSuccess(thTer) && onTransferFail == OnTransferFail::keepClaim)
    {
        return thTer;
    }

    if (sleClaimID)
    {
        auto const cidOwner = (*sleClaimID)[sfAccount];
        {
            // Remove the claim id
            auto const sleOwner = psb.peek(keylet::account(cidOwner));
            auto const page = (*sleClaimID)[sfOwnerNode];
            if (!psb.dirRemove(
                    keylet::ownerDir(cidOwner), page, sleClaimID->key(), true))
            {
                JLOG(j.fatal())
                    << "Unable to delete xchain seq number from owner.";
                return tefBAD_LEDGER;
            }

            // Remove the claim id from the ledger
            psb.erase(sleClaimID);

            adjustOwnerCount(psb, sleOwner, -1, j);
        }
    }

    if (!rewardAccounts.empty())
    {
        // distribute the reward pool
        // if the transfer failed, distribute the pool for "OnTransferFail"
        // cases (the attesters did their job)
        STAmount const share = [&] {
            STAmount const den{rewardAccounts.size()};
            return divide(rewardPool, den, rewardPool.issue());
        }();
        STAmount distributed = rewardPool.zeroed();
        for (auto const& rewardAccount : rewardAccounts)
        {
            auto const thTer = transferHelper(
                psb,
                rewardPoolSrc,
                rewardAccount,
                /*dstTag*/ std::nullopt,
                // claim owner is not relevant to distributing rewards
                /*claimOwner*/ std::nullopt,
                share,
                TransferHelperCanCreateDst::no,
                j);

            if (thTer == tecINSUFFICIENT_FUNDS || thTer == tecINTERNAL)
                return thTer;

            if (isTesSuccess(thTer))
                distributed += share;

            // let txn succeed if error distributing rewards (other than
            // inability to pay)
        }

        if (distributed > rewardPool)
            return tecINTERNAL;
    }

    return thTer;
}

/** Get signers list corresponding to the account that owns the bridge

    @param view View to read the signer's list from.
    @param sleBridge Sle of the bridge.
    @param j Log

    @return map of the signer's list (AccountIDs and weights), the quorum, and
            error code

    @note If the account includes a regular key or master key, it is included
   with the signer's list with a maximum weight. If the account does not include
   a signer's list, the threshold is set to 1 (i.e. either the master key or
   regular key can sign)
*/
std::tuple<std::unordered_map<AccountID, std::uint32_t>, std::uint32_t, TER>
getSignersListAndQuorum(
    ReadView const& view,
    SLE const& sleBridge,
    beast::Journal j)
{
    std::unordered_map<AccountID, std::uint32_t> r;
    std::uint32_t q = std::numeric_limits<std::uint32_t>::max();

    AccountID const thisDoor = sleBridge[sfAccount];
    auto const sleDoor = [&] { return view.read(keylet::account(thisDoor)); }();

    if (!sleDoor)
    {
        return {r, q, tecINTERNAL};
    }

    auto const masterKey = [&]() -> std::optional<AccountID> {
        if (sleDoor->isFlag(lsfDisableMaster))
            return std::nullopt;
        return thisDoor;
    }();

    std::optional<AccountID> regularKey = (*sleDoor)[~sfRegularKey];

    auto const sleS = view.read(keylet::signers(sleBridge[sfAccount]));
    if (!sleS)
    {
        if (masterKey || regularKey)
        {
            q = 1;
            if (masterKey)
                r[*masterKey] = std::numeric_limits<std::uint16_t>::max();
            if (regularKey)
                r[*regularKey] = std::numeric_limits<std::uint16_t>::max();

            return {std::move(r), q, tesSUCCESS};
        }
        return {r, q, tecXCHAIN_NO_SIGNERS_LIST};
    }
    q = (*sleS)[sfSignerQuorum];

    auto const accountSigners = SignerEntries::deserialize(*sleS, j, "ledger");

    if (!accountSigners)
    {
        return {r, q, tecINTERNAL};
    }

    for (auto const& as : *accountSigners)
    {
        r[as.account] = as.weight;
    }

    // add the master and regular keys. If they are already part of the signer's
    // list, overwrite their weights.
    if (masterKey)
        r[*masterKey] = std::numeric_limits<std::uint16_t>::max();
    if (regularKey)
        r[*regularKey] = std::numeric_limits<std::uint16_t>::max();

    return {std::move(r), q, tesSUCCESS};
};

template <class R, class F>
R
readOrpeekBridge(F&& getter, STXChainBridge const& bridgeSpec)
{
    auto tryGet = [&](STXChainBridge::ChainType ct) -> R {
        if (auto r = getter(bridgeSpec, ct))
        {
            if ((*r)[sfXChainBridge] == bridgeSpec)
                return r;
        }
        return nullptr;
    };
    if (auto r = tryGet(STXChainBridge::ChainType::locking))
        return r;
    return tryGet(STXChainBridge::ChainType::issuing);
}

std::shared_ptr<SLE>
peekBridge(ApplyView& v, STXChainBridge const& bridgeSpec)
{
    return readOrpeekBridge<std::shared_ptr<SLE>>(
        [&v](STXChainBridge const& b, STXChainBridge::ChainType ct)
            -> std::shared_ptr<SLE> {
            return v.peek(keylet::bridge(b.door(ct)));
        },
        bridgeSpec);
}

std::shared_ptr<SLE const>
readBridge(ReadView const& v, STXChainBridge const& bridgeSpec)
{
    return readOrpeekBridge<std::shared_ptr<SLE const>>(
        [&v](STXChainBridge const& b, STXChainBridge::ChainType ct)
            -> std::shared_ptr<SLE const> {
            return v.read(keylet::bridge(b.door(ct)));
        },
        bridgeSpec);
}

// Precondition: all the claims in the range are consistent. They must sign for
// the same event (amount, sending account, claim id, etc).
template <class TIter>
TER
applyClaimAttestations(
    ApplyView& view,
    RawView& rawView,
    TIter attBegin,
    TIter attEnd,
    STXChainBridge const& bridgeSpec,
    STXChainBridge::ChainType const srcChain,
    std::unordered_map<AccountID, std::uint32_t> const& signersList,
    std::uint32_t quorum,
    beast::Journal j)
{
    if (attBegin == attEnd)
        return tesSUCCESS;

    PaymentSandbox psb(&view);

    auto const sleClaimID =
        psb.peek(keylet::xChainClaimID(bridgeSpec, attBegin->claimID));
    if (!sleClaimID)
        return tecXCHAIN_NO_CLAIM_ID;
    AccountID const cidOwner = (*sleClaimID)[sfAccount];

    // Add claims that are part of the signer's list to the "claims" vector
    std::vector<AttestationBatch::AttestationClaim> atts;
    atts.reserve(std::distance(attBegin, attEnd));
    for (auto att = attBegin; att != attEnd; ++att)
    {
        if (!signersList.count(calcAccountID(att->publicKey)))
            continue;
        atts.push_back(*att);
    }

    if (atts.empty())
    {
        return tecXCHAIN_PROOF_UNKNOWN_KEY;
    }

    AccountID const otherChainSource = (*sleClaimID)[sfOtherChainSource];
    if (attBegin->sendingAccount != otherChainSource)
    {
        return tecXCHAIN_SENDING_ACCOUNT_MISMATCH;
    }

    {
        STXChainBridge::ChainType const dstChain =
            STXChainBridge::otherChain(srcChain);

        STXChainBridge::ChainType const attDstChain =
            STXChainBridge::dstChain(attBegin->wasLockingChainSend);

        if (attDstChain != dstChain)
        {
            return tecXCHAIN_WRONG_CHAIN;
        }
    }

    XChainClaimAttestations curAtts{
        sleClaimID->getFieldArray(sfXChainClaimAttestations)};

    auto const rewardAccounts = curAtts.onNewAttestations(
        &atts[0], &atts[0] + atts.size(), quorum, signersList);

    // update the claim id
    sleClaimID->setFieldArray(sfXChainClaimAttestations, curAtts.toSTArray());
    psb.update(sleClaimID);

    if (rewardAccounts && attBegin->dst)
    {
        auto const& rewardPoolSrc = (*sleClaimID)[sfAccount];
        auto const r = finalizeClaimHelper(
            psb,
            bridgeSpec,
            *attBegin->dst,
            /*dstTag*/ std::nullopt,
            cidOwner,
            attBegin->sendingAmount,
            rewardPoolSrc,
            (*sleClaimID)[sfSignatureReward],
            *rewardAccounts,
            srcChain,
            sleClaimID,
            OnTransferFail::keepClaim,
            j);
        if (!isTesSuccess(r))
            return r;
    }

    psb.apply(rawView);

    return tesSUCCESS;
}

template <class TIter>
TER
applyCreateAccountAttestations(
    ApplyView& view,
    RawView& rawView,
    TIter attBegin,
    TIter attEnd,
    AccountID const& doorAccount,
    Keylet const& doorK,
    STXChainBridge const& bridgeSpec,
    Keylet const& bridgeK,
    STXChainBridge::ChainType const srcChain,
    std::unordered_map<AccountID, std::uint32_t> const& signersList,
    std::uint32_t quorum,
    beast::Journal j)
{
    if (attBegin == attEnd)
        return tesSUCCESS;

    PaymentSandbox psb(&view);

    auto const sleDoor = psb.peek(doorK);
    if (!sleDoor)
        return tecINTERNAL;

    auto const sleBridge = psb.peek(bridgeK);
    if (!sleBridge)
        return tecINTERNAL;

    std::int64_t const claimCount = (*sleBridge)[sfXChainAccountClaimCount];

    if (attBegin->createCount <= claimCount)
    {
        return tecXCHAIN_ACCOUNT_CREATE_PAST;
    }
    if (attBegin->createCount >= claimCount + 128)
    {
        // Limit the number of claims on the account
        return tecXCHAIN_ACCOUNT_CREATE_TOO_MANY;
    }

    {
        STXChainBridge::ChainType const dstChain =
            STXChainBridge::otherChain(srcChain);

        STXChainBridge::ChainType const attDstChain =
            STXChainBridge::dstChain(attBegin->wasLockingChainSend);

        if (attDstChain != dstChain)
        {
            return tecXCHAIN_WRONG_CHAIN;
        }
    }

    auto const claimKeylet =
        keylet::xChainCreateAccountClaimID(bridgeSpec, attBegin->createCount);

    // sleClaimID may be null. If it's null it isn't created until the end of
    // this function (if needed)
    auto const sleClaimID = psb.peek(claimKeylet);
    bool createCID = false;
    if (!sleClaimID)
    {
        createCID = true;

        // Check reserve
        auto const balance = (*sleDoor)[sfBalance];
        auto const reserve =
            psb.fees().accountReserve((*sleDoor)[sfOwnerCount] + 1);

        if (balance < reserve)
            return tecINSUFFICIENT_RESERVE;
    }

    std::vector<AttestationBatch::AttestationCreateAccount> atts;
    atts.reserve(std::distance(attBegin, attEnd));
    for (auto att = attBegin; att != attEnd; ++att)
    {
        if (!signersList.count(calcAccountID(att->publicKey)))
            continue;
        atts.push_back(*att);
    }
    if (atts.empty())
    {
        return tecXCHAIN_PROOF_UNKNOWN_KEY;
    }

    XChainCreateAccountAttestations curAtts = [&] {
        if (sleClaimID)
            return XChainCreateAccountAttestations{
                sleClaimID->getFieldArray(sfXChainCreateAccountAttestations)};
        return XChainCreateAccountAttestations{};
    }();

    auto const rewardAccounts = curAtts.onNewAttestations(
        &atts[0], &atts[0] + atts.size(), quorum, signersList);

    if (!createCID)
    {
        // Modify the object before it's potentially deleted, so the meta data
        // will include the new attestations
        if (!sleClaimID)
            return tecINTERNAL;
        sleClaimID->setFieldArray(
            sfXChainCreateAccountAttestations, curAtts.toSTArray());
        psb.update(sleClaimID);
    }

    // Account create transactions must happen in order
    if (rewardAccounts && claimCount + 1 == attBegin->createCount)
    {
        auto const r = finalizeClaimHelper(
            psb,
            bridgeSpec,
            attBegin->toCreate,
            /*dstTag*/ std::nullopt,
            doorAccount,
            attBegin->sendingAmount,
            /*rewardPoolSrc*/ doorAccount,
            attBegin->rewardAmount,
            *rewardAccounts,
            srcChain,
            sleClaimID,
            OnTransferFail::removeClaim,
            j);
        if (!isTesSuccess(r))
        {
            if (r == tecINTERNAL || r == tecINSUFFICIENT_FUNDS ||
                isTefFailure(r))
                return r;
        }
        // Move past this claim id even if it fails, so it doesn't block
        // subsequent claim ids
        (*sleBridge)[sfXChainAccountClaimCount] = attBegin->createCount;
        psb.update(sleBridge);
    }
    else if (createCID)
    {
        if (sleClaimID)
            return tecINTERNAL;

        auto const createdSleClaimID = std::make_shared<SLE>(claimKeylet);
        (*createdSleClaimID)[sfAccount] = doorAccount;
        (*createdSleClaimID)[sfXChainBridge] = bridgeSpec;
        (*createdSleClaimID)[sfXChainAccountCreateCount] =
            attBegin->createCount;
        createdSleClaimID->setFieldArray(
            sfXChainCreateAccountAttestations, curAtts.toSTArray());

        // Add to owner directory of the door account
        auto const page = psb.dirInsert(
            keylet::ownerDir(doorAccount),
            claimKeylet,
            describeOwnerDir(doorAccount));
        if (!page)
            return tecDIR_FULL;
        (*createdSleClaimID)[sfOwnerNode] = *page;

        // Reserve was already checked
        adjustOwnerCount(psb, sleDoor, 1, j);
        psb.insert(createdSleClaimID);
        psb.update(sleDoor);
    }

    psb.apply(rawView);

    return tesSUCCESS;
}

template <class TAttestation>
std::optional<TAttestation>
toClaim(STTx const& tx)
{
    static_assert(
        std::is_same_v<TAttestation, AttestationBatch::AttestationClaim> ||
        std::is_same_v<
            TAttestation,
            AttestationBatch::AttestationCreateAccount>);

    try
    {
        STObject o{tx};
        o.setAccountID(sfAccount, o[sfOtherChainSource]);
        return TAttestation(o);
    }
    catch (...)
    {
    }
    return std::nullopt;
}

template <class TAttestation>
NotTEC
attestationPreflight(PreflightContext const& ctx)
{
    if (!ctx.rules.enabled(featureXChainBridge))
        return temDISABLED;

    if (auto const ret = preflight1(ctx); !isTesSuccess(ret))
        return ret;

    if (ctx.tx.getFlags() & tfUniversalMask)
        return temINVALID_FLAG;

    auto const att = toClaim<TAttestation>(ctx.tx);
    if (!att)
        return temMALFORMED;

    STXChainBridge const bridgeSpec = ctx.tx[sfXChainBridge];
    if (!att->verify(bridgeSpec))
        return temBAD_XCHAIN_PROOF;
    if (!att->validAmounts())
        return temBAD_XCHAIN_PROOF;

    if (att->sendingAmount.signum() <= 0)
        return temBAD_XCHAIN_PROOF;
    auto const expectedIssue =
        bridgeSpec.issue(STXChainBridge::srcChain(att->wasLockingChainSend));
    if (att->sendingAmount.issue() != expectedIssue)
        return temBAD_XCHAIN_PROOF;

    return preflight2(ctx);
}

template <class TAttestation>
TER
attestationDoApply(ApplyContext& ctx)
{
    auto const att = toClaim<TAttestation>(ctx.tx);
    if (!att)
        // Should already be checked in preflight
        return tecINTERNAL;

    STXChainBridge const bridgeSpec = ctx.tx[sfXChainBridge];

    // Note: sle's lifetimes should not overlap calls to applyCreateAccount
    // and applyClaims because those functions create a sandbox `sleBridge` is
    // reset before those calls and should not be used after those calls are
    // made. (it is not `const` because it is reset)
    auto sleBridge = readBridge(ctx.view(), bridgeSpec);
    if (!sleBridge)
    {
        return tecNO_ENTRY;
    }
    Keylet const bridgeK{ltBRIDGE, sleBridge->key()};
    AccountID const thisDoor = (*sleBridge)[sfAccount];

    STXChainBridge::ChainType dstChain = STXChainBridge::ChainType::locking;
    {
        if (thisDoor == bridgeSpec.lockingChainDoor())
            dstChain = STXChainBridge::ChainType::locking;
        else if (thisDoor == bridgeSpec.issuingChainDoor())
            dstChain = STXChainBridge::ChainType::issuing;
        else
            return tecINTERNAL;
    }
    STXChainBridge::ChainType const srcChain =
        STXChainBridge::otherChain(dstChain);

    // signersList is a map from account id to weights
    auto const [signersList, quorum, slTer] =
        getSignersListAndQuorum(ctx.view(), *sleBridge, ctx.journal);
    // It is difficult to reduce the scope of sleBridge. However, its scope
    // should be considered to end here. It's important that sles from one view
    // are not used after a child view is created from the view it is taken from
    // (as applyCreateAccount and applyClaims do).
    sleBridge.reset();

    if (!isTesSuccess(slTer))
        return slTer;

    static_assert(
        std::is_same_v<TAttestation, AttestationBatch::AttestationClaim> ||
        std::is_same_v<
            TAttestation,
            AttestationBatch::AttestationCreateAccount>);

    if constexpr (std::is_same_v<
                      TAttestation,
                      AttestationBatch::AttestationClaim>)
    {
        return applyClaimAttestations(
            ctx.view(),
            ctx.rawView(),
            &*att,
            &*att + 1,
            bridgeSpec,
            srcChain,
            signersList,
            quorum,
            ctx.journal);
    }
    else if constexpr (std::is_same_v<
                           TAttestation,
                           AttestationBatch::AttestationCreateAccount>)
    {
        return applyCreateAccountAttestations(
            ctx.view(),
            ctx.rawView(),
            &*att,
            &*att + 1,
            thisDoor,
            keylet::account(thisDoor),
            bridgeSpec,
            bridgeK,
            srcChain,
            signersList,
            quorum,
            ctx.journal);
    }
}

}  // namespace
//------------------------------------------------------------------------------

NotTEC
XChainCreateBridge::preflight(PreflightContext const& ctx)
{
    if (!ctx.rules.enabled(featureXChainBridge))
        return temDISABLED;

    if (auto const ret = preflight1(ctx); !isTesSuccess(ret))
        return ret;

    if (ctx.tx.getFlags() & tfUniversalMask)
        return temINVALID_FLAG;

    auto const account = ctx.tx[sfAccount];
    auto const reward = ctx.tx[sfSignatureReward];
    auto const minAccountCreate = ctx.tx[~sfMinAccountCreateAmount];
    auto const bridgeSpec = ctx.tx[sfXChainBridge];
    // Doors must be distinct to help prevent transaction replay attacks
    if (bridgeSpec.lockingChainDoor() == bridgeSpec.issuingChainDoor())
    {
        return temEQUAL_DOOR_ACCOUNTS;
    }

    if (bridgeSpec.lockingChainDoor() != account &&
        bridgeSpec.issuingChainDoor() != account)
    {
        return temSIDECHAIN_NONDOOR_OWNER;
    }

    if (isXRP(bridgeSpec.lockingChainIssue()) !=
        isXRP(bridgeSpec.issuingChainIssue()))
    {
        // Because ious and xrp have different numeric ranges, both the src and
        // dst issues must be both XRP or both IOU.
        return temSIDECHAIN_BAD_ISSUES;
    }

    if (!isXRP(reward) || reward.signum() < 0)
    {
        return temXCHAIN_BRIDGE_BAD_REWARD_AMOUNT;
    }

    if (minAccountCreate &&
        ((!isXRP(*minAccountCreate) || minAccountCreate->signum() <= 0) ||
         !isXRP(bridgeSpec.lockingChainIssue()) ||
         !isXRP(bridgeSpec.issuingChainIssue())))
    {
        return temXCHAIN_BRIDGE_BAD_MIN_ACCOUNT_CREATE_AMOUNT;
    }

    if (isXRP(bridgeSpec.issuingChainIssue()))
    {
        // Issuing account must be the root account for XRP (which presumably
        // owns all the XRP). This is done so the issuing account can't "run
        // out" of wrapped tokens.
        static auto const rootAccount = calcAccountID(
            generateKeyPair(
                KeyType::secp256k1, generateSeed("masterpassphrase"))
                .first);
        if (bridgeSpec.issuingChainDoor() != rootAccount)
        {
            return temSIDECHAIN_BAD_ISSUES;
        }
    }
    else
    {
        // Issuing account must be the issuer for non-XRP. This is done so the
        // issuing account can't "run out" of wrapped tokens.
        if (bridgeSpec.issuingChainDoor() !=
            bridgeSpec.issuingChainIssue().account)
        {
            return temSIDECHAIN_BAD_ISSUES;
        }
    }

    return preflight2(ctx);
}

TER
XChainCreateBridge::preclaim(PreclaimContext const& ctx)
{
    auto const account = ctx.tx[sfAccount];
    auto const bridgeSpec = ctx.tx[sfXChainBridge];

    // The bridge can't already exist on this ledger, and the bridge for the
    // locking chain and issuing chain can't live on the same ledger.
    if (ctx.view.read(keylet::bridge(
            bridgeSpec.door(STXChainBridge::ChainType::locking))) ||
        ctx.view.read(keylet::bridge(
            bridgeSpec.door(STXChainBridge::ChainType::issuing))))
    {
        return tecDUPLICATE;
    }

    STXChainBridge::ChainType const chainType =
        STXChainBridge::srcChain(account == bridgeSpec.lockingChainDoor());

    if (!isXRP(bridgeSpec.issue(chainType)) &&
        !ctx.view.read(keylet::account(bridgeSpec.issue(chainType).account)))
    {
        return tecNO_ISSUER;
    }

    {
        // Check reserve
        auto const sle = ctx.view.read(keylet::account(account));
        if (!sle)
            return terNO_ACCOUNT;

        auto const balance = (*sle)[sfBalance];
        auto const reserve =
            ctx.view.fees().accountReserve((*sle)[sfOwnerCount] + 1);

        if (balance < reserve)
            return tecINSUFFICIENT_RESERVE;
    }

    return tesSUCCESS;
}

TER
XChainCreateBridge::doApply()
{
    auto const account = ctx_.tx[sfAccount];
    auto const bridgeSpec = ctx_.tx[sfXChainBridge];
    auto const reward = ctx_.tx[sfSignatureReward];
    auto const minAccountCreate = ctx_.tx[~sfMinAccountCreateAmount];

    auto const sleAcct = ctx_.view().peek(keylet::account(account));
    if (!sleAcct)
        return tecINTERNAL;

    STXChainBridge::ChainType const chainType =
        STXChainBridge::srcChain(account == bridgeSpec.lockingChainDoor());

    Keylet const bridgeKeylet = keylet::bridge(bridgeSpec.door(chainType));
    auto const sleBridge = std::make_shared<SLE>(bridgeKeylet);

    (*sleBridge)[sfAccount] = account;
    (*sleBridge)[sfSignatureReward] = reward;
    if (minAccountCreate)
        (*sleBridge)[sfMinAccountCreateAmount] = *minAccountCreate;
    (*sleBridge)[sfXChainBridge] = bridgeSpec;
    (*sleBridge)[sfXChainClaimID] = 0;
    (*sleBridge)[sfXChainAccountCreateCount] = 0;
    (*sleBridge)[sfXChainAccountClaimCount] = 0;

    // Add to owner directory
    {
        auto const page = ctx_.view().dirInsert(
            keylet::ownerDir(account), bridgeKeylet, describeOwnerDir(account));
        if (!page)
            return tecDIR_FULL;
        (*sleBridge)[sfOwnerNode] = *page;
    }

    adjustOwnerCount(ctx_.view(), sleAcct, 1, ctx_.journal);

    ctx_.view().insert(sleBridge);
    ctx_.view().update(sleAcct);

    return tesSUCCESS;
}

//------------------------------------------------------------------------------

NotTEC
BridgeModify::preflight(PreflightContext const& ctx)
{
    if (!ctx.rules.enabled(featureXChainBridge))
        return temDISABLED;

    if (auto const ret = preflight1(ctx); !isTesSuccess(ret))
        return ret;

    if (ctx.tx.getFlags() & tfUniversalMask)
        return temINVALID_FLAG;

    auto const account = ctx.tx[sfAccount];
    auto const reward = ctx.tx[~sfSignatureReward];
    auto const minAccountCreate = ctx.tx[~sfMinAccountCreateAmount];
    auto const bridgeSpec = ctx.tx[sfXChainBridge];

    if (!reward && !minAccountCreate)
    {
        // Must change something
        return temMALFORMED;
    }

    if (bridgeSpec.lockingChainDoor() != account &&
        bridgeSpec.issuingChainDoor() != account)
    {
        return temSIDECHAIN_NONDOOR_OWNER;
    }

    if (reward && (!isXRP(*reward) || reward->signum() < 0))
    {
        return temXCHAIN_BRIDGE_BAD_REWARD_AMOUNT;
    }

    if (minAccountCreate &&
        ((!isXRP(*minAccountCreate) || minAccountCreate->signum() <= 0) ||
         !isXRP(bridgeSpec.lockingChainIssue()) ||
         !isXRP(bridgeSpec.issuingChainIssue())))
    {
        return temXCHAIN_BRIDGE_BAD_MIN_ACCOUNT_CREATE_AMOUNT;
    }

    return preflight2(ctx);
}

TER
BridgeModify::preclaim(PreclaimContext const& ctx)
{
    auto const account = ctx.tx[sfAccount];
    auto const bridgeSpec = ctx.tx[sfXChainBridge];

    STXChainBridge::ChainType const chainType =
        STXChainBridge::srcChain(account == bridgeSpec.lockingChainDoor());

    if (!ctx.view.read(keylet::bridge(bridgeSpec.door(chainType))))
    {
        return tecNO_ENTRY;
    }

    return tesSUCCESS;
}

TER
BridgeModify::doApply()
{
    auto const account = ctx_.tx[sfAccount];
    auto const bridgeSpec = ctx_.tx[sfXChainBridge];
    auto const reward = ctx_.tx[~sfSignatureReward];
    auto const minAccountCreate = ctx_.tx[~sfMinAccountCreateAmount];

    auto const sleAcct = ctx_.view().peek(keylet::account(account));
    if (!sleAcct)
        return tecINTERNAL;

    STXChainBridge::ChainType const chainType =
        STXChainBridge::srcChain(account == bridgeSpec.lockingChainDoor());

    auto const sleBridge =
        ctx_.view().peek(keylet::bridge(bridgeSpec.door(chainType)));
    if (!sleBridge)
        return tecINTERNAL;

    if (reward)
        (*sleBridge)[sfSignatureReward] = *reward;
    if (minAccountCreate)
    {
        // TODO: How do I modify minAccountCreate to clear it? With a flag?
        (*sleBridge)[sfMinAccountCreateAmount] = *minAccountCreate;
    }
    ctx_.view().update(sleBridge);

    return tesSUCCESS;
}

//------------------------------------------------------------------------------

NotTEC
XChainClaim::preflight(PreflightContext const& ctx)
{
    if (!ctx.rules.enabled(featureXChainBridge))
        return temDISABLED;

    if (auto const ret = preflight1(ctx); !isTesSuccess(ret))
        return ret;

    if (ctx.tx.getFlags() & tfUniversalMask)
        return temINVALID_FLAG;

    STXChainBridge const bridgeSpec = ctx.tx[sfXChainBridge];
    auto const amount = ctx.tx[sfAmount];

    if (amount.signum() <= 0 ||
        (amount.issue() != bridgeSpec.lockingChainIssue() &&
         amount.issue() != bridgeSpec.issuingChainIssue()))
    {
        return temBAD_AMOUNT;
    }

    return preflight2(ctx);
}

TER
XChainClaim::preclaim(PreclaimContext const& ctx)
{
    AccountID const account = ctx.tx[sfAccount];
    STXChainBridge const bridgeSpec = ctx.tx[sfXChainBridge];
    STAmount const& thisChainAmount = ctx.tx[sfAmount];
    auto const claimID = ctx.tx[sfXChainClaimID];

    auto const sleBridge = readBridge(ctx.view, bridgeSpec);
    if (!sleBridge)
    {
        return tecNO_ENTRY;
    }

    if (!ctx.view.read(keylet::account(ctx.tx[sfDestination])))
    {
        return tecNO_DST;
    }

    auto const thisDoor = (*sleBridge)[sfAccount];
    bool isLockingChain = false;
    {
        if (thisDoor == bridgeSpec.lockingChainDoor())
            isLockingChain = true;
        else if (thisDoor == bridgeSpec.issuingChainDoor())
            isLockingChain = false;
        else
            return tecINTERNAL;
    }

    {
        // Check that the amount specified matches the expected issue

        if (isLockingChain)
        {
            if (bridgeSpec.lockingChainIssue() != thisChainAmount.issue())
                return tecBAD_XCHAIN_TRANSFER_ISSUE;
        }
        else
        {
            if (bridgeSpec.issuingChainIssue() != thisChainAmount.issue())
                return tecBAD_XCHAIN_TRANSFER_ISSUE;
        }
    }

    if (isXRP(bridgeSpec.lockingChainIssue()) !=
        isXRP(bridgeSpec.issuingChainIssue()))
    {
        // Should have been caught when creating the bridge
        // Detect here so `otherChainAmount` doesn't switch from IOU -> XRP
        // and the numeric issues that need to be addressed with that.
        return tecINTERNAL;
    }

    auto const otherChainAmount = [&]() -> STAmount {
        STAmount r(thisChainAmount);
        if (isLockingChain)
            r.setIssue(bridgeSpec.issuingChainIssue());
        else
            r.setIssue(bridgeSpec.lockingChainIssue());
        return r;
    }();

    auto const sleClaimID =
        ctx.view.read(keylet::xChainClaimID(bridgeSpec, claimID));
    {
        // Check that the sequence number is owned by the sender of this
        // transaction
        if (!sleClaimID)
        {
            return tecXCHAIN_NO_CLAIM_ID;
        }

        if ((*sleClaimID)[sfAccount] != account)
        {
            // Sequence number isn't owned by the sender of this transaction
            return tecXCHAIN_BAD_CLAIM_ID;
        }
    }

    // quorum is checked in `doApply`
    return tesSUCCESS;
}

TER
XChainClaim::doApply()
{
    PaymentSandbox psb(&ctx_.view());

    AccountID const account = ctx_.tx[sfAccount];
    auto const dst = ctx_.tx[sfDestination];
    STXChainBridge const bridgeSpec = ctx_.tx[sfXChainBridge];
    STAmount const& thisChainAmount = ctx_.tx[sfAmount];
    auto const claimID = ctx_.tx[sfXChainClaimID];

    auto const sleAcct = psb.peek(keylet::account(account));
    auto const sleBridge = peekBridge(psb, bridgeSpec);
    auto const sleClaimID =
        psb.peek(keylet::xChainClaimID(bridgeSpec, claimID));

    if (!(sleBridge && sleClaimID && sleAcct))
        return tecINTERNAL;

    AccountID const thisDoor = (*sleBridge)[sfAccount];

    STXChainBridge::ChainType dstChain = STXChainBridge::ChainType::locking;
    {
        if (thisDoor == bridgeSpec.lockingChainDoor())
            dstChain = STXChainBridge::ChainType::locking;
        else if (thisDoor == bridgeSpec.issuingChainDoor())
            dstChain = STXChainBridge::ChainType::issuing;
        else
            return tecINTERNAL;
    }
    STXChainBridge::ChainType const srcChain =
        STXChainBridge::otherChain(dstChain);

    auto const sendingAmount = [&]() -> STAmount {
        STAmount r(thisChainAmount);
        r.setIssue(bridgeSpec.issue(srcChain));
        return r;
    }();

    auto const [signersList, quorum, slTer] =
        getSignersListAndQuorum(ctx_.view(), *sleBridge, ctx_.journal);

    if (!isTesSuccess(slTer))
        return slTer;

    XChainClaimAttestations curAtts{
        sleClaimID->getFieldArray(sfXChainClaimAttestations)};

    auto const claimR = curAtts.onClaim(
        sendingAmount,
        /*wasLockingChainSend*/ srcChain == STXChainBridge::ChainType::locking,
        quorum,
        signersList);
    if (!claimR.has_value())
        return claimR.error();

    auto const& rewardAccounts = claimR.value();
    auto const& rewardPoolSrc = (*sleClaimID)[sfAccount];

    std::optional<std::uint32_t> const dstTag = ctx_.tx[~sfDestinationTag];

    auto const r = finalizeClaimHelper(
        psb,
        bridgeSpec,
        dst,
        dstTag,
        /*claimOwner*/ account,
        sendingAmount,
        rewardPoolSrc,
        (*sleClaimID)[sfSignatureReward],
        rewardAccounts,
        srcChain,
        sleClaimID,
        OnTransferFail::keepClaim,
        ctx_.journal);
    if (!isTesSuccess(r))
        return r;

    psb.apply(ctx_.rawView());

    return tesSUCCESS;
}

//------------------------------------------------------------------------------

TxConsequences
XChainCommit::makeTxConsequences(PreflightContext const& ctx)
{
    auto const maxSpend = [&] {
        auto const amount = ctx.tx[sfAmount];
        if (amount.native() && amount.signum() > 0)
            return amount.xrp();
        return XRPAmount{beast::zero};
    }();

    return TxConsequences{ctx.tx, maxSpend};
}

NotTEC
XChainCommit::preflight(PreflightContext const& ctx)
{
    if (!ctx.rules.enabled(featureXChainBridge))
        return temDISABLED;

    if (auto const ret = preflight1(ctx); !isTesSuccess(ret))
        return ret;

    if (ctx.tx.getFlags() & tfUniversalMask)
        return temINVALID_FLAG;

    auto const amount = ctx.tx[sfAmount];

    if (amount.signum() <= 0 || !isLegalNet(amount))
        return temBAD_AMOUNT;

    return preflight2(ctx);
}

TER
XChainCommit::preclaim(PreclaimContext const& ctx)
{
    auto const bridgeSpec = ctx.tx[sfXChainBridge];
    auto const amount = ctx.tx[sfAmount];

    auto const sleBridge = readBridge(ctx.view, bridgeSpec);
    if (!sleBridge)
    {
        return tecNO_ENTRY;
    }

    AccountID const thisDoor = (*sleBridge)[sfAccount];
    AccountID const account = ctx.tx[sfAccount];

    if (thisDoor == account)
    {
        // Door account can't lock funds onto itself
        return tecXCHAIN_SELF_COMMIT;
    }

    bool isLockingChain = false;
    {
        if (thisDoor == bridgeSpec.lockingChainDoor())
            isLockingChain = true;
        else if (thisDoor == bridgeSpec.issuingChainDoor())
            isLockingChain = false;
        else
            return tecINTERNAL;
    }

    if (isLockingChain)
    {
        if (bridgeSpec.lockingChainIssue() != ctx.tx[sfAmount].issue())
            return tecBAD_XCHAIN_TRANSFER_ISSUE;
    }
    else
    {
        if (bridgeSpec.issuingChainIssue() != ctx.tx[sfAmount].issue())
            return tecBAD_XCHAIN_TRANSFER_ISSUE;
    }

    return tesSUCCESS;
}

TER
XChainCommit::doApply()
{
    PaymentSandbox psb(&ctx_.view());

    auto const account = ctx_.tx[sfAccount];
    auto const amount = ctx_.tx[sfAmount];
    auto const bridgeSpec = ctx_.tx[sfXChainBridge];

    if (!psb.read(keylet::account(account)))
        return tecINTERNAL;

    auto const sleBridge = readBridge(psb, bridgeSpec);
    if (!sleBridge)
        return tecINTERNAL;

    auto const dst = (*sleBridge)[sfAccount];

    auto const thTer = transferHelper(
        psb,
        account,
        dst,
        /*dstTag*/ std::nullopt,
        /*claimOwner*/ std::nullopt,
        amount,
        TransferHelperCanCreateDst::no,
        ctx_.journal);

    if (!isTesSuccess(thTer))
        return thTer;

    psb.apply(ctx_.rawView());

    return tesSUCCESS;
}

//------------------------------------------------------------------------------

NotTEC
XChainCreateClaimID::preflight(PreflightContext const& ctx)
{
    if (!ctx.rules.enabled(featureXChainBridge))
        return temDISABLED;

    if (auto const ret = preflight1(ctx); !isTesSuccess(ret))
        return ret;

    if (ctx.tx.getFlags() & tfUniversalMask)
        return temINVALID_FLAG;

    auto const reward = ctx.tx[sfSignatureReward];

    if (!isXRP(reward) || reward.signum() < 0 || !isLegalNet(reward))
        return temXCHAIN_BRIDGE_BAD_REWARD_AMOUNT;

    return preflight2(ctx);
}

TER
XChainCreateClaimID::preclaim(PreclaimContext const& ctx)
{
    auto const account = ctx.tx[sfAccount];
    auto const bridgeSpec = ctx.tx[sfXChainBridge];
    auto const sleBridge = readBridge(ctx.view, bridgeSpec);

    if (!sleBridge)
    {
        return tecNO_ENTRY;
    }

    // Check that the reward matches
    auto const reward = ctx.tx[sfSignatureReward];

    if (reward != (*sleBridge)[sfSignatureReward])
    {
        return tecXCHAIN_REWARD_MISMATCH;
    }

    {
        // Check reserve
        auto const sle = ctx.view.read(keylet::account(account));
        if (!sle)
            return terNO_ACCOUNT;

        auto const balance = (*sle)[sfBalance];
        auto const reserve =
            ctx.view.fees().accountReserve((*sle)[sfOwnerCount] + 1);

        if (balance < reserve)
            return tecINSUFFICIENT_RESERVE;
    }

    return tesSUCCESS;
}

TER
XChainCreateClaimID::doApply()
{
    auto const account = ctx_.tx[sfAccount];
    auto const bridgeSpec = ctx_.tx[sfXChainBridge];
    auto const reward = ctx_.tx[sfSignatureReward];
    auto const otherChainSrc = ctx_.tx[sfOtherChainSource];

    auto const sleAcct = ctx_.view().peek(keylet::account(account));
    if (!sleAcct)
        return tecINTERNAL;

    auto const sleBridge = peekBridge(ctx_.view(), bridgeSpec);
    if (!sleBridge)
        return tecINTERNAL;

    std::uint32_t const claimID = (*sleBridge)[sfXChainClaimID] + 1;
    if (claimID == 0)
        return tecINTERNAL;  // overflow

    (*sleBridge)[sfXChainClaimID] = claimID;

    Keylet const claimIDKeylet = keylet::xChainClaimID(bridgeSpec, claimID);
    if (ctx_.view().exists(claimIDKeylet))
        return tecINTERNAL;  // already checked out!?!

    auto const sleClaimID = std::make_shared<SLE>(claimIDKeylet);

    (*sleClaimID)[sfAccount] = account;
    (*sleClaimID)[sfXChainBridge] = bridgeSpec;
    (*sleClaimID)[sfXChainClaimID] = claimID;
    (*sleClaimID)[sfOtherChainSource] = otherChainSrc;
    (*sleClaimID)[sfSignatureReward] = reward;
    sleClaimID->setFieldArray(
        sfXChainClaimAttestations, STArray{sfXChainClaimAttestations});

    // Add to owner directory
    {
        auto const page = ctx_.view().dirInsert(
            keylet::ownerDir(account),
            claimIDKeylet,
            describeOwnerDir(account));
        if (!page)
            return tecDIR_FULL;
        (*sleClaimID)[sfOwnerNode] = *page;
    }

    adjustOwnerCount(ctx_.view(), sleAcct, 1, ctx_.journal);

    ctx_.view().insert(sleClaimID);
    ctx_.view().update(sleBridge);
    ctx_.view().update(sleAcct);

    return tesSUCCESS;
}

//------------------------------------------------------------------------------

NotTEC
XChainAddClaimAttestation::preflight(PreflightContext const& ctx)
{
    return attestationPreflight<AttestationBatch::AttestationClaim>(ctx);
}

TER
XChainAddClaimAttestation::preclaim(PreclaimContext const& ctx)
{
    return tesSUCCESS;
}

TER
XChainAddClaimAttestation::doApply()
{
    return attestationDoApply<AttestationBatch::AttestationClaim>(ctx_);
}

//------------------------------------------------------------------------------

NotTEC
XChainAddAccountCreateAttestation::preflight(PreflightContext const& ctx)
{
    return attestationPreflight<AttestationBatch::AttestationCreateAccount>(
        ctx);
}

TER
XChainAddAccountCreateAttestation::preclaim(PreclaimContext const& ctx)
{
    return tesSUCCESS;
}

TER
XChainAddAccountCreateAttestation::doApply()
{
    return attestationDoApply<AttestationBatch::AttestationCreateAccount>(ctx_);
}

//------------------------------------------------------------------------------

NotTEC
XChainCreateAccountCommit::preflight(PreflightContext const& ctx)
{
    if (!ctx.rules.enabled(featureXChainBridge))
        return temDISABLED;

    if (auto const ret = preflight1(ctx); !isTesSuccess(ret))
        return ret;

    if (ctx.tx.getFlags() & tfUniversalMask)
        return temINVALID_FLAG;

    auto const amount = ctx.tx[sfAmount];

    if (amount.signum() <= 0 || !amount.native())
        return temBAD_AMOUNT;

    auto const reward = ctx.tx[sfSignatureReward];
    if (reward.signum() < 0 || !reward.native())
        return temBAD_AMOUNT;

    if (reward.issue() != amount.issue())
        return temBAD_AMOUNT;

    return preflight2(ctx);
}

TER
XChainCreateAccountCommit::preclaim(PreclaimContext const& ctx)
{
    STXChainBridge const bridgeSpec = ctx.tx[sfXChainBridge];
    STAmount const amount = ctx.tx[sfAmount];
    STAmount const reward = ctx.tx[sfSignatureReward];

    auto const sleBridge = readBridge(ctx.view, bridgeSpec);
    if (!sleBridge)
    {
        return tecNO_ENTRY;
    }

    if (reward != (*sleBridge)[sfSignatureReward])
    {
        return tecXCHAIN_REWARD_MISMATCH;
    }

    std::optional<STAmount> const minCreateAmount =
        (*sleBridge)[~sfMinAccountCreateAmount];

    if (!minCreateAmount || amount < *minCreateAmount)
    {
        return tecXCHAIN_INSUFF_CREATE_AMOUNT;
    }

    if (minCreateAmount->issue() != amount.issue())
        return tecBAD_XCHAIN_TRANSFER_ISSUE;

    AccountID const thisDoor = (*sleBridge)[sfAccount];
    AccountID const account = ctx.tx[sfAccount];
    if (thisDoor == account)
    {
        // Door account can't lock funds onto itself
        return tecXCHAIN_SELF_COMMIT;
    }

    STXChainBridge::ChainType srcChain = STXChainBridge::ChainType::locking;
    {
        if (thisDoor == bridgeSpec.lockingChainDoor())
            srcChain = STXChainBridge::ChainType::locking;
        else if (thisDoor == bridgeSpec.issuingChainDoor())
            srcChain = STXChainBridge::ChainType::issuing;
        else
            return tecINTERNAL;
    }
    STXChainBridge::ChainType const dstChain =
        STXChainBridge::otherChain(srcChain);

    if (bridgeSpec.issue(srcChain) != ctx.tx[sfAmount].issue())
        return tecBAD_XCHAIN_TRANSFER_ISSUE;

    if (!isXRP(bridgeSpec.issue(dstChain)))
        return tecXCHAIN_CREATE_ACCOUNT_NONXRP_ISSUE;

    return tesSUCCESS;
}

TER
XChainCreateAccountCommit::doApply()
{
    PaymentSandbox psb(&ctx_.view());

    AccountID const account = ctx_.tx[sfAccount];
    STAmount const amount = ctx_.tx[sfAmount];
    STAmount const reward = ctx_.tx[sfSignatureReward];
    STXChainBridge const bridge = ctx_.tx[sfXChainBridge];

    auto const sle = psb.peek(keylet::account(account));
    if (!sle)
        return tecINTERNAL;

    auto const sleBridge = peekBridge(psb, bridge);
    if (!sleBridge)
        return tecINTERNAL;

    auto const dst = (*sleBridge)[sfAccount];

    STAmount const toTransfer = amount + reward;
    auto const thTer = transferHelper(
        psb,
        account,
        dst,
        /*dstTag*/ std::nullopt,
        /*claimOwner*/ std::nullopt,
        toTransfer,
        TransferHelperCanCreateDst::yes,
        ctx_.journal);

    if (!isTesSuccess(thTer))
        return thTer;

    (*sleBridge)[sfXChainAccountCreateCount] =
        (*sleBridge)[sfXChainAccountCreateCount] + 1;
    psb.update(sleBridge);

    psb.apply(ctx_.rawView());

    return tesSUCCESS;
}

}  // namespace ripple
