//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2024 Ripple Labs Inc.

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

#include <xrpld/app/misc/CredentialsHelper.h>
#include <xrpld/ledger/View.h>

namespace ripple {
namespace credentials {

bool
checkExpired(
    std::shared_ptr<SLE const> const& sleCredential,
    NetClock::time_point const& closed)
{
    std::uint32_t const exp = (*sleCredential)[~sfExpiration].value_or(
        std::numeric_limits<std::uint32_t>::max());
    std::uint32_t const now = closed.time_since_epoch().count();
    return now > exp;
}

bool
removeExpired(ApplyView& view, STTx const& tx, beast::Journal const j)
{
    auto const closeTime = view.info().parentCloseTime;
    bool foundExpired = false;

    STVector256 const& arr(tx.getFieldV256(sfCredentialIDs));
    for (auto const& h : arr)
    {
        // Credentials already checked in preclaim. Look only for expired here.
        auto const k = keylet::credential(h);
        auto const sleCred = view.peek(k);

        if (checkExpired(sleCred, closeTime))
        {
            JLOG(j.trace())
                << "Credentials are expired. Cred: " << sleCred->getText();
            // delete expired credentials even if the transaction failed
            deleteSLE(view, sleCred, j);
            foundExpired = true;
        }
    }

    return foundExpired;
}

TER
deleteSLE(
    ApplyView& view,
    std::shared_ptr<SLE> const& sleCredential,
    beast::Journal j)
{
    if (!sleCredential)
        return tecNO_ENTRY;

    auto delSLE =
        [&view, &sleCredential, j](
            AccountID const& account, SField const& node, bool isOwner) -> TER {
        auto const sleAccount = view.peek(keylet::account(account));
        if (!sleAccount)
        {
            JLOG(j.fatal()) << "Internal error: can't retrieve Owner account.";
            return tecINTERNAL;
        }

        // Remove object from owner directory
        std::uint64_t const page = sleCredential->getFieldU64(node);
        if (!view.dirRemove(
                keylet::ownerDir(account), page, sleCredential->key(), false))
        {
            JLOG(j.fatal()) << "Unable to delete Credential from owner.";
            return tefBAD_LEDGER;
        }

        if (isOwner)
            adjustOwnerCount(view, sleAccount, -1, j);

        return tesSUCCESS;
    };

    auto const issuer = sleCredential->getAccountID(sfIssuer);
    auto const subject = sleCredential->getAccountID(sfSubject);
    bool const accepted = sleCredential->getFlags() & lsfAccepted;

    auto err = delSLE(issuer, sfIssuerNode, !accepted || (subject == issuer));
    if (!isTesSuccess(err))
        return err;

    if (subject != issuer)
    {
        err = delSLE(subject, sfSubjectNode, accepted);
        if (!isTesSuccess(err))
            return err;
    }

    // Remove object from ledger
    view.erase(sleCredential);

    return tesSUCCESS;
}

NotTEC
check(PreflightContext const& ctx)
{
    if (!ctx.tx.isFieldPresent(sfCredentialIDs))
        return tesSUCCESS;

    if (!ctx.rules.enabled(featureCredentials))
    {
        JLOG(ctx.j.trace()) << "Credentials rule is disabled.";
        return temDISABLED;
    }

    auto const& credentials = ctx.tx.getFieldV256(sfCredentialIDs);
    if (credentials.empty() || (credentials.size() > credentialsArrayMaxSize))
    {
        JLOG(ctx.j.trace())
            << "Malformed transaction: Credentials array size is invalid: "
            << credentials.size();
        return temMALFORMED;
    }

    return tesSUCCESS;
}

TER
valid(
    PreclaimContext const& ctx,
    AccountID const& src,
    AccountID const& dst,
    std::optional<std::reference_wrapper<std::shared_ptr<SLE const> const>>
        sleDstOpt)
{
    if (!ctx.tx.isFieldPresent(sfCredentialIDs))
        return tesSUCCESS;

    std::shared_ptr<SLE const> const& sleDst =
        sleDstOpt ? *sleDstOpt : ctx.view.read(keylet::account(dst));

    bool const reqAuth =
        sleDst && (sleDst->getFlags() & lsfDepositAuth) && (src != dst);

    // credentials must be checked even if reqAuth is false
    STArray authCreds;
    for (auto const& h : ctx.tx.getFieldV256(sfCredentialIDs))
    {
        auto const sleCred = ctx.view.read(keylet::credential(h));
        if (!sleCred)
        {
            JLOG(ctx.j.trace()) << "Credential doesn't exist. Cred: " << h;
            return tecBAD_CREDENTIALS;
        }

        if (sleCred->getAccountID(sfSubject) != src)
        {
            JLOG(ctx.j.trace())
                << "Credential doesn’t belong to the source account. Cred: "
                << h;
            return tecBAD_CREDENTIALS;
        }

        if (!(sleCred->getFlags() & lsfAccepted))
        {
            JLOG(ctx.j.trace()) << "Credential isn't accepted. Cred: " << h;
            return tecBAD_CREDENTIALS;
        }

        if (reqAuth)
        {
            auto credential = STObject::makeInnerObject(sfCredential);
            credential.setAccountID(sfIssuer, sleCred->getAccountID(sfIssuer));
            credential.setFieldVL(
                sfCredentialType, sleCred->getFieldVL(sfCredentialType));
            authCreds.push_back(std::move(credential));
        }
    }

    if (reqAuth && !ctx.view.exists(keylet::depositPreauth(dst, authCreds)))
    {
        JLOG(ctx.j.trace()) << "DepositPreauth doesn't exist";
        return tecNO_PERMISSION;
    }

    return tesSUCCESS;
}

}  // namespace credentials
}  // namespace ripple