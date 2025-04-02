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

#include <wamr_so.h>

#include <memory>

namespace ripple {

//////////////////////////////////////////////////////////////////////////////////////////

namespace {
static wasm_trap_t*
get_ledger_sqn(void* env, const wasm_val_vec_t*, wasm_val_vec_t* results)
{
    auto sqn = reinterpret_cast<LedgerDataProvider*>(env)->get_ledger_sqn();
    if (results->size)
    {
        results->data[0] = WASM_I32_VAL(sqn);
    }

    return nullptr;
}

static void
print_wasm_error(const char* message, wasm_trap_t* trap)
{
    fprintf(stderr, "error: %s\n", message);
    wasm_byte_vec_t error_message;

    if (trap)
    {
        wamr_trap_message(trap, &error_message);
        wamr_trap_delete(trap);
    }
    fprintf(stderr, "%.*s\n", (int)error_message.size, error_message.data);
    wamr_byte_vec_delete(&error_message);
}

// clang-format off
struct wsm_res
{
    wasm_val_vec_t r;
    wsm_res(unsigned N = 0):r{0, nullptr, 0, 0, nullptr} {if (N) wamr_val_vec_new_uninitialized(&r, N);}
    ~wsm_res() { if (r.size) wamr_val_vec_delete(&r); }
    wsm_res(wsm_res const &) = delete;
    wsm_res& operator=(wsm_res const &) = delete;

    wsm_res(wsm_res &&o) {*this = std::move(o);}
    wsm_res& operator=(wsm_res  &&o){r = o.r; o.r = {0, nullptr, 0, 0, nullptr}; return *this;}
    //operator wasm_val_vec_t &() {return r;}
};
// clang-format on

using module_t = std::unique_ptr<wasm_module_t, decltype(&wamr_module_delete)>;
using mod_inst_t =
    std::unique_ptr<wasm_instance_t, decltype(&wamr_instance_delete)>;

struct my_mod_inst_t
{
    wasm_extern_vec_t exports;
    mod_inst_t mod_inst;

private:
    static mod_inst_t
    init(
        wasm_store_t* s,
        wasm_module_t* m,
        wasm_extern_vec_t* expt,
        wasm_extern_vec_t const& imports = WASM_EMPTY_VEC)
    {
        wasm_trap_t* trap = nullptr;
        mod_inst_t mi = mod_inst_t(
            wamr_instance_new(s, m, &imports, &trap), &wamr_instance_delete);
        if (!mi || trap)
        {
            print_wasm_error("can't create instance", trap);
            throw std::runtime_error(
                std::string(engineName(wasmEngines::Wamr)) +
                ": can't create instance");
        }
        wamr_instance_exports(mi.get(), expt);
        return mi;
    }

public:
    my_mod_inst_t()
        : exports{0, nullptr, 0, 0, nullptr}
        , mod_inst(nullptr, &wamr_instance_delete)
    {
    }

    my_mod_inst_t(my_mod_inst_t&& o)
        : exports{0, nullptr, 0, 0, nullptr}
        , mod_inst(nullptr, &wamr_instance_delete)
    {
        *this = std::move(o);
    }

    my_mod_inst_t&
    operator=(my_mod_inst_t&& o)
    {
        if (this == &o)
            return *this;

        if (exports.size)
            wamr_extern_vec_delete(&exports);
        exports = o.exports;
        o.exports = {0, nullptr, 0, 0, nullptr};

        mod_inst = std::move(o.mod_inst);

        return *this;
    }

    my_mod_inst_t(
        wasm_store_t* s,
        wasm_module_t* m,
        wasm_extern_vec_t const& imports = WASM_EMPTY_VEC)
        : exports{0, nullptr, 0, 0, nullptr}
        , mod_inst(init(s, m, &exports, imports))
    {
    }

    ~my_mod_inst_t()
    {
        if (exports.size)
            wamr_extern_vec_delete(&exports);
    }

    operator bool() const
    {
        return static_cast<bool>(mod_inst);
    }

    wasm_func_t*
    getFunc(
        std::string_view funcName,
        wasm_exporttype_vec_t const& export_types) const
    {
        wasm_func_t* f = nullptr;

        if (!export_types.size)
            throw std::runtime_error(
                std::string(engineName(wasmEngines::Wamr)) + ": no export");
        if (export_types.size != exports.size)
            throw std::runtime_error(
                std::string(engineName(wasmEngines::Wamr)) +
                ": invalid export");

        for (unsigned i = 0; i < export_types.size; ++i)
        {
            auto const* exp_type(export_types.data[i]);

            wasm_name_t const* name = wamr_exporttype_name(exp_type);
            const wasm_externtype_t* exn_type = wamr_exporttype_type(exp_type);
            if (wamr_externtype_kind(exn_type) == WASM_EXTERN_FUNC)
            {
                if (funcName == std::string_view(name->data, name->size - 1))
                {
                    auto* exn(exports.data[i]);
                    if (wamr_extern_kind(exn) != WASM_EXTERN_FUNC)
                        throw std::runtime_error(
                            std::string(engineName(wasmEngines::Wamr)) +
                            ": invalid export");

                    f = wamr_extern_as_func(exn);
                    break;
                }
            }
        }

        if (!f)
            throw std::runtime_error(
                std::string(engineName(wasmEngines::Wamr)) +
                ": can't find function " + std::string(funcName));

        return f;
    }

    vmem
    getMem() const
    {
        wasm_memory_t* mem = nullptr;
        for (unsigned i = 0; i < exports.size; ++i)
        {
            auto* e(exports.data[i]);
            if (wamr_extern_kind(e) == WASM_EXTERN_MEMORY)
            {
                mem = wamr_extern_as_memory(e);
                break;
            }
        }

        if (!mem)
            throw std::runtime_error(
                std::string(engineName(wasmEngines::Wamr)) +
                ": no memory exported");

        return {
            reinterpret_cast<std::uint8_t*>(wamr_memory_data(mem)),
            wamr_memory_data_size(mem)};
    }
};

struct my_module_t
{
    module_t module;
    std::vector<my_mod_inst_t> mod_inst;
    wasm_exporttype_vec_t export_types;

private:
    static module_t
    init(wasm_store_t* s, vbytes const& wasmBin)
    {
        wasm_byte_vec_t const code{
            wasmBin.size(),
            (char*)(wasmBin.data()),
            wasmBin.size(),
            sizeof(std::remove_reference_t<decltype(wasmBin)>::value_type),
            nullptr};
        module_t m = module_t(wamr_module_new(s, &code), &wamr_module_delete);
        return m;
    }

public:
    my_module_t()
        : module(nullptr, &wamr_module_delete)
        , export_types{0, nullptr, 0, 0, nullptr}
    {
    }

    my_module_t(my_module_t&& o)
        : module(nullptr, &wamr_module_delete)
        , export_types{0, nullptr, 0, 0, nullptr}
    {
        *this = std::move(o);
    }

    my_module_t&
    operator=(my_module_t&& o)
    {
        if (this == &o)
            return *this;

        module = std::move(o.module);
        mod_inst = std::move(o.mod_inst);
        if (export_types.size)
            wamr_exporttype_vec_delete(&export_types);
        export_types = o.export_types;
        o.export_types = {0, nullptr, 0, 0, nullptr};
        return *this;
    }

    my_module_t(
        wasm_store_t* s,
        vbytes const& wasmBin,
        bool instantiate,
        wasm_extern_vec_t const& imports = WASM_EMPTY_VEC)
        : module(init(s, wasmBin)), export_types{0, nullptr, 0, 0, nullptr}
    {
        if (!module)
            throw std::runtime_error(
                std::string(engineName(wasmEngines::Wamr)) +
                " + can't create module");

        wamr_module_exports(module.get(), &export_types);
        if (instantiate)
            mod_inst.emplace_back(s, module.get(), imports);
    }

    ~my_module_t()
    {
        if (export_types.size)
            wamr_exporttype_vec_delete(&export_types);
    }

    wasm_func_t*
    getFunc(std::string_view funcName, int i) const
    {
        return mod_inst[i].getFunc(funcName, export_types);
    }

    vmem
    getMem(int i) const
    {
        return mod_inst[i].getMem();
    }

    int
    addInstance(
        wasm_store_t* s,
        wasm_extern_vec_t const& imports = WASM_EMPTY_VEC)
    {
        for (int i = 0, e = mod_inst.size(); i < e; ++i)
        {
            auto& ins(mod_inst[i]);
            if (!ins)
            {
                ins = {s, module.get(), imports};
                return i;
            }
        }
        mod_inst.emplace_back(s, module.get(), imports);
        return static_cast<int>(mod_inst.size()) - 1;
    }

    int
    delInstance(int i)
    {
        if (i >= mod_inst.size())
            return -1;
        if (!mod_inst[i])
            mod_inst[i] = my_mod_inst_t();
        return i;
    }
};

// struct scoped_instance
// {
//     scoped_instance()
// };

}  // namespace

class WamrEngineImpl
{
    std::unique_ptr<wasm_engine_t, decltype(&wamr_engine_delete)> engine;
    std::unique_ptr<wasm_store_t, decltype(&wamr_store_delete)> store;
    std::vector<my_module_t> modules;
    wasm_trap_t* trap = nullptr;

public:
    WamrEngineImpl();
    ~WamrEngineImpl() = default;

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

    Expected<std::pair<bool, std::string>, TER>
    justRunP4(
        std::string_view funcName,
        vbytes const& escrow_tx_json_data,
        vbytes const& escrow_lo_json_data,
        int m,
        int i);

    Expected<bool, TER>
    run(vbytes const& wasmCode,
        std::string_view funcName,
        LedgerDataProvider* ledgerDataProvider);

    Expected<int, TER>
    preRun(vbytes const& wasmCode, LedgerDataProvider* ledgerDataProvider);

    Expected<bool, TER>
    justRun(
        std::string_view funcName,
        LedgerDataProvider* ledgerDataProvider,
        int m,
        int i);

    Expected<int, TER>
    justRun(std::string_view funcName, int m, int i);

    int
    addModule(vbytes const& wasmCode, bool instantiate);
    void
    clearModules()
    {
        modules.clear();
        store.reset();  // to free the memory before creating new store
        store = {wamr_store_new(engine.get()), &wamr_store_delete};
    }
    int
    addInstance(int m);

    int32_t
    runFunc(std::string_view const funcName, int32_t p, int m, int i);

    int64_t
    runFunc64(std::string_view const funcName, int64_t p, int m, int i);

    std::vector<uint64_t>
    runSha(std::string_view const data, int m, int i);

protected:
    int
    makeModule(
        vbytes const& wasmCode,
        wasm_extern_vec_t const& imports = WASM_EMPTY_VEC);

    wasm_func_t*
    getFunc(std::string_view funcName, int m, int i);

    vmem
    getMem(int m, int i);

    void
    add_param(std::vector<wasm_val_t>& in, int32_t p);
    void
    add_param(std::vector<wasm_val_t>& in, int64_t p);

    template <int NR, class... Types>
    inline wsm_res
    call(std::string_view func, int m, int i, Types... args);

    template <int NR, class... Types>
    inline wsm_res
    call(wasm_func_t* func, int m, int i, Types... args);

    template <int NR, class... Types>
    inline wsm_res
    call(wasm_func_t* f, int m, int i, std::vector<wasm_val_t>& in);

    template <int NR, class... Types>
    inline wsm_res
    call(
        wasm_func_t* func,
        int m,
        int i,
        std::vector<wasm_val_t>& in,
        std::int32_t p,
        Types... args);

    template <int NR, class... Types>
    inline wsm_res
    call(
        wasm_func_t* func,
        int m,
        int i,
        std::vector<wasm_val_t>& in,
        std::int64_t p,
        Types... args);

    template <int NR, class... Types>
    inline wsm_res
    call(
        wasm_func_t* func,
        int m,
        int i,
        std::vector<wasm_val_t>& in,
        uint8_t const* d,
        std::size_t sz,
        Types... args);

    template <int NR, class... Types>
    inline wsm_res
    call(
        wasm_func_t* func,
        int m,
        int i,
        std::vector<wasm_val_t>& in,
        vbytes const& p,
        Types... args);
};

WamrEngineImpl::WamrEngineImpl()
    : engine(wamr_engine_new(), &wamr_engine_delete)
    , store(wamr_store_new(engine.get()), &wamr_store_delete)
{
    // wamr_runtime_set_default_running_mode(Mode_LLVM_JIT);
    wamr_runtime_set_log_level(WASM_LOG_LEVEL_FATAL);
}

int
WamrEngineImpl::makeModule(
    vbytes const& wasmCode,
    wasm_extern_vec_t const& imports)
{
    modules.emplace_back(store.get(), wasmCode, true, imports);
    return static_cast<int>(modules.size()) - 1;
}

int
WamrEngineImpl::addModule(vbytes const& wasmCode, bool instantiate)
{
    modules.emplace_back(store.get(), wasmCode, instantiate);
    return static_cast<int>(modules.size()) - 1;
}

int
WamrEngineImpl::addInstance(int m)
{
    return modules[m].addInstance(store.get());
}

wasm_func_t*
WamrEngineImpl::getFunc(std::string_view funcName, int m, int i)
{
    return modules[m].getFunc(funcName, i);
}

vmem
WamrEngineImpl::getMem(int m, int i)
{
    return modules[m].getMem(i);
}

void
WamrEngineImpl::add_param(std::vector<wasm_val_t>& in, int32_t p)
{
    in.emplace_back();
    auto& el(in.back());
    memset(&el, 0, sizeof(el));
    el = WASM_I32_VAL(p);  // WASM_I32;
}

void
WamrEngineImpl::add_param(std::vector<wasm_val_t>& in, int64_t p)
{
    in.emplace_back();
    auto& el(in.back());
    el = WASM_I64_VAL(p);
}

template <int NR, class... Types>
wsm_res
WamrEngineImpl::call(std::string_view func, int m, int i, Types... args)
{
    // Lookup our export function
    auto* f = getFunc(func, m, i);
    return call<NR>(f, m, i, std::forward<Types>(args)...);
}

template <int NR, class... Types>
wsm_res
WamrEngineImpl::call(wasm_func_t* func, int m, int i, Types... args)
{
    std::vector<wasm_val_t> in;
    return call<NR>(func, m, i, in, std::forward<Types>(args)...);
}

template <int NR, class... Types>
wsm_res
WamrEngineImpl::call(
    wasm_func_t* func,
    int m,
    int i,
    std::vector<wasm_val_t>& in)
{
    // wasm_val_t rs[1] = {WASM_I32_VAL(0)};
    wsm_res ret(NR);
    // if (NR)  {   wamr_val_vec_new_uninitialized(&ret, NR);    //
    // wamr_val_vec_new(&ret, NR, &rs[0]);    // ret = WASM_ARRAY_VEC(rs);    }

    wasm_val_vec_t const inv{
        in.size(),
        in.data(),
        in.size(),
        sizeof(std::remove_reference_t<decltype(in)>::value_type),
        nullptr};
    trap = wamr_func_call(func, &inv, &ret.r);
    if (trap)
        print_wasm_error("failed to call func", trap);

    // assert(results[0].kind == WASM_I32);
    // if (NR) printf("Result P5: %d\n", ret[0].of.i32);

    return ret;
}

template <int NR, class... Types>
wsm_res
WamrEngineImpl::call(
    wasm_func_t* func,
    int m,
    int i,
    std::vector<wasm_val_t>& in,
    std::int32_t p,
    Types... args)
{
    add_param(in, p);
    return call<NR>(func, m, i, in, std::forward<Types>(args)...);
}

template <int NR, class... Types>
wsm_res
WamrEngineImpl::call(
    wasm_func_t* func,
    int m,
    int i,
    std::vector<wasm_val_t>& in,
    std::int64_t p,
    Types... args)
{
    add_param(in, p);
    return call<NR>(func, m, i, in, std::forward<Types>(args)...);
}

template <int NR, class... Types>
wsm_res
WamrEngineImpl::call(
    wasm_func_t* func,
    int m,
    int i,
    std::vector<wasm_val_t>& in,
    uint8_t const* d,
    std::size_t sz,
    Types... args)
{
    auto res = call<1>(V_ALLOC, m, i, static_cast<int32_t>(sz));

    if (trap || (res.r.data[0].kind != WASM_I32))
        return {};
    auto const ptr = res.r.data[0].of.i32;

    auto mem = getMem(m, i);
    memcpy(mem.p + ptr, d, sz);

    add_param(in, ptr);
    add_param(in, static_cast<int32_t>(sz));
    return call<NR>(func, m, i, in, std::forward<Types>(args)...);
}

template <int NR, class... Types>
wsm_res
WamrEngineImpl::call(
    wasm_func_t* func,
    int m,
    int i,
    std::vector<wasm_val_t>& in,
    vbytes const& p,
    Types... args)
{
    return call<NR>(
        func, m, i, in, p.data(), p.size(), std::forward<Types>(args)...);
}

Expected<bool, TER>
WamrEngineImpl::run(
    vbytes const& wasmCode,
    std::string_view funcName,
    int32_t input)
{
    // Create and instantiate the module.
    int const m = makeModule(wasmCode);
    int const i = 0;
    if (m < 0)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    // Call it!
    auto* f = getFunc(funcName, m, i);
    auto res = call<1>(f, m, i, input);
    if (!res.r.size || trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    assert(res.r.data[0].kind == WASM_I32);
    // printf("Result: %d\n", results[0].of.i32);
    return res.r.data[0].of.i32 != 0;
}

Expected<bool, TER>
WamrEngineImpl::run(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& accountID)
{
    // Create and instantiate the module.
    int const m = makeModule(wasmCode);
    int const i = 0;
    if (m < 0)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    auto* f = getFunc(funcName, m, i);
    auto res = call<1>(f, m, i, accountID);
    if (!res.r.size || trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    assert(res.r.data[0].kind == WASM_I32);
    // printf("Result: %d\n", results[0].of.i32);
    return res.r.data[0].of.i32 == 1;
}

Expected<bool, TER>
WamrEngineImpl::run(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& escrow_tx_json_data,
    vbytes const& escrow_lo_json_data)
{
    // Create and instantiate the module.
    int const m = makeModule(wasmCode);
    int const i = 0;
    if (m < 0)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    auto* f = getFunc(funcName, m, i);
    auto res = call<1>(f, m, i, escrow_tx_json_data, escrow_lo_json_data);
    if (!res.r.size || trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    assert(res.r.data[0].kind == WASM_I32);
    // printf("Result: %d\n", results[0].of.i32);
    return res.r.data[0].of.i32 == 1;
}

Expected<std::pair<bool, std::string>, TER>
WamrEngineImpl::runP4(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& escrow_tx_json_data,
    vbytes const& escrow_lo_json_data)
{
    // Create and instantiate the module.
    int const m = makeModule(wasmCode);
    int const i = 0;
    if (m < 0)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    return justRunP4(funcName, escrow_tx_json_data, escrow_lo_json_data, m, i);
}

Expected<std::pair<bool, std::string>, TER>
WamrEngineImpl::justRunP4(
    std::string_view funcName,
    vbytes const& escrow_tx_json_data,
    vbytes const& escrow_lo_json_data,
    int m,
    int i)
{
    auto* f = getFunc(funcName, m, i);
    auto res = call<1>(f, m, i, escrow_tx_json_data, escrow_lo_json_data);
    if (!res.r.size || trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    assert(res.r.data[0].kind == WASM_I32);
    // printf("Result: %d\n", results[0].of.i32);
    // return res.data[0].of.i32 == 1;
    auto const ptr = res.r.data[0].of.i32;
    std::uint8_t buf[16];
    memset(buf, 0, sizeof(buf));

    auto const mem = getMem(m, i);
    memcpy(buf, mem.p + ptr, 9);

    auto const flag = buf[0];
    auto const ret_pointer = *reinterpret_cast<int32_t const*>(buf + 1);
    auto const ret_len = *reinterpret_cast<int32_t const*>(buf + 5);
    // printf("re flag %d, ptr %d, len %d\n", flag, ret_pointer, ret_len);

    vbytes buf2(ret_len);
    memcpy(buf2.data(), mem.p + ret_pointer, ret_len);

    std::string newData(buf2.begin(), buf2.end());

    call<0>(V_DEALLOC, m, i, ret_pointer, ret_len);
    if (trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    call<0>(V_DEALLOC, m, i, ptr, 9);
    if (trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    return std::pair<bool, std::string>(flag == 1, newData);
}

Expected<bool, TER>
WamrEngineImpl::run(
    vbytes const& wasmCode,
    std::string_view funcName,
    LedgerDataProvider* ledgerDataProvider)
{
    wasm_valtype_t* vtype(wamr_valtype_new_i32());
    std::unique_ptr<wasm_functype_t, decltype(&wamr_functype_delete)> ftype(
        wamr_functype_new_0_1(vtype), &wamr_functype_delete);

    wasm_func_t* func = wamr_func_new_with_env(
        store.get(), ftype.get(), &get_ledger_sqn, ledgerDataProvider, nullptr);

    wasm_extern_t* arr[] = {wamr_func_as_extern(func)};
    wasm_extern_vec_t imports = WASM_ARRAY_VEC(arr);
    int const m = makeModule(wasmCode, {imports});
    int const i = 0;
    if (m < 0)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    return justRun(funcName, ledgerDataProvider, m, i);
}

Expected<int, TER>
WamrEngineImpl::preRun(
    vbytes const& wasmCode,
    LedgerDataProvider* ledgerDataProvider)
{
    wasm_valtype_t* vtype(wamr_valtype_new_i32());
    std::unique_ptr<wasm_functype_t, decltype(&wamr_functype_delete)> ftype(
        wamr_functype_new_0_1(vtype), &wamr_functype_delete);

    wasm_func_t* func = wamr_func_new_with_env(
        store.get(), ftype.get(), &get_ledger_sqn, ledgerDataProvider, nullptr);

    wasm_extern_t* arr[] = {wamr_func_as_extern(func)};
    wasm_extern_vec_t imports = WASM_ARRAY_VEC(arr);
    int const m = makeModule(wasmCode, {imports});
    if (m < 0)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    return m;
}

Expected<bool, TER>
WamrEngineImpl::justRun(
    std::string_view funcName,
    LedgerDataProvider* ledgerDataProvider,
    int m,
    int i)
{
    auto* f = getFunc(funcName, m, i);
    auto res = call<1>(f, m, i);
    if (!res.r.size || res.r.data[0].kind != WASM_I32 || trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    return res.r.data[0].of.i32;
}

Expected<int, TER>
WamrEngineImpl::justRun(std::string_view funcName, int m, int i)
{
    auto* f = getFunc(funcName, m, i);
    auto res = call<1>(f, m, i);
    if (!res.r.size || trap || res.r.data[0].kind != WASM_I32)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    return res.r.data[0].of.i32;
}

int32_t
WamrEngineImpl::runFunc(
    std::string_view const funcName,
    int32_t p,
    int m,
    int i)
{
    auto* f = getFunc(funcName, m, i);
    auto res = call<1>(f, m, i, p);
    if (!res.r.size || trap)
        return -1;

    return res.r.data[0].kind == WASM_I32 ? res.r.data[0].of.i32 : -1;
}

int64_t
WamrEngineImpl::runFunc64(
    std::string_view const funcName,
    int64_t p,
    int m,
    int i)
{
    auto* f = getFunc(funcName, m, i);
    auto res = call<1>(f, m, i, p);
    if (!res.r.size || trap)
        return -1;

    return res.r.data[0].kind == WASM_I64 ? res.r.data[0].of.i64 : -1;
}

std::vector<uint64_t>
WamrEngineImpl::runSha(std::string_view const data, int m, int i)
{
    std::string_view funcName = "sha512_process";
    auto* f = getFunc(funcName, m, i);
    auto res = call<1>(
        f, m, i, reinterpret_cast<uint8_t const*>(data.data()), data.size());
    if (!res.r.size || trap)
        return {};

    auto const ptr = res.r.data[0].of.i32;
    std::uint64_t buf[8];
    memset(buf, 0, sizeof(buf));

    auto const mem = getMem(m, i);
    memcpy(buf, mem.p + ptr, 8 * sizeof(std::uint64_t));
    return {&buf[0], &buf[8]};
}

//////////////////////////////////////////////////////////////////////////////////////////

WamrEngine::WamrEngine()
    : WasmEngine({1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})
    , impl(std::make_unique<WamrEngineImpl>())
{
}

WamrEngine::~WamrEngine() = default;

Expected<bool, TER>
WamrEngine::run(
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
WamrEngine::run(
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
WamrEngine::run(
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
WamrEngine::runP4(
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
    catch (...)
    {
    }
    return Unexpected<TER>(tecFAILED_PROCESSING);
}

Expected<std::pair<bool, std::string>, TER>
WamrEngine::justRunP4(
    std::string_view funcName,
    vbytes const& escrow_tx_json_data,
    vbytes const& escrow_lo_json_data,
    int m,
    int i)
{
    try
    {
        return impl->justRunP4(
            funcName, escrow_tx_json_data, escrow_lo_json_data, m, i);
    }
    catch (std::exception const&)
    {
    }
    return Unexpected<TER>(tecFAILED_PROCESSING);
}

Expected<bool, TER>
WamrEngine::run(
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

Expected<int, TER>
WamrEngine::preRun(
    vbytes const& wasmCode,
    LedgerDataProvider* ledgerDataProvider)
{
    try
    {
        return impl->preRun(wasmCode, ledgerDataProvider);
    }
    catch (std::exception const&)
    {
    }
    return Unexpected<TER>(tecFAILED_PROCESSING);
}

Expected<bool, TER>
WamrEngine::justRun(
    std::string_view funcName,
    LedgerDataProvider* ledgerDataProvider,
    int m,
    int i)
{
    try
    {
        return impl->justRun(funcName, ledgerDataProvider, m, i);
    }
    catch (std::exception const&)
    {
    }
    return Unexpected<TER>(tecFAILED_PROCESSING);
}

Expected<int, TER>
WamrEngine::justRun(std::string_view funcName, int m, int i)
{
    try
    {
        return impl->justRun(funcName, m, i);
    }
    catch (std::exception const&)
    {
    }
    return Unexpected<TER>(tecFAILED_PROCESSING);
}

int
WamrEngine::addModule(vbytes const& wasmCode, bool instantiate)
{
    try
    {
        return impl->addModule(wasmCode, instantiate);
    }
    catch (std::exception const& e)
    {
        std::cerr << engineName(wasmEngines::Wamr) << ": " << e.what()
                  << std::endl;
    }
    return -1;
}

void
WamrEngine::clearModules()
{
    return impl->clearModules();
}

int
WamrEngine::addInstance(int m)
{
    try
    {
        return impl->addInstance(m);
    }
    catch (std::exception const& e)
    {
        std::cerr << engineName(wasmEngines::Wamr) << ": " << e.what()
                  << std::endl;
    }
    return -1;
}

int32_t
WamrEngine::runFunc(std::string_view const funcName, int32_t p, int m, int i)
{
    return impl->runFunc(funcName, p, m, i);
}

int64_t
WamrEngine::runFunc64(std::string_view const funcName, int64_t p, int m, int i)
{
    return impl->runFunc64(funcName, p, m, i);
}

std::vector<uint64_t>
WamrEngine::runSha(std::string_view const data, int m, int i)
{
    return impl->runSha(data, m, i);
}

}  // namespace ripple
