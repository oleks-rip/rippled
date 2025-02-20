//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2020 Ripple Labs Inc.

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

#include <xrpld/app/misc/WamrVM.h>
#include <xrpld/app/misc/WasmEdgeVM.h>
#include <xrpld/app/misc/WasmTimeVM.h>
#include <xrpld/app/misc/WasmerVM.h>
#include <xrpld/app/misc/WasmiVM.h>

#include <memory>

namespace ripple {

static wasmEngines g_engine = wasmEngines::Edge;

void
setWasmEngine(wasmEngines engine)
{
    g_engine = engine;
    // printf("Set Engine: %d\n", static_cast<int>(engine));
}

Expected<bool, TER>
runEscrowWasm(vbytes const& wasmCode, std::string_view funcName, int32_t input)
{
    std::unique_ptr<WasmEngine> engine = WasmEngine::instance();
    return engine->run(wasmCode, funcName, input);
}

Expected<bool, TER>
runEscrowWasm(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& accountID)
{
    std::unique_ptr<WasmEngine> engine = WasmEngine::instance();
    return engine->run(wasmCode, funcName, accountID);
}

Expected<bool, TER>
runEscrowWasm(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& escrow_tx_json_data,
    vbytes const& escrow_lo_json_data)
{
    std::unique_ptr<WasmEngine> engine = WasmEngine::instance();
    return engine->run(
        wasmCode, funcName, escrow_tx_json_data, escrow_lo_json_data);
}

Expected<std::pair<bool, std::string>, TER>
runEscrowWasmP4(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& escrow_tx_json_data,
    vbytes const& escrow_lo_json_data)
{
    std::unique_ptr<WasmEngine> engine = WasmEngine::instance();
    return engine->runP4(
        wasmCode, funcName, escrow_tx_json_data, escrow_lo_json_data);
}

Expected<bool, TER>
runEscrowWasm(
    vbytes const& wasmCode,
    std::string_view funcName,
    LedgerDataProvider* ledgerDataProvider)
{
    std::unique_ptr<WasmEngine> engine = WasmEngine::instance();
    return engine->run(wasmCode, funcName, ledgerDataProvider);
}

std::unique_ptr<WasmEngine>
WasmEngine::instance()
{
    switch (g_engine)
    {
        case wasmEngines::Wamr:
            return std::make_unique<WamrEngine>();
        case wasmEngines::I:
            return std::make_unique<WasmEngineI>();
        case wasmEngines::Er:
            return std::make_unique<WasmEngineEr>();
        case wasmEngines::Time:
            return std::make_unique<WasmEngineTime>();
        case wasmEngines::Edge:
        default:
            return std::make_unique<WasmEngineEdge>();
    }
}

}  // namespace ripple
