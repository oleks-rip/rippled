//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2023 Ripple Labs Inc.

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
/*
#include <xrpld/app/misc/WasmVM.h>

namespace ripple {

class WasmEngineIImpl;

class WasmEngineI final : public WasmEngine
{
    std::unique_ptr<WasmEngineIImpl> impl;

public:
    WasmEngineI();
    virtual ~WasmEngineI();

    virtual Expected<bool, TER>
    run(vbytes const& wasmCode,
        std::string_view funcName,
        int32_t input) override;

    virtual Expected<bool, TER>
    run(vbytes const& wasmCode,
        std::string_view funcName,
        vbytes const& accountID) override;

    virtual Expected<bool, TER>
    run(vbytes const& wasmCode,
        std::string_view funcName,
        vbytes const& escrow_tx_json_data,
        vbytes const& escrow_lo_json_data) override;

    virtual Expected<std::pair<bool, std::string>, TER>
    runP4(
        vbytes const& wasmCode,
        std::string_view funcName,
        vbytes const& escrow_tx_json_data,
        vbytes const& escrow_lo_json_data) override;

    virtual Expected<std::pair<bool, std::string>, TER>
    justRunP4(
        std::string_view funcName,
        vbytes const& escrow_tx_json_data,
        vbytes const& escrow_lo_json_data,
        int m,
        int i) override;

    virtual Expected<bool, TER>
    run(vbytes const& wasmCode,
        std::string_view funcName,
        LedgerDataProvider* ledgerDataProvider) override;

    virtual Expected<int, TER>
    preRun(vbytes const& wasmCode, LedgerDataProvider* ledgerDataProvider)
        override;

    virtual Expected<bool, TER>
    justRun(
        std::string_view funcName,
        LedgerDataProvider* ledgerDataProvider,
        int m,
        int i) override;

    Expected<int, TER>
    justRun(std::string_view funcName, int m, int i) override;

    virtual int
    addModule(vbytes const& wasmCode, bool instantiate) override;
    virtual void
    clearModules() override;
    virtual int
    addInstance(int m) override;

    virtual int32_t
    runFunc(std::string_view const funcName, int32_t p, int m, int i) override;

    virtual int64_t
    runFunc64(std::string_view const funcName, int64_t p, int m, int i)
        override;

    virtual std::vector<uint64_t>
    runSha(std::string_view const data, int m, int i) override;

    virtual int32_t
    runEnc(
        std::string_view const funcName,
        std::string& sv_res,
        std::string_view const data,
        int m,
        int i) override;
};

}  // namespace ripple
*/
