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

#include <xrpld/app/misc/WasmiVM.h>

#include <wasmi_so.h>

#include <memory>

namespace ripple {

//////////////////////////////////////////////////////////////////////////////////////////

static wasm_trap_t*
get_ledger_sqn_WTime(void* env, const wasm_val_vec_t*, wasm_val_vec_t* results)
{
    auto sqn = reinterpret_cast<LedgerDataProvider*>(env)->get_ledger_sqn();
    if (results->size)
    {
        results->data[0] = WASM_I32_VAL(sqn);
    }

    return nullptr;
}

using uvec = std::unique_ptr<wasm_val_vec_t, decltype(&wasmi_val_vec_delete)>;

class WasmEngineIImpl
{
    std::unique_ptr<wasm_engine_t, decltype(&wasmi_engine_delete)> engine;
    std::unique_ptr<wasm_store_t, decltype(&wasmi_store_delete2)> store;
    std::unique_ptr<wasm_module_t, decltype(&wasmi_module_delete)> module;
    std::unique_ptr<wasm_instance_t, decltype(&wasmi_instance_delete)> mod_inst;
    // wasmtime_context_t* context = nullptr;
    // wasmtime_error_t* error = nullptr;
    wasm_trap_t* trap = nullptr;

    wasm_exporttype_vec_t export_types = {0, nullptr};
    wasm_extern_vec_t exports = {0, nullptr};

public:
    WasmEngineIImpl();
    ~WasmEngineIImpl();

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
    print_wasmi_error(const char* message, wasm_trap_t* trap);

    bool
    makeModule(
        vbytes const& wasmCode,
        wasm_extern_vec_t const& imports = WASM_EMPTY_VEC);

    wasm_func_t*
    getFunc(std::string_view funcName);

    vmem
    getMem();

    void
    add_param(std::vector<wasm_val_t>& in, int32_t p);
    void
    add_param(std::vector<wasm_val_t>& in, int64_t p);

    template <int NR, class... Types>
    inline wasm_val_vec_t
    call(std::string_view func, Types... args);

    template <int NR, class... Types>
    inline wasm_val_vec_t
    call(wasm_func_t* func, Types... args);

    template <int NR, class... Types>
    wasm_val_vec_t
    call(wasm_func_t* f, std::vector<wasm_val_t>& in);

    template <int NR, class... Types>
    inline wasm_val_vec_t
    call(
        wasm_func_t* func,
        std::vector<wasm_val_t>& in,
        std::int32_t p,
        Types... args);

    template <int NR, class... Types>
    inline wasm_val_vec_t
    call(
        wasm_func_t* func,
        std::vector<wasm_val_t>& in,
        std::int64_t p,
        Types... args);

    template <int NR, class... Types>
    inline wasm_val_vec_t
    call(
        wasm_func_t* func,
        std::vector<wasm_val_t>& in,
        uint8_t const* m,
        std::size_t sz,
        Types... args);

    template <int NR, class... Types>
    inline wasm_val_vec_t
    call(
        wasm_func_t* func,
        std::vector<wasm_val_t>& in,
        vbytes const& p,
        Types... args);
};

void
WasmEngineIImpl::print_wasmi_error(const char* message, wasm_trap_t* trap)
{
    fprintf(stderr, "error: %s\n", message);
    wasm_byte_vec_t error_message;

    if (trap)
    {
        wasmi_trap_message(trap, &error_message);
        wasmi_trap_delete(trap);
    }
    fprintf(stderr, "%.*s\n", (int)error_message.size, error_message.data);
    wasmi_byte_vec_delete(&error_message);
}

WasmEngineIImpl::WasmEngineIImpl()
    : engine(wasmi_engine_new(), &wasmi_engine_delete)
    , store(wasmi_store_new2(engine.get()), &wasmi_store_delete2)
    , module(nullptr, &wasmi_module_delete)
    , mod_inst(nullptr, &wasmi_instance_delete)
{
}

WasmEngineIImpl::~WasmEngineIImpl()
{
    wasmi_exporttype_vec_delete(&export_types);
    wasmi_extern_vec_delete(&exports);
}

bool
WasmEngineIImpl::makeModule(
    vbytes const& wasmCode,
    wasm_extern_vec_t const& imports)
{
    wasm_byte_vec_t const code{wasmCode.size(), (char*)(wasmCode.data())};

    module = decltype(module)(
        wasmi_module_new(store.get(), &code), &wasmi_module_delete);
    if (!module)
        throw std::runtime_error("WasmEngineIImpl: can't create module");

    mod_inst = decltype(mod_inst)(
        wasmi_instance_new(store.get(), module.get(), &imports, &trap),
        &wasmi_instance_delete);
    if (!mod_inst || trap)
        throw std::runtime_error("WasmEngineIImpl: can't create instance");

    wasmi_module_exports(module.get(), &export_types);
    wasmi_instance_exports(mod_inst.get(), &exports);

    return false;  // to be compatible with other VMs
}

wasm_func_t*
WasmEngineIImpl::getFunc(std::string_view funcName)
{
    wasm_func_t* f = nullptr;

    if (!export_types.size)
        throw std::runtime_error("WasmEngineIImpl: no export");
    if (export_types.size != exports.size)
        throw std::runtime_error("WasmEngineIImpl: invalid export");

    for (unsigned i = 0; i < export_types.size; ++i)
    {
        auto* exp_type(export_types.data[i]);

        const wasm_externtype_t* exn_type = wasmi_exporttype_type(exp_type);
        if (wasmi_externtype_kind(exn_type) == WASM_EXTERN_FUNC)
        {
            wasm_name_t const* name = wasmi_exporttype_name(exp_type);
            if (funcName == std::string_view(name->data, name->size))
            {
                auto* exn(exports.data[i]);
                if (wasmi_extern_kind(exn) != WASM_EXTERN_FUNC)
                    throw std::runtime_error("WasmEngineIImpl: invalid export");

                f = wasmi_extern_as_func(exn);
                break;
            }
        }
    }

    if (!f)
        throw std::runtime_error("WasmEngineIImpl: can't find function");

    return f;
}

vmem
WasmEngineIImpl::getMem()
{
    wasm_memory_t* mem = nullptr;
    for (unsigned i = 0; i < exports.size; ++i)
    {
        auto* e(exports.data[i]);
        if (wasmi_extern_kind(e) == WASM_EXTERN_MEMORY)
        {
            mem = wasmi_extern_as_memory(e);
            break;
        }
    }

    if (!mem)
        throw std::runtime_error("WasmEngineIImpl: no memory exported");

    return {
        reinterpret_cast<std::uint8_t*>(wasmi_memory_data(mem)),
        wasmi_memory_data_size(mem)};
}

void
WasmEngineIImpl::add_param(std::vector<wasm_val_t>& in, int32_t p)
{
    in.emplace_back();
    auto& el(in.back());
    memset(&el, 0, sizeof(el));
    el = WASM_I32_VAL(p);  // WASM_I32;
}

void
WasmEngineIImpl::add_param(std::vector<wasm_val_t>& in, int64_t p)
{
    in.emplace_back();
    auto& el(in.back());
    el = WASM_I64_VAL(p);
}

template <int NR, class... Types>
inline wasm_val_vec_t
WasmEngineIImpl::call(std::string_view func, Types... args)
{
    // Lookup our export function
    auto* f = getFunc(func);
    if (!f)
        throw std::runtime_error(std::string("Can't find ") + func.data());

    return call<NR>(f, std::forward<Types>(args)...);
}

template <int NR, class... Types>
wasm_val_vec_t
WasmEngineIImpl::call(wasm_func_t* func, Types... args)
{
    std::vector<wasm_val_t> in;
    return call<NR>(func, in, std::forward<Types>(args)...);
}

template <int NR, class... Types>
wasm_val_vec_t
WasmEngineIImpl::call(wasm_func_t* func, std::vector<wasm_val_t>& in)
{
    wasm_val_vec_t ret{0, nullptr};
    if (NR)
        wasmi_val_vec_new_uninitialized(&ret, NR);

    wasm_val_vec_t const inv{in.size(), in.data()};
    trap = wasmi_func_call(func, &inv, &ret);
    if (trap)
        print_wasmi_error("failed to call func", trap);

    // assert(results[0].kind == WASM_I32);
    // if (NR) printf("Result P5: %d\n", ret[0].of.i32);

    return ret;
}

template <int NR, class... Types>
wasm_val_vec_t
WasmEngineIImpl::call(
    wasm_func_t* func,
    std::vector<wasm_val_t>& in,
    std::int32_t p,
    Types... args)
{
    add_param(in, p);
    return call<NR>(func, in, std::forward<Types>(args)...);
}

template <int NR, class... Types>
wasm_val_vec_t
WasmEngineIImpl::call(
    wasm_func_t* func,
    std::vector<wasm_val_t>& in,
    std::int64_t p,
    Types... args)
{
    add_param(in, p);
    return call<NR>(func, in, std::forward<Types>(args)...);
}

template <int NR, class... Types>
wasm_val_vec_t
WasmEngineIImpl::call(
    wasm_func_t* func,
    std::vector<wasm_val_t>& in,
    uint8_t const* m,
    std::size_t sz,
    Types... args)
{
    auto const res = call<1>(V_ALLOC, static_cast<int32_t>(sz));
    if (trap || (res.data[0].kind != WASM_I32))
        return {0, nullptr};
    auto const ptr = res.data[0].of.i32;

    auto mem = getMem();
    memcpy(mem.p + ptr, m, sz);

    add_param(in, ptr);
    add_param(in, static_cast<int32_t>(sz));
    return call<NR>(func, in, std::forward<Types>(args)...);
}

template <int NR, class... Types>
wasm_val_vec_t
WasmEngineIImpl::call(
    wasm_func_t* func,
    std::vector<wasm_val_t>& in,
    vbytes const& p,
    Types... args)
{
    return call<NR>(func, in, p.data(), p.size(), std::forward<Types>(args)...);
}

Expected<bool, TER>
WasmEngineIImpl::run(
    vbytes const& wasmCode,
    std::string_view funcName,
    int32_t input)
{
    // Create and instantiate the module.
    if (makeModule(wasmCode))
        return Unexpected<TER>(tecFAILED_PROCESSING);

    // Call it!
    auto res = call<1>(funcName, input);
    uvec del_res(&res, &wasmi_val_vec_delete);
    if (!res.size || trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    assert(res.data[0].kind == WASM_I32);
    // printf("Result: %d\n", results[0].of.i32);
    return res.data[0].of.i32 != 0;
}

Expected<bool, TER>
WasmEngineIImpl::run(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& accountID)
{
    // Create and instantiate the module.
    if (makeModule(wasmCode))
        return Unexpected<TER>(tecFAILED_PROCESSING);

    auto res = call<1>(funcName, accountID);
    uvec del_res(&res, &wasmi_val_vec_delete);
    if (!res.size || trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    assert(res.data[0].kind == WASM_I32);
    // printf("Result: %d\n", results[0].of.i32);
    return res.data[0].of.i32 == 1;
}

Expected<bool, TER>
WasmEngineIImpl::run(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& escrow_tx_json_data,
    vbytes const& escrow_lo_json_data)
{
    // Create and instantiate the module.
    if (makeModule(wasmCode))
        return Unexpected<TER>(tecFAILED_PROCESSING);

    auto res = call<1>(funcName, escrow_tx_json_data, escrow_lo_json_data);
    uvec del_res(&res, &wasmi_val_vec_delete);
    if (!res.size || trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    assert(res.data[0].kind == WASM_I32);
    // printf("Result: %d\n", results[0].of.i32);
    return res.data[0].of.i32 == 1;
}

Expected<std::pair<bool, std::string>, TER>
WasmEngineIImpl::runP4(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& escrow_tx_json_data,
    vbytes const& escrow_lo_json_data)
{
    // Create and instantiate the module.
    if (makeModule(wasmCode))
        return Unexpected<TER>(tecFAILED_PROCESSING);

    auto res = call<1>(funcName, escrow_tx_json_data, escrow_lo_json_data);
    uvec del_res(&res, &wasmi_val_vec_delete);
    if (!res.size || trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    assert(res.data[0].kind == WASM_I32);
    // printf("Result: %d\n", results[0].of.i32);
    // return res.data[0].of.i32 == 1;
    auto const ptr = res.data[0].of.i32;
    std::uint8_t buf[16];
    memset(buf, 0, sizeof(buf));

    auto const mem = getMem();
    memcpy(buf, mem.p + ptr, 9);

    auto const flag = buf[0];
    auto const ret_pointer = *reinterpret_cast<int32_t const*>(buf + 1);
    auto const ret_len = *reinterpret_cast<int32_t const*>(buf + 5);
    // printf("re flag %d, ptr %d, len %d\n", flag, ret_pointer, ret_len);

    vbytes buf2(ret_len);
    memcpy(buf2.data(), mem.p + ret_pointer, ret_len);

    std::string newData(buf2.begin(), buf2.end());

    call<0>(V_DEALLOC, ret_pointer, ret_len);
    if (trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);
    call<0>(V_DEALLOC, ptr, 9);
    if (trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    return std::pair<bool, std::string>(flag == 1, newData);
}

Expected<bool, TER>
WasmEngineIImpl::run(
    vbytes const& wasmCode,
    std::string_view funcName,
    LedgerDataProvider* ledgerDataProvider)
{
    std::unique_ptr<wasm_valtype_t, decltype(&wasmi_valtype_delete)> vtype(
        wasmi_valtype_new_i32(), &wasmi_valtype_delete);
    std::unique_ptr<wasm_functype_t, decltype(&wasmi_functype_delete)> ftype(
        wasmi_functype_new_0_1(vtype.get()), &wasmi_functype_delete);

    // std::unique_ptr<wasm_func_t, decltype(&wasmi_func_delete)> func(
    //     wasmi_func_new_with_env(store.get(),ftype.get(),
    //     &get_ledger_sqn_WTime, ledgerDataProvider, nullptr),
    //     &wasmi_func_delete);

    wasm_func_t* func = wasmi_func_new_with_env(
        store.get(),
        ftype.get(),
        &get_ledger_sqn_WTime,
        ledgerDataProvider,
        nullptr);

    wasm_extern_t* arr[] = {wasmi_func_as_extern(func)};
    wasm_extern_vec_t imports = WASM_ARRAY_VEC(arr);
    if (makeModule(wasmCode, {imports}))
        return Unexpected<TER>(tecFAILED_PROCESSING);

    auto res = call<1>(funcName);
    uvec del_res(&res, &wasmi_val_vec_delete);
    if (!res.size || trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    return res.data[0].kind == WASM_I32 && res.data[0].of.i32;
}

//////////////////////////////////////////////////////////////////////////////////////////

WasmEngineI::WasmEngineI() : impl(std::make_unique<WasmEngineIImpl>())
{
}

WasmEngineI::~WasmEngineI() = default;

Expected<bool, TER>
WasmEngineI::run(
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
WasmEngineI::run(
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
WasmEngineI::run(
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
WasmEngineI::runP4(
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
WasmEngineI::run(
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
