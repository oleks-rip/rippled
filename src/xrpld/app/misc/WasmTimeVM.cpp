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

#include <xrpld/app/misc/WasmTimeVM.h>

#include <wasmtime.h>

#include <memory>

namespace ripple {

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

class WasmEngineTimeImpl
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
    WasmEngineTimeImpl();
    ~WasmEngineTimeImpl() = default;

    Expected<bool, TER>
    run(vbytes const& wasmCode, std::string_view funcName, int32_t input);

    Expected<bool, TER>
    run(vbytes const& wasmCode,
        std::string_view funcName,
        vbytes const& accountID);

    Expected<bool, TER>
    run(vbytes const& wasmCode,
        std::string_view funcName,
        vbytes const& escrow_tx_json_data,
        vbytes const& escrow_lo_json_data);

    Expected<std::pair<bool, std::string>, TER>
    runP4(
        vbytes const& wasmCode,
        std::string_view funcName,
        vbytes const& escrow_tx_json_data,
        vbytes const& escrow_lo_json_data);

    Expected<bool, TER>
    run(vbytes const& wasmCode,
        std::string_view funcName,
        LedgerDataProvider* ledgerDataProvider);

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
        vbytes const& wasmCode,
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
        vbytes const& p,
        Types... args);
};

void
WasmEngineTimeImpl::print_wasmi_error(
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

WasmEngineTimeImpl::WasmEngineTimeImpl()
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
WasmEngineTimeImpl::makeModule(
    vbytes const& wasmCode,
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
WasmEngineTimeImpl::getFunc(std::string_view funcName)
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
WasmEngineTimeImpl::getMem()
{
    wasmtime_extern_t item;
    memset(&item, 0, sizeof(item));

    bool ok = wasmtime_instance_export_get(
        context, &mod_inst, V_MEM.data(), V_MEM.size(), &item);
    if (!ok || (item.kind != WASMTIME_EXTERN_MEMORY))
        throw std::runtime_error("No wasmtime memory");

    auto* mem = wasmtime_memory_data(context, &item.of.memory);
    return mem;
}

void
WasmEngineTimeImpl::add_param(std::vector<wasmtime_val_t>& in, int32_t p)
{
    in.emplace_back();
    auto& el(in.back());
    el = WASM_I32_VAL(p);
}

void
WasmEngineTimeImpl::add_param(std::vector<wasmtime_val_t>& in, int64_t p)
{
    in.emplace_back();
    auto& el(in.back());
    el = WASM_I64_VAL(p);
}

template <int NR, class... Types>
std::vector<wasmtime_val_t>
WasmEngineTimeImpl::call(std::string_view func, Types... args)
{
    // Lookup our export function
    auto [good, wasmFunc] = getFunc(func);
    if (!good)
        throw std::runtime_error(std::string("Can't find ") + func.data());

    return call<NR>(wasmFunc, std::forward<Types>(args)...);
}

template <int NR, class... Types>
std::vector<wasmtime_val_t>
WasmEngineTimeImpl::call(wasmtime_extern_t const& func, Types... args)
{
    std::vector<wasmtime_val_t> in;
    return call<NR>(func, in, std::forward<Types>(args)...);
}

template <int NR, class... Types>
std::vector<wasmtime_val_t>
WasmEngineTimeImpl::call(
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
WasmEngineTimeImpl::call(
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
WasmEngineTimeImpl::call(
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
WasmEngineTimeImpl::call(
    wasmtime_extern_t const& func,
    std::vector<wasmtime_val_t>& in,
    uint8_t const* m,
    std::size_t sz,
    Types... args)
{
    auto const res = call<1>(V_ALLOC, static_cast<int32_t>(sz));
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
WasmEngineTimeImpl::call(
    wasmtime_extern_t const& func,
    std::vector<wasmtime_val_t>& in,
    vbytes const& p,
    Types... args)
{
    return call<NR>(func, in, p.data(), p.size(), std::forward<Types>(args)...);
}

Expected<bool, TER>
WasmEngineTimeImpl::run(
    vbytes const& wasmCode,
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
WasmEngineTimeImpl::run(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& accountID)
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
WasmEngineTimeImpl::run(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& escrow_tx_json_data,
    vbytes const& escrow_lo_json_data)
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
WasmEngineTimeImpl::runP4(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& escrow_tx_json_data,
    vbytes const& escrow_lo_json_data)
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

    vbytes buf2(ret_len);
    memcpy(buf2.data(), mem + ret_pointer, ret_len);

    std::string newData(buf2.begin(), buf2.end());

    call<0>(V_DEALLOC, ret_pointer, ret_len);
    if (error || trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);
    call<0>(V_DEALLOC, ptr, 9);
    if (error || trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    return std::pair<bool, std::string>(flag == 1, newData);
}

Expected<bool, TER>
WasmEngineTimeImpl::run(
    vbytes const& wasmCode,
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

WasmEngineTime::WasmEngineTime() : impl(std::make_unique<WasmEngineTimeImpl>())
{
}

WasmEngineTime::~WasmEngineTime() = default;

Expected<bool, TER>
WasmEngineTime::run(
    vbytes const& wasmCode,
    std::string_view funcName,
    int32_t input)
{
    try
    {
        return impl->run(wasmCode, funcName, input);
    }
    catch (std::exception const&)
    {
    }
    return Unexpected<TER>(tecFAILED_PROCESSING);
}

Expected<bool, TER>
WasmEngineTime::run(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& accountID)
{
    try
    {
        return impl->run(wasmCode, funcName, accountID);
    }
    catch (std::exception const&)
    {
    }
    return Unexpected<TER>(tecFAILED_PROCESSING);
}

Expected<bool, TER>
WasmEngineTime::run(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& escrow_tx_json_data,
    vbytes const& escrow_lo_json_data)
{
    try
    {
        return impl->run(
            wasmCode, funcName, escrow_tx_json_data, escrow_lo_json_data);
    }
    catch (std::exception const&)
    {
    }
    return Unexpected<TER>(tecFAILED_PROCESSING);
}

Expected<std::pair<bool, std::string>, TER>
WasmEngineTime::runP4(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& escrow_tx_json_data,
    vbytes const& escrow_lo_json_data)
{
    try
    {
        return impl->runP4(
            wasmCode, funcName, escrow_tx_json_data, escrow_lo_json_data);
    }
    catch (std::exception const&)
    {
    }
    return Unexpected<TER>(tecFAILED_PROCESSING);
}

Expected<bool, TER>
WasmEngineTime::run(
    vbytes const& wasmCode,
    std::string_view funcName,
    LedgerDataProvider* ledgerDataProvider)
{
    try
    {
        return impl->run(wasmCode, funcName, ledgerDataProvider);
    }
    catch (std::exception const&)
    {
    }
    return Unexpected<TER>(tecFAILED_PROCESSING);
}
}  // namespace ripple
