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

#include <xrpld/app/misc/WasmVM.h>

#include <memory>

namespace ripple {

static wasmEngines g_engine = wasmEngines::Edge;

static const std::string_view M_MEM = "memory";
static const std::string_view M_STORE = "store";
static const std::string_view M_LOAD = "load";
static const std::string_view M_SIZE = "size";

static const std::string_view M_ALLOC = "allocate";
static const std::string_view M_DEALLOC = "deallocate";

void
setWasmEngine(wasmEngines engine)
{
    g_engine = engine;
    // printf("Set Engine: %d\n", static_cast<int>(engine));
}

class WasmEngineEdge;
class WasmEngineTime;

class WasmEngine
{
public:
    virtual ~WasmEngine() = default;

    virtual Expected<bool, TER>
    run(std::vector<uint8_t> const& wasmCode,
        std::string_view funcName,
        int32_t input)
    {
        return Unexpected<TER>(tecFAILED_PROCESSING);
    }

    virtual Expected<bool, TER>
    run(std::vector<uint8_t> const& wasmCode,
        std::string_view funcName,
        std::vector<uint8_t> const& accountID)
    {
        return Unexpected<TER>(tecFAILED_PROCESSING);
    }

    virtual Expected<bool, TER>
    run(std::vector<uint8_t> const& wasmCode,
        std::string_view funcName,
        std::vector<uint8_t> const& escrow_tx_json_data,
        std::vector<uint8_t> const& escrow_lo_json_data)
    {
        return Unexpected<TER>(tecFAILED_PROCESSING);
    }

    virtual Expected<std::pair<bool, std::string>, TER>
    runP4(
        std::vector<uint8_t> const& wasmCode,
        std::string_view funcName,
        std::vector<uint8_t> const& escrow_tx_json_data,
        std::vector<uint8_t> const& escrow_lo_json_data)
    {
        return Unexpected<TER>(tecFAILED_PROCESSING);
    }

    virtual Expected<bool, TER>
    run(std::vector<uint8_t> const& wasmCode,
        std::string_view funcName,
        LedgerDataProvider* ledgerDataProvider)
    {
        return Unexpected<TER>(tecFAILED_PROCESSING);
    }

    static std::unique_ptr<WasmEngine>
    instance();
};

Expected<bool, TER>
runEscrowWasm(
    std::vector<uint8_t> const& wasmCode,
    std::string_view funcName,
    int32_t input)
{
    std::unique_ptr<WasmEngine> engine = WasmEngine::instance();
    return engine->run(wasmCode, funcName, input);
}

Expected<bool, TER>
runEscrowWasm(
    std::vector<uint8_t> const& wasmCode,
    std::string_view funcName,
    std::vector<uint8_t> const& accountID)
{
    std::unique_ptr<WasmEngine> engine = WasmEngine::instance();
    return engine->run(wasmCode, funcName, accountID);
}

Expected<bool, TER>
runEscrowWasm(
    std::vector<uint8_t> const& wasmCode,
    std::string_view funcName,
    std::vector<uint8_t> const& escrow_tx_json_data,
    std::vector<uint8_t> const& escrow_lo_json_data)
{
    std::unique_ptr<WasmEngine> engine = WasmEngine::instance();
    return engine->run(
        wasmCode, funcName, escrow_tx_json_data, escrow_lo_json_data);
}

Expected<std::pair<bool, std::string>, TER>
runEscrowWasmP4(
    std::vector<uint8_t> const& wasmCode,
    std::string_view funcName,
    std::vector<uint8_t> const& escrow_tx_json_data,
    std::vector<uint8_t> const& escrow_lo_json_data)
{
    std::unique_ptr<WasmEngine> engine = WasmEngine::instance();
    return engine->runP4(
        wasmCode, funcName, escrow_tx_json_data, escrow_lo_json_data);
}

Expected<bool, TER>
runEscrowWasm(
    std::vector<uint8_t> const& wasmCode,
    std::string_view funcName,
    LedgerDataProvider* ledgerDataProvider)
{
    std::unique_ptr<WasmEngine> engine = WasmEngine::instance();
    return engine->run(wasmCode, funcName, ledgerDataProvider);
}

WasmEdge_Result
get_ledger_sqn(
    void* data,
    const WasmEdge_CallingFrameContext*,
    const WasmEdge_Value* In,
    WasmEdge_Value* Out)
{
    Out[0] =
        WasmEdge_ValueGenI32(((LedgerDataProvider*)data)->get_ledger_sqn());
    return WasmEdge_Result_Success;
}

//////////////////////////////////////////////////////////////////////////////////////////

class WasmEngineEdge final : public WasmEngine
{
public:
    ~WasmEngineEdge() = default;

    virtual Expected<bool, TER>
    run(std::vector<uint8_t> const& wasmCode,
        std::string_view funcName,
        int32_t input) override;

    virtual Expected<bool, TER>
    run(std::vector<uint8_t> const& wasmCode,
        std::string_view funcName,
        std::vector<uint8_t> const& accountID) override;

    virtual Expected<bool, TER>
    run(std::vector<uint8_t> const& wasmCode,
        std::string_view funcName,
        std::vector<uint8_t> const& escrow_tx_json_data,
        std::vector<uint8_t> const& escrow_lo_json_data) override;

    virtual Expected<std::pair<bool, std::string>, TER>
    runP4(
        std::vector<uint8_t> const& wasmCode,
        std::string_view funcName,
        std::vector<uint8_t> const& escrow_tx_json_data,
        std::vector<uint8_t> const& escrow_lo_json_data) override;

    virtual Expected<bool, TER>
    run(std::vector<uint8_t> const& wasmCode,
        std::string_view funcName,
        LedgerDataProvider* ledgerDataProvider) override;
};

Expected<bool, TER>
WasmEngineEdge::run(
    std::vector<uint8_t> const& wasmCode,
    std::string_view funcName,
    int32_t input)
{
    WasmEdge_VMContext* VMCxt = WasmEdge_VMCreate(NULL, NULL);
    WasmEdge_Value Params[1] = {WasmEdge_ValueGenI32(input)};
    WasmEdge_Value Returns[1];
    WasmEdge_String FuncName = WasmEdge_StringCreateByCString(funcName.data());
    WasmEdge_Result Res = WasmEdge_VMRunWasmFromBuffer(
        VMCxt,
        wasmCode.data(),
        wasmCode.size(),
        FuncName,
        Params,
        1,
        Returns,
        1);

    bool ok = WasmEdge_ResultOK(Res);
    bool re = false;
    if (ok)
    {
        auto result = WasmEdge_ValueGetI32(Returns[0]);
        // printf("Get the result: %d\n", result);
        if (result != 0)
            re = true;
    }
    else
    {
        printf("Error message: %s\n", WasmEdge_ResultGetMessage(Res));
    }

    WasmEdge_VMDelete(VMCxt);
    WasmEdge_StringDelete(FuncName);
    if (ok)
        return re;
    else
        return Unexpected<TER>(tecFAILED_PROCESSING);
}

Expected<bool, TER>
WasmEngineEdge::run(
    std::vector<uint8_t> const& wasmCode,
    std::string_view funcName,
    std::vector<uint8_t> const& accountID)
{
    auto dataLen = (int32_t)accountID.size();
    // printf("accountID size: %d\n", dataLen);
    WasmEdge_VMContext* VMCxt = WasmEdge_VMCreate(NULL, NULL);

    WasmEdge_Value allocParams[1] = {WasmEdge_ValueGenI32(dataLen)};
    WasmEdge_Value allocReturns[1];
    WasmEdge_String allocFunc = WasmEdge_StringCreateByCString("allocate");
    WasmEdge_Result allocRes = WasmEdge_VMRunWasmFromBuffer(
        VMCxt,
        wasmCode.data(),
        wasmCode.size(),
        allocFunc,
        allocParams,
        1,
        allocReturns,
        1);

    bool ok = WasmEdge_ResultOK(allocRes);
    bool re = false;
    if (ok)
    {
        auto pointer = WasmEdge_ValueGetI32(allocReturns[0]);
        // printf("Alloc pointer: %d\n", pointer);

        const WasmEdge_ModuleInstanceContext* m =
            WasmEdge_VMGetActiveModule(VMCxt);
        WasmEdge_String mName = WasmEdge_StringCreateByCString("memory");
        WasmEdge_MemoryInstanceContext* mi =
            WasmEdge_ModuleInstanceFindMemory(m, mName);
        WasmEdge_Result setRes = WasmEdge_MemoryInstanceSetData(
            mi, accountID.data(), pointer, dataLen);

        ok = WasmEdge_ResultOK(setRes);
        if (ok)
        {
            // printf("Set data ok\n");

            WasmEdge_Value params[2] = {
                WasmEdge_ValueGenI32(pointer), WasmEdge_ValueGenI32(dataLen)};
            WasmEdge_Value returns[1];
            WasmEdge_String func =
                WasmEdge_StringCreateByCString(funcName.data());
            WasmEdge_Result funcRes =
                WasmEdge_VMExecute(VMCxt, func, params, 2, returns, 1);

            ok = WasmEdge_ResultOK(funcRes);
            if (ok)
            {
                // printf("func ok\n");
                re = (WasmEdge_ValueGetI32(returns[0]) == 1);
            }
            else
            {
                printf(
                    "Func message: %s\n", WasmEdge_ResultGetMessage(funcRes));
            }
        }
        else
        {
            printf(
                "Set error message: %s\n", WasmEdge_ResultGetMessage(setRes));
        }
    }
    else
    {
        printf(
            "Alloc error message: %s\n", WasmEdge_ResultGetMessage(allocRes));
    }

    WasmEdge_VMDelete(VMCxt);
    // TODO free everything
    //    WasmEdge_StringDelete(FuncName);
    if (ok)
    {
        // printf("runEscrowWasm ok, result %d\n", re);
        return re;
    }
    else
        return Unexpected<TER>(tecFAILED_PROCESSING);
}

Expected<bool, TER>
WasmEngineEdge::run(
    std::vector<uint8_t> const& wasmCode,
    std::string_view funcName,
    std::vector<uint8_t> const& escrow_tx_json_data,
    std::vector<uint8_t> const& escrow_lo_json_data)
{
    WasmEdge_VMContext* VMCxt = WasmEdge_VMCreate(NULL, NULL);

    WasmEdge_Result loadRes =
        WasmEdge_VMLoadWasmFromBuffer(VMCxt, wasmCode.data(), wasmCode.size());
    if (!WasmEdge_ResultOK(loadRes))
    {
        printf("load error\n");
        return Unexpected<TER>(tecFAILED_PROCESSING);
    }

    WasmEdge_Result validateRes = WasmEdge_VMValidate(VMCxt);
    if (!WasmEdge_ResultOK(validateRes))
    {
        printf("validate error\n");
        return Unexpected<TER>(tecFAILED_PROCESSING);
    }

    WasmEdge_Result instantiateRes = WasmEdge_VMInstantiate(VMCxt);
    if (!WasmEdge_ResultOK(instantiateRes))
    {
        printf("instantiate error\n");
        return Unexpected<TER>(tecFAILED_PROCESSING);
    }

    auto wasmAlloc = [VMCxt](std::vector<uint8_t> const& data) -> int32_t {
        auto dataLen = (int32_t)data.size();
        WasmEdge_Value allocParams[1] = {WasmEdge_ValueGenI32(dataLen)};
        WasmEdge_Value allocReturns[1];
        WasmEdge_String allocFunc = WasmEdge_StringCreateByCString("allocate");

        WasmEdge_Result allocRes = WasmEdge_VMExecute(
            VMCxt, allocFunc, allocParams, 1, allocReturns, 1);

        if (WasmEdge_ResultOK(allocRes))
        {
            auto pointer = WasmEdge_ValueGetI32(allocReturns[0]);
            //            printf("alloc ptr %d, len %d\n", pointer, dataLen);
            const WasmEdge_ModuleInstanceContext* m =
                WasmEdge_VMGetActiveModule(VMCxt);
            WasmEdge_String mName = WasmEdge_StringCreateByCString("memory");
            WasmEdge_MemoryInstanceContext* mi =
                WasmEdge_ModuleInstanceFindMemory(m, mName);
            WasmEdge_Result setRes = WasmEdge_MemoryInstanceSetData(
                mi, data.data(), pointer, dataLen);
            if (WasmEdge_ResultOK(setRes))
            {
                return pointer;
            }
        }

        return 0;
    };

    auto tx_ptr = wasmAlloc(escrow_tx_json_data);
    auto lo_ptr = wasmAlloc(escrow_lo_json_data);
    if (tx_ptr == 0 || lo_ptr == 0)
    {
        printf("data error\n");
        return Unexpected<TER>(tecFAILED_PROCESSING);
    }

    auto txLen = (int32_t)escrow_tx_json_data.size();
    auto loLen = (int32_t)escrow_lo_json_data.size();

    WasmEdge_Value params[4] = {
        WasmEdge_ValueGenI32(tx_ptr),
        WasmEdge_ValueGenI32(txLen),
        WasmEdge_ValueGenI32(lo_ptr),
        WasmEdge_ValueGenI32(loLen)};
    WasmEdge_Value returns[1];
    WasmEdge_String func = WasmEdge_StringCreateByCString(funcName.data());
    WasmEdge_Result funcRes =
        WasmEdge_VMExecute(VMCxt, func, params, 4, returns, 1);

    if (WasmEdge_ResultOK(funcRes))
    {
        // printf("func ok\n");
        return WasmEdge_ValueGetI32(returns[0]) == 1;
    }
    else
    {
        printf("Func message: %s\n", WasmEdge_ResultGetMessage(funcRes));
        return Unexpected<TER>(tecFAILED_PROCESSING);
    }
}

Expected<std::pair<bool, std::string>, TER>
WasmEngineEdge::runP4(
    std::vector<uint8_t> const& wasmCode,
    std::string_view funcName,
    std::vector<uint8_t> const& escrow_tx_json_data,
    std::vector<uint8_t> const& escrow_lo_json_data)
{
    WasmEdge_VMContext* VMCxt = WasmEdge_VMCreate(NULL, NULL);

    WasmEdge_Result loadRes =
        WasmEdge_VMLoadWasmFromBuffer(VMCxt, wasmCode.data(), wasmCode.size());
    if (!WasmEdge_ResultOK(loadRes))
    {
        printf("load error\n");
        return Unexpected<TER>(tecFAILED_PROCESSING);
    }

    WasmEdge_Result validateRes = WasmEdge_VMValidate(VMCxt);
    if (!WasmEdge_ResultOK(validateRes))
    {
        printf("validate error\n");
        return Unexpected<TER>(tecFAILED_PROCESSING);
    }

    WasmEdge_Result instantiateRes = WasmEdge_VMInstantiate(VMCxt);
    if (!WasmEdge_ResultOK(instantiateRes))
    {
        printf("instantiate error\n");
        return Unexpected<TER>(tecFAILED_PROCESSING);
    }

    auto wasmAlloc = [VMCxt](std::vector<uint8_t> const& data) -> int32_t {
        auto dataLen = (int32_t)data.size();
        WasmEdge_Value allocParams[1] = {WasmEdge_ValueGenI32(dataLen)};
        WasmEdge_Value allocReturns[1];
        WasmEdge_String allocFunc = WasmEdge_StringCreateByCString("allocate");

        WasmEdge_Result allocRes = WasmEdge_VMExecute(
            VMCxt, allocFunc, allocParams, 1, allocReturns, 1);

        if (WasmEdge_ResultOK(allocRes))
        {
            auto pointer = WasmEdge_ValueGetI32(allocReturns[0]);
            //            printf("alloc ptr %d, len %d\n", pointer, dataLen);
            const WasmEdge_ModuleInstanceContext* m =
                WasmEdge_VMGetActiveModule(VMCxt);
            WasmEdge_String mName = WasmEdge_StringCreateByCString("memory");
            WasmEdge_MemoryInstanceContext* mi =
                WasmEdge_ModuleInstanceFindMemory(m, mName);
            WasmEdge_Result setRes = WasmEdge_MemoryInstanceSetData(
                mi, data.data(), pointer, dataLen);
            if (WasmEdge_ResultOK(setRes))
            {
                return pointer;
            }
        }

        return 0;
    };

    auto tx_ptr = wasmAlloc(escrow_tx_json_data);
    auto lo_ptr = wasmAlloc(escrow_lo_json_data);
    if (tx_ptr == 0 || lo_ptr == 0)
    {
        printf("data error\n");
        return Unexpected<TER>(tecFAILED_PROCESSING);
    }

    auto txLen = (int32_t)escrow_tx_json_data.size();
    auto loLen = (int32_t)escrow_lo_json_data.size();

    WasmEdge_Value params[4] = {
        WasmEdge_ValueGenI32(tx_ptr),
        WasmEdge_ValueGenI32(txLen),
        WasmEdge_ValueGenI32(lo_ptr),
        WasmEdge_ValueGenI32(loLen)};
    WasmEdge_Value returns[1];
    WasmEdge_String func = WasmEdge_StringCreateByCString(funcName.data());
    WasmEdge_Result funcRes =
        WasmEdge_VMExecute(VMCxt, func, params, 4, returns, 1);

    if (WasmEdge_ResultOK(funcRes))
    {
        auto pointer = WasmEdge_ValueGetI32(returns[0]);
        const WasmEdge_ModuleInstanceContext* m =
            WasmEdge_VMGetActiveModule(VMCxt);
        WasmEdge_String mName = WasmEdge_StringCreateByCString("memory");
        WasmEdge_MemoryInstanceContext* mi =
            WasmEdge_ModuleInstanceFindMemory(m, mName);
        uint8_t buff[9];
        WasmEdge_Result getRes =
            WasmEdge_MemoryInstanceGetData(mi, buff, pointer, 9);
        if (!WasmEdge_ResultOK(getRes))
        {
            printf(
                "re mem get message: %s\n", WasmEdge_ResultGetMessage(getRes));
            return Unexpected<TER>(tecFAILED_PROCESSING);
        }
        auto flag = buff[0];

        auto leToInt32 = [](const uint8_t* d) -> uint32_t {
            uint32_t r = 0;
            for (int i = 0; i < 4; ++i)
            {
                r |= static_cast<uint32_t>(d[i]) << (i * 8);
                //                printf("leToInt32 %d\n", r);
            }
            return r;
        };
        auto ret_pointer =
            leToInt32(reinterpret_cast<const uint8_t*>(&buff[1]));
        auto ret_len = leToInt32(reinterpret_cast<const uint8_t*>(&buff[5]));
        //        printf("re flag %d, ptr %d, len %d\n", flag, ret_pointer,
        //        ret_len);

        std::vector<uint8_t> buff2(ret_len);
        getRes = WasmEdge_MemoryInstanceGetData(
            mi, buff2.data(), ret_pointer, ret_len);
        if (!WasmEdge_ResultOK(getRes))
        {
            printf(
                "re 2 mem get message: %s\n",
                WasmEdge_ResultGetMessage(getRes));
            return Unexpected<TER>(tecFAILED_PROCESSING);
        }

        std::string newData(buff2.begin(), buff2.end());

        // free
        WasmEdge_String freeFunc = WasmEdge_StringCreateByCString("deallocate");
        WasmEdge_Value freeParams[2] = {
            WasmEdge_ValueGenI32(ret_pointer), WasmEdge_ValueGenI32(ret_len)};
        WasmEdge_Value freeReturns[0];
        WasmEdge_VMExecute(VMCxt, freeFunc, freeParams, 2, freeReturns, 0);
        // free pointer too, with len = 9 too
        freeParams[0] = WasmEdge_ValueGenI32(pointer);
        freeParams[1] = WasmEdge_ValueGenI32(9);
        WasmEdge_VMExecute(VMCxt, freeFunc, freeParams, 2, freeReturns, 0);

        return std::pair<bool, std::string>(flag == 1, newData);
    }
    else
    {
        printf("Func message: %s\n", WasmEdge_ResultGetMessage(funcRes));
        return Unexpected<TER>(tecFAILED_PROCESSING);
    }
}

Expected<bool, TER>
WasmEngineEdge::run(
    std::vector<uint8_t> const& wasmCode,
    std::string_view funcName,
    LedgerDataProvider* ledgerDataProvider)
{
    WasmEdge_VMContext* VMCxt = WasmEdge_VMCreate(NULL, NULL);
    {  // register host function
        WasmEdge_ValType ReturnList[1] = {WasmEdge_ValTypeGenI32()};
        WasmEdge_FunctionTypeContext* HostFType =
            WasmEdge_FunctionTypeCreate(NULL, 0, ReturnList, 1);
        WasmEdge_FunctionInstanceContext* HostFunc =
            WasmEdge_FunctionInstanceCreate(
                HostFType, get_ledger_sqn, ledgerDataProvider, 0);
        WasmEdge_FunctionTypeDelete(HostFType);

        WasmEdge_String HostName = WasmEdge_StringCreateByCString("host_lib");
        WasmEdge_ModuleInstanceContext* HostMod =
            WasmEdge_ModuleInstanceCreate(HostName);
        WasmEdge_StringDelete(HostName);

        WasmEdge_String HostFuncName =
            WasmEdge_StringCreateByCString("get_ledger_sqn");
        WasmEdge_ModuleInstanceAddFunction(HostMod, HostFuncName, HostFunc);
        WasmEdge_StringDelete(HostFuncName);

        WasmEdge_Result regRe =
            WasmEdge_VMRegisterModuleFromImport(VMCxt, HostMod);
        if (!WasmEdge_ResultOK(regRe))
        {
            printf("host func reg error\n");
            return Unexpected<TER>(tecFAILED_PROCESSING);
        }
    }
    WasmEdge_Result loadRes =
        WasmEdge_VMLoadWasmFromBuffer(VMCxt, wasmCode.data(), wasmCode.size());
    if (!WasmEdge_ResultOK(loadRes))
    {
        printf("load error\n");
        return Unexpected<TER>(tecFAILED_PROCESSING);
    }
    WasmEdge_Result validateRes = WasmEdge_VMValidate(VMCxt);
    if (!WasmEdge_ResultOK(validateRes))
    {
        printf("validate error\n");
        return Unexpected<TER>(tecFAILED_PROCESSING);
    }
    WasmEdge_Result instantiateRes = WasmEdge_VMInstantiate(VMCxt);
    if (!WasmEdge_ResultOK(instantiateRes))
    {
        printf("instantiate error\n");
        return Unexpected<TER>(tecFAILED_PROCESSING);
    }

    WasmEdge_Value funcReturns[1];
    memset(funcReturns, 0, sizeof(funcReturns));
    WasmEdge_String func = WasmEdge_StringCreateByCString(funcName.data());

    WasmEdge_Result funcRes =
        WasmEdge_VMExecute(VMCxt, func, NULL, 0, funcReturns, 1);

    bool ok = WasmEdge_ResultOK(funcRes);
    bool re = false;
    if (ok)
    {
        auto result = WasmEdge_ValueGetI32(funcReturns[0]);
        if (result != 0)
            re = true;
    }
    else
    {
        printf("Error message: %s\n", WasmEdge_ResultGetMessage(funcRes));
    }

    WasmEdge_VMDelete(VMCxt);
    WasmEdge_StringDelete(func);
    if (ok)
        return re;
    else
        return Unexpected<TER>(tecFAILED_PROCESSING);
}

//////////////////////////////////////////////////////////////////////////////////////////

class WasmEngineTime final : public WasmEngine
{
    std::unique_ptr<wasm_engine_t, decltype(&wasm_engine_delete)> engine;
    std::unique_ptr<wasmtime_store_t, decltype(&wasmtime_store_delete)> store;
    wasmtime_context_t* context = nullptr;
    wasmtime_error_t* error = nullptr;
    std::unique_ptr<wasmtime_module_t, decltype(&wasmtime_module_delete)>
        module;
    wasmtime_instance_t mod_inst;
    wasm_trap_t* trap = nullptr;

public:
    WasmEngineTime();
    ~WasmEngineTime() = default;

    virtual Expected<bool, TER>
    run(std::vector<uint8_t> const& wasmCode,
        std::string_view funcName,
        int32_t input) override;

    virtual Expected<bool, TER>
    run(std::vector<uint8_t> const& wasmCode,
        std::string_view funcName,
        std::vector<uint8_t> const& accountID) override;

    virtual Expected<bool, TER>
    run(std::vector<uint8_t> const& wasmCode,
        std::string_view funcName,
        std::vector<uint8_t> const& escrow_tx_json_data,
        std::vector<uint8_t> const& escrow_lo_json_data) override;

    virtual Expected<std::pair<bool, std::string>, TER>
    runP4(
        std::vector<uint8_t> const& wasmCode,
        std::string_view funcName,
        std::vector<uint8_t> const& escrow_tx_json_data,
        std::vector<uint8_t> const& escrow_lo_json_data) override;

    virtual Expected<bool, TER>
    run(std::vector<uint8_t> const& wasmCode,
        std::string_view funcName,
        LedgerDataProvider* ledgerDataProvider) override;

protected:
    static void
    print_wasmi_error(
        const char* message,
        wasmtime_error_t* error,
        wasm_trap_t* trap);

    Expected<bool, TER>
    runSimple(std::string_view funcName, int32_t input);

    wasmtime_error_t*
    makeModule(
        std::vector<uint8_t> const& wasmCode,
        std::vector<wasmtime_extern_t> const& import = {});

    std::pair<bool, wasmtime_extern_t>
    getFunc(std::string_view funcName);

    uint8_t*
    getMem();

    void
    add_param(std::vector<wasmtime_val_t>& in, int32_t p);
    void
    add_param(std::vector<wasmtime_val_t>& in, int64_t p);

    template <int NR, class... Types>
    inline std::vector<wasmtime_val_t>
    call(std::string_view func, Types... args);

    template <int NR, class... Types>
    inline std::vector<wasmtime_val_t>
    call(wasmtime_extern_t const& func, Types... args);

    template <int NR, class... Types>
    std::vector<wasmtime_val_t>
    call(wasmtime_extern_t const& func, std::vector<wasmtime_val_t>& in);

    template <int NR, class... Types>
    inline std::vector<wasmtime_val_t>
    call(
        wasmtime_extern_t const& func,
        std::vector<wasmtime_val_t>& in,
        std::int32_t p,
        Types... args);

    template <int NR, class... Types>
    inline std::vector<wasmtime_val_t>
    call(
        wasmtime_extern_t const& func,
        std::vector<wasmtime_val_t>& in,
        std::int64_t p,
        Types... args);

    template <int NR, class... Types>
    inline std::vector<wasmtime_val_t>
    call(
        wasmtime_extern_t const& func,
        std::vector<wasmtime_val_t>& in,
        uint8_t const* m,
        std::size_t sz,
        Types... args);

    template <int NR, class... Types>
    inline std::vector<wasmtime_val_t>
    call(
        wasmtime_extern_t const& func,
        std::vector<wasmtime_val_t>& in,
        std::vector<uint8_t> const& p,
        Types... args);
};

void
WasmEngineTime::print_wasmi_error(
    const char* message,
    wasmtime_error_t* error,
    wasm_trap_t* trap)
{
    fprintf(stderr, "error: %s\n", message);
    wasm_byte_vec_t error_message;
    if (error != NULL)
    {
        wasmtime_error_message(error, &error_message);
        wasmtime_error_delete(error);
    }
    else
    {
        wasm_trap_message(trap, &error_message);
        wasm_trap_delete(trap);
    }
    fprintf(stderr, "%.*s\n", (int)error_message.size, error_message.data);
    wasm_byte_vec_delete(&error_message);
}

WasmEngineTime::WasmEngineTime()
    : engine(wasm_engine_new(), &wasm_engine_delete)
    , store(
          wasmtime_store_new(engine.get(), nullptr, nullptr),
          &wasmtime_store_delete)
    , context(wasmtime_store_context(store.get()))
    , module(nullptr, &wasmtime_module_delete)
{
    memset(&mod_inst, 0, sizeof(mod_inst));
}

wasmtime_error_t*
WasmEngineTime::makeModule(
    std::vector<uint8_t> const& wasmCode,
    std::vector<wasmtime_extern_t> const& import)
{
    wasmtime_module_t* m = nullptr;
    error =
        wasmtime_module_new(engine.get(), wasmCode.data(), wasmCode.size(), &m);
    if (error)
    {
        print_wasmi_error("failed to compile module", error, nullptr);
        return error;
    }

    module = decltype(module)(m, &wasmtime_module_delete);
    error = wasmtime_instance_new(
        context,
        module.get(),
        import.empty() ? nullptr : import.data(),
        import.size(),
        &mod_inst,
        &trap);
    if (error || trap)
        print_wasmi_error("failed to instantiate module", error, trap);

    return error;
}

std::pair<bool, wasmtime_extern_t>
WasmEngineTime::getFunc(std::string_view funcName)
{
    // Lookup our export function
    wasmtime_extern_t wasmFunc;
    memset(&wasmFunc, 0, sizeof(wasmFunc));

    if (!wasmtime_instance_export_get(
            context, &mod_inst, funcName.data(), funcName.size(), &wasmFunc) ||
        (wasmFunc.kind != WASMTIME_EXTERN_FUNC))
    {
        printf("Can't find: %s\n", funcName.data());
        return std::make_pair(false, wasmFunc);
    }

    return std::make_pair(true, wasmFunc);
}

uint8_t*
WasmEngineTime::getMem()
{
    wasmtime_extern_t item;
    memset(&item, 0, sizeof(item));

    bool ok = wasmtime_instance_export_get(
        context, &mod_inst, M_MEM.data(), M_MEM.size(), &item);
    if (!ok || (item.kind != WASMTIME_EXTERN_MEMORY))
        throw std::runtime_error("No wasmtime memory");

    auto* mem = wasmtime_memory_data(context, &item.of.memory);
    return mem;
}

void
WasmEngineTime::add_param(std::vector<wasmtime_val_t>& in, int32_t p)
{
    in.emplace_back();
    auto& el(in.back());
    memset(&el, 0, sizeof(el));
    el.kind = WASMTIME_I32;
    el.of.i32 = p;
}

void
WasmEngineTime::add_param(std::vector<wasmtime_val_t>& in, int64_t p)
{
    in.emplace_back();
    auto& el(in.back());
    memset(&el, 0, sizeof(el));
    el.kind = WASMTIME_I64;
    el.of.i32 = p;
}

template <int NR, class... Types>
std::vector<wasmtime_val_t>
WasmEngineTime::call(std::string_view func, Types... args)
{
    // Lookup our export function
    auto [good, wasmFunc] = getFunc(func);
    if (!good)
        throw std::runtime_error(std::string("Can't find ") + func.data());

    return call<NR>(wasmFunc, std::forward<Types>(args)...);
}

template <int NR, class... Types>
std::vector<wasmtime_val_t>
WasmEngineTime::call(wasmtime_extern_t const& func, Types... args)
{
    std::vector<wasmtime_val_t> in;
    return call<NR>(func, in, std::forward<Types>(args)...);
}

template <int NR, class... Types>
std::vector<wasmtime_val_t>
WasmEngineTime::call(
    wasmtime_extern_t const& func,
    std::vector<wasmtime_val_t>& in)
{
    std::vector<wasmtime_val_t> ret;
    if (NR)
    {
        ret.resize(NR);
        memset(ret.data(), 0, NR * sizeof(wasmtime_val_t));
    }

    error = wasmtime_func_call(
        context,
        &func.of.func,
        in.data(),
        in.size(),
        NR ? ret.data() : nullptr,
        NR,
        &trap);
    if (error || trap)
        print_wasmi_error("failed to call func", error, trap);
    // assert(results[0].kind == WASMTIME_I32);
    // if (NR) printf("Result P5: %d\n", ret[0].of.i32);

    return ret;
}

template <int NR, class... Types>
std::vector<wasmtime_val_t>
WasmEngineTime::call(
    wasmtime_extern_t const& func,
    std::vector<wasmtime_val_t>& in,
    std::int32_t p,
    Types... args)
{
    add_param(in, p);
    return call<NR>(func, in, std::forward<Types>(args)...);
}

template <int NR, class... Types>
std::vector<wasmtime_val_t>
WasmEngineTime::call(
    wasmtime_extern_t const& func,
    std::vector<wasmtime_val_t>& in,
    std::int64_t p,
    Types... args)
{
    add_param(in, p);
    return call<NR>(func, in, std::forward<Types>(args)...);
}

template <int NR, class... Types>
inline std::vector<wasmtime_val_t>
WasmEngineTime::call(
    wasmtime_extern_t const& func,
    std::vector<wasmtime_val_t>& in,
    uint8_t const* m,
    std::size_t sz,
    Types... args)
{
    auto const res = call<1>(M_ALLOC, static_cast<int32_t>(sz));
    if (error || trap || (res[0].kind != WASMTIME_I32))
        return {};
    auto const ptr = res[0].of.i32;

    auto* mem = getMem();
    memcpy(mem + ptr, m, sz);

    add_param(in, ptr);
    add_param(in, static_cast<int32_t>(sz));
    return call<NR>(func, in, std::forward<Types>(args)...);
}

template <int NR, class... Types>
inline std::vector<wasmtime_val_t>
WasmEngineTime::call(
    wasmtime_extern_t const& func,
    std::vector<wasmtime_val_t>& in,
    std::vector<uint8_t> const& p,
    Types... args)
{
    return call<NR>(func, in, p.data(), p.size(), std::forward<Types>(args)...);
}

Expected<bool, TER>
WasmEngineTime::run(
    std::vector<uint8_t> const& wasmCode,
    std::string_view funcName,
    int32_t input)
{
    // Create and instantiate the module.
    if (makeModule(wasmCode))
        return Unexpected<TER>(tecFAILED_PROCESSING);

    // Call it!
    auto const res = call<1>(funcName, input);
    if (error || trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    assert(res[0].kind == WASMTIME_I32);
    // printf("Result: %d\n", results[0].of.i32);
    return res[0].of.i32 != 0;
}

Expected<bool, TER>
WasmEngineTime::run(
    std::vector<uint8_t> const& wasmCode,
    std::string_view funcName,
    std::vector<uint8_t> const& accountID)
{
    // Create and instantiate the module.
    if (makeModule(wasmCode))
        return Unexpected<TER>(tecFAILED_PROCESSING);

    auto const res = call<1>(funcName, accountID);
    if (error || trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    assert(res[0].kind == WASMTIME_I32);
    // printf("Result: %d\n", results[0].of.i32);
    return res[0].of.i32 == 1;
}

Expected<bool, TER>
WasmEngineTime::run(
    std::vector<uint8_t> const& wasmCode,
    std::string_view funcName,
    std::vector<uint8_t> const& escrow_tx_json_data,
    std::vector<uint8_t> const& escrow_lo_json_data)
{
    // Create and instantiate the module.
    if (makeModule(wasmCode))
        return Unexpected<TER>(tecFAILED_PROCESSING);

    auto const res =
        call<1>(funcName, escrow_tx_json_data, escrow_lo_json_data);
    if (error || trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    assert(res[0].kind == WASMTIME_I32);
    // printf("Result: %d\n", results[0].of.i32);
    return res[0].of.i32 == 1;
}

Expected<std::pair<bool, std::string>, TER>
WasmEngineTime::runP4(
    std::vector<uint8_t> const& wasmCode,
    std::string_view funcName,
    std::vector<uint8_t> const& escrow_tx_json_data,
    std::vector<uint8_t> const& escrow_lo_json_data)
{
    // Create and instantiate the module.
    if (makeModule(wasmCode))
        return Unexpected<TER>(tecFAILED_PROCESSING);

    auto const res =
        call<1>(funcName, escrow_tx_json_data, escrow_lo_json_data);
    if (error || trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    assert(res[0].kind == WASMTIME_I32);
    // printf("Result: %d\n", results[0].of.i32);
    // return res[0].of.i32 == 1;
    auto const ptr = res[0].of.i32;
    std::uint8_t buf[16];
    memset(buf, 0, sizeof(buf));

    auto const* const mem = getMem();
    memcpy(buf, mem + ptr, 9);

    auto const flag = buf[0];
    auto const ret_pointer = *reinterpret_cast<int32_t const*>(buf + 1);
    auto const ret_len = *reinterpret_cast<int32_t const*>(buf + 5);
    // printf("re flag %d, ptr %d, len %d\n", flag, ret_pointer, ret_len);

    std::vector<uint8_t> buf2(ret_len);
    memcpy(buf2.data(), mem + ret_pointer, ret_len);

    std::string newData(buf2.begin(), buf2.end());

    call<0>(M_DEALLOC, ret_pointer, ret_len);
    if (error || trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);
    call<0>(M_DEALLOC, ptr, 9);
    if (error || trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    return std::pair<bool, std::string>(flag == 1, newData);
}

static wasm_trap_t*
get_ledger_sqn_WTime(
    void* env,
    wasmtime_caller_t*,
    const wasmtime_val_t*,
    size_t,
    wasmtime_val_t* results,
    size_t nresults)
{
    auto sqn = reinterpret_cast<LedgerDataProvider*>(env)->get_ledger_sqn();
    if (nresults)
    {
        results[0].kind = WASMTIME_I32;
        results[0].of.i32 = sqn;
    }
    return nullptr;
}

Expected<bool, TER>
WasmEngineTime::run(
    std::vector<uint8_t> const& wasmCode,
    std::string_view funcName,
    LedgerDataProvider* ledgerDataProvider)
{
    std::unique_ptr<wasm_valtype_t, decltype(&wasm_valtype_delete)> ntype(
        wasm_valtype_new_i32(), &wasm_valtype_delete);

    // std::unique_ptr<wasm_functype_t, decltype(&wasm_functype_delete)>
    // HostFType(wasm_functype_new_0_1(ntype.get()), &wasm_functype_delete);
    wasm_functype_t* HostFType = wasm_functype_new_0_1(ntype.get());

    wasmtime_func_t HostFunc;
    memset(&HostFunc, 0, sizeof(HostFunc));
    wasmtime_func_new(
        context,
        HostFType,
        get_ledger_sqn_WTime,
        ledgerDataProvider,
        nullptr,
        &HostFunc);

    wasmtime_extern_t import;
    import.kind = WASMTIME_EXTERN_FUNC;
    import.of.func = HostFunc;
    if (makeModule(wasmCode, {import}))
        return Unexpected<TER>(tecFAILED_PROCESSING);

    auto res = call<1>(funcName);
    if (error || trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    // wasm_functype_delete(HostFType); // crash
    return !res.empty() && res[0].kind == WASMTIME_I32 && res[0].of.i32;
}

//////////////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<WasmEngine>
WasmEngine::instance()
{
    switch (g_engine)
    {
        case wasmEngines::Time:
            return std::make_unique<WasmEngineTime>();
        case wasmEngines::Edge:
        default:
            return std::make_unique<WasmEngineEdge>();
    }
}

}  // namespace ripple
