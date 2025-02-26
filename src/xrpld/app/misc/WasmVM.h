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

#include <xrpl/basics/Expected.h>
#include <xrpl/beast/utility/Journal.h>
#include <xrpl/protocol/TER.h>

#include <string_view>

namespace ripple {

using vbytes = std::vector<std::uint8_t>;
struct vmem
{
    std::uint8_t* p = nullptr;
    std::size_t s = 0;
};


static const std::string_view V_MEM = "memory";
static const std::string_view V_STORE = "store";
static const std::string_view V_LOAD = "load";
static const std::string_view V_SIZE = "size";

static const std::string_view V_ALLOC = "allocate";
static const std::string_view V_DEALLOC = "deallocate";

enum class wasmEngines : int { Edge, Time, Er, I, Wamr, END };
void setWasmEngine(wasmEngines);

Expected<bool, TER>
runEscrowWasm(vbytes const& wasmCode, std::string_view funcName, int32_t input);

Expected<bool, TER>
runEscrowWasmWTime(
    vbytes const& wasmCode,
    std::string_view funcName,
    int32_t input);

Expected<bool, TER>
runEscrowWasm(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& accountID);

Expected<bool, TER>
runEscrowWasm(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& escrow_tx_json_data,
    vbytes const& escrow_lo_json_data);

Expected<std::pair<bool, std::string>, TER>
runEscrowWasmP4(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& escrow_tx_json_data,
    vbytes const& escrow_lo_json_data);

struct LedgerDataProvider
{
    virtual int32_t
    get_ledger_sqn()
    {
        return 1;
    }

    virtual ~LedgerDataProvider() = default;
};

Expected<bool, TER>
runEscrowWasm(
    vbytes const& wasmCode,
    std::string_view funcName,
    LedgerDataProvider* ledgerDataProvider);

class WasmEngine
{
public:
    virtual ~WasmEngine() = default;

    virtual Expected<bool, TER>
    run(vbytes const& wasmCode, std::string_view funcName, int32_t input)
    {
        return Unexpected<TER>(tecFAILED_PROCESSING);
    }

    virtual Expected<bool, TER>
    run(vbytes const& wasmCode,
        std::string_view funcName,
        vbytes const& accountID)
    {
        return Unexpected<TER>(tecFAILED_PROCESSING);
    }

    virtual Expected<bool, TER>
    run(vbytes const& wasmCode,
        std::string_view funcName,
        vbytes const& escrow_tx_json_data,
        vbytes const& escrow_lo_json_data)
    {
        return Unexpected<TER>(tecFAILED_PROCESSING);
    }

    virtual Expected<std::pair<bool, std::string>, TER>
    runP4(
        vbytes const& wasmCode,
        std::string_view funcName,
        vbytes const& escrow_tx_json_data,
        vbytes const& escrow_lo_json_data)
    {
        return Unexpected<TER>(tecFAILED_PROCESSING);
    }

    virtual Expected<bool, TER>
    run(vbytes const& wasmCode,
        std::string_view funcName,
        LedgerDataProvider* ledgerDataProvider)
    {
        return Unexpected<TER>(tecFAILED_PROCESSING);
    }

    static std::unique_ptr<WasmEngine>
    instance();
};

}  // namespace ripple
