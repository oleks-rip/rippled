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

#pragma once

#include <xrpld/app/tx/detail/Transactor.h>

namespace ripple {
namespace credentials {

// These function will be used by the code that use DepositPreauth / Credentials
// (and any future preauthorization modes) as part of authorization (all the
// transfer funds transactions)

// Check if credential sfExpiration field has passed ledger's parentCloseTime
bool
checkExpired(
    std::shared_ptr<SLE const> const& sleCredential,
    NetClock::time_point const& closed);

// Return true if at least 1 expired credentials was found(and deleted)
bool
removeExpired(ApplyView& view, STTx const& tx, beast::Journal const j);

// Actually remove a credentials object from the ledger
TER
deleteSLE(
    ApplyView& view,
    std::shared_ptr<SLE> const& sleCredential,
    beast::Journal j);

// Amendment and parameters checks for sfCredentialIDs field
NotTEC
check(PreflightContext const& ctx);

// Accessing the ledger to check if provided credentials are valid
TER
valid(
    PreclaimContext const& ctx,
    AccountID const& src,
    AccountID const& dst,
    std::optional<std::reference_wrapper<std::shared_ptr<SLE const> const>>
        sleDstOpt = {});

}  // namespace credentials
}  // namespace ripple