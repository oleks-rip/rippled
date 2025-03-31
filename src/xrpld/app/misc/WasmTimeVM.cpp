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

#include <wasmtime_so.h>

#include <memory>

namespace ripple {

namespace {

static wasm_trap_t*
get_ledger_sqn(
    void* env,
    wasmtime_caller_t* caller,
    const wasmtime_val_t* args,
    size_t nargs,
    wasmtime_val_t* results,
    size_t nresults)
{
    auto sqn = reinterpret_cast<LedgerDataProvider*>(env)->get_ledger_sqn();
    if (nresults)
    {
        results[0] = wasmtime_val_t WASM_I32_VAL(sqn);
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
        wasmtime2_trap_message(trap, &error_message);
        wasmtime2_trap_delete(trap);
    }
    fprintf(stderr, "%.*s\n", (int)error_message.size, error_message.data);
    wasmtime2_byte_vec_delete(&error_message);
}

// clang-format off
struct wsm_res
{
    std::vector<wasmtime_val_t> r;

    wsm_res(unsigned N = 0) {if (N) r.resize(N); }
    ~wsm_res() { }
    wsm_res(wsm_res const &) = delete;
    wsm_res& operator=(wsm_res const &) = delete;

    wsm_res(wsm_res &&o) = default;
    wsm_res& operator=(wsm_res  &&o) = default;
    //operator wasm_val_vec_t &() {return r;}
};
// clang-format on

using module_t =
    std::unique_ptr<wasmtime_module_t, decltype(&wasmtime3_module_delete)>;
using mod_inst_t = wasmtime_instance_t;

static wasm_trap_t*
_proc_exit(
    void* env,
    wasmtime_caller_t* caller,
    const wasmtime_val_t* args,
    size_t nargs,
    wasmtime_val_t* results,
    size_t nresults)
{
    if (nargs)
        std::cout << "Exit called: " << std::to_string(args[0].of.i32)
                  << std::endl;
    return nullptr;
}

struct my_mod_inst_t
{
    wasmtime_context_t* context = nullptr;
    // std::vector<std::pair<std::string, wasmtime_extern_t>> exports;
    mod_inst_t mod_inst = {0, 0};

private:
    static void
    checkImport(
        std::vector<wasmtime_extern_t>& out,
        wasmtime_context_t* ct,
        wasmtime_module_t* m,
        std::vector<wasmtime_extern_t> const& in)
    {
        wasm_importtype_vec_t impts = {0, nullptr};
        wasmtime3_module_imports(m, &impts);

        for (int i = 0; i < impts.size; ++i)
        {
            auto const* impt(impts.data[i]);

            wasm_name_t const* name = wasmtime2_importtype_name(impt);
            wasm_externtype_t const* xtype = wasmtime2_importtype_type(impt);
            if ((wasmtime2_externtype_kind(xtype) == WASM_EXTERN_FUNC) &&
                (VW_PROC_EXIT == std::string_view(name->data, name->size)))
            {
                std::unique_ptr<
                    wasm_functype_t,
                    decltype(&wasmtime2_functype_delete)>
                    ftype(
                        wasmtime2_functype_new_1_0(wasmtime2_valtype_new_i32()),
                        &wasmtime2_functype_delete);

                wasmtime_func_t func;
                wasmtime3_func_new(
                    ct, ftype.get(), &_proc_exit, nullptr, nullptr, &func);

                out.push_back(
                    {.kind = WASMTIME_EXTERN_FUNC, .of = {.func = func}});

                break;
            }
        }

        for (auto const& imp : in)
            out.push_back(imp);
    }

    static mod_inst_t
    init(
        wasmtime_context_t* ct,
        wasmtime_module_t* m,
        // std::vector<std::pair<std::string, wasmtime_extern_t>>& expt,
        std::vector<wasmtime_extern_t> const& imports)
    {
        wasm_trap_t* trap = nullptr;

        ////////////////////////////////////////////////////////////////
        // check wasi

        std::vector<wasmtime_extern_t> imports2;
        checkImport(imports2, ct, m, imports);

        ////////////////////////////////////////////////////////////////

        mod_inst_t mi = {0, 0};
        auto* e = wasmtime3_instance_new(
            ct, m, imports2.data(), imports2.size(), &mi, &trap);
        if (e || trap)
        {
            print_wasm_error("can't create instance", trap);
            throw std::runtime_error(
                std::string(engineName(wasmEngines::Time)) +
                ": can't create instance");
        }

        // char* name;
        // std::size_t len;
        // wasmtime_extern_t item;
        // for (unsigned idx = 0;
        //      wasmtime3_instance_export_nth(ct, &mi, idx, &name, &len, &item);
        //      ++idx)
        //     expt.emplace_back(std::string(name, len), item);

        return mi;
    }

public:
    my_mod_inst_t() = default;
    my_mod_inst_t(my_mod_inst_t&& o) : context(nullptr), mod_inst{0, 0}
    {
        *this = std::move(o);
    }

    my_mod_inst_t&
    operator=(my_mod_inst_t&& o)
    {
        if (this == &o)
            return *this;

        // if (exports.size) wasmtime2_extern_vec_delete(&exports);
        // exports = std::move(o.exports);
        // o.exports = {0, nullptr};

        context = o.context;
        o.context = nullptr;
        mod_inst = o.mod_inst;
        o.mod_inst = {0, 0};

        return *this;
    }

    my_mod_inst_t(
        wasmtime_context_t* ct,
        wasmtime_module_t* m,
        std::vector<wasmtime_extern_t> const& imports)
        : context(ct), mod_inst(init(ct, m, imports))
    {
    }

    ~my_mod_inst_t()
    {
        // if (exports.size) wasmtime2_extern_vec_delete(&exports);
    }

    operator bool() const
    {
        return true;  // static_cast<bool>(mod_inst.store_id >= 0);
    }

    wasmtime_func_t
    getFunc(std::string_view funcName)
    {
        wasmtime_extern_t item;
        bool ok = wasmtime3_instance_export_get(
            context, &mod_inst, funcName.data(), funcName.size(), &item);

        // if (exports.empty())
        //     throw std::runtime_error(
        //         std::string(engineName(wasmEngines::Time)) + ": no export");

        // for (auto& [name, ext] : exports)
        // {
        //     if ((ext.kind == WASMTIME_EXTERN_FUNC) && (funcName == name))
        //     {
        //         f = &ext.of.func;
        //         break;
        //     }
        // }

        if (!(ok && item.kind == WASMTIME_EXTERN_FUNC))
            throw std::runtime_error(
                std::string(engineName(wasmEngines::Time)) +
                ": can't find function " + std::string(funcName));

        return item.of.func;
    }

    vmem
    getMem()
    {
        wasmtime_extern_t item;
        bool ok = wasmtime3_instance_export_get(
            context, &mod_inst, V_MEM.data(), V_MEM.size(), &item);

        // if (exports.empty())
        //     throw std::runtime_error(
        //         std::string(engineName(wasmEngines::Time)) + ": no export");

        // wasmtime_memory_t* mem = nullptr;
        // for (auto& [name, ext] : exports)
        // {
        //     if ((ext.kind == WASMTIME_EXTERN_MEMORY))
        //     {
        //         mem = &ext.of.memory;
        //         break;
        //     }
        // }

        // if (!mem)
        if (!(ok && item.kind == WASMTIME_EXTERN_MEMORY))
            throw std::runtime_error(
                std::string(engineName(wasmEngines::Time)) +
                ": no memory exported");

        auto& mem(item.of.memory);
        return {
            wasmtime3_memory_data(context, &mem),
            wasmtime3_memory_data_size(context, &mem)};
    }
};

struct my_module_t
{
    wasmtime_context_t* context = nullptr;
    module_t module;
    std::vector<my_mod_inst_t> mod_inst;
    // wasm_exporttype_vec_t export_types;

private:
    static module_t
    init(wasm_engine_t* e, vbytes const& wasmBin)
    {
        // wasm_byte_vec_t const code{wasmBin.size(), (char*)(wasmBin.data())};
        wasmtime_module_t* m = nullptr;
        wasmtime3_module_new(e, wasmBin.data(), wasmBin.size(), &m);
        module_t mt = {m, &wasmtime3_module_delete};
        return mt;
    }

public:
    my_module_t() : context(nullptr), module(nullptr, &wasmtime3_module_delete)
    // , export_types{0, nullptr}
    {
    }

    my_module_t(my_module_t&& o)
        : context(nullptr), module(nullptr, &wasmtime3_module_delete)
    // , export_types{0, nullptr}
    {
        *this = std::move(o);
    }

    my_module_t&
    operator=(my_module_t&& o)
    {
        if (this == &o)
            return *this;

        context = o.context;
        o.context = nullptr;
        module = std::move(o.module);
        mod_inst = std::move(o.mod_inst);

        // if (export_types.size)
        //     wasmtime2_exporttype_vec_delete(&export_types);
        // export_types = o.export_types;
        // o.export_types = {0, nullptr};

        return *this;
    }

    my_module_t(
        wasm_engine_t* e,
        wasmtime_context_t* ct,
        vbytes const& wasmBin,
        bool instantiate,
        std::vector<wasmtime_extern_t> const& imports = {})
        : context(ct), module(init(e, wasmBin))
    //, export_types{0, nullptr}
    {
        if (!module)
            throw std::runtime_error(
                std::string(engineName(wasmEngines::Time)) +
                " can't create module");

        // wasmtime2_module_exports(module.get(), &export_types);
        if (instantiate)
            mod_inst.emplace_back(ct, module.get(), imports);
    }

    ~my_module_t()
    {
        // if (export_types.size)
        // wasmtime2_exporttype_vec_delete(&export_types);
    }

    wasmtime_func_t
    getFunc(std::string_view funcName, int i)
    {
        return mod_inst[i].getFunc(funcName);
    }

    vmem
    getMem(int i)
    {
        return mod_inst[i].getMem();
    }

    int
    addInstance(std::vector<wasmtime_extern_t> const& imports = {})
    {
        for (int i = 0, e = mod_inst.size(); i < e; ++i)
        {
            auto& ins(mod_inst[i]);
            if (!ins)
            {
                ins = {context, module.get(), imports};
                return i;
            }
        }
        mod_inst.emplace_back(context, module.get(), imports);
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

}  // namespace

class WasmEngineTimeImpl
{
    std::unique_ptr<wasm_engine_t, decltype(&wasmtime2_engine_delete)> engine;
    std::unique_ptr<wasmtime_store_t, decltype(&wasmtime3_store_delete)> store;
    wasmtime_context_t* context = nullptr;
    std::vector<my_module_t> modules;
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

    int
    addModule(vbytes const& wasmCode, bool instantiate);
    void
    clearModules()
    {
        modules.clear();
        store.reset();
        store = {
            wasmtime3_store_new(engine.get(), nullptr, nullptr),
            &wasmtime3_store_delete};
        context = wasmtime3_store_context(store.get());
    }
    int
    addInstance(int m);

    int32_t
    runFunc(std::string_view const funcName, int32_t p, int m, int i);

    int64_t
    runFunc64(std::string_view const funcName, int64_t p, int m, int i);

    std::vector<uint64_t>
    runSha(std::string_view const data, int m, int i);

    std::int64_t
    setMeter(std::int64_t def);

    std::int64_t
    setGas(std::int64_t gas, int m, int i);

    std::int64_t
    getRemainingGas(int m, int i);

protected:
    int
    makeModule(
        vbytes const& wasmCode,
        std::vector<wasmtime_extern_t> const& imports =
            std::vector<wasmtime_extern_t>());

    wasmtime_func_t
    getFunc(std::string_view funcName, int m, int i = 0);

    vmem
    getMem(int m, int i = 0);

    void
    add_param(std::vector<wasmtime_val_t>& in, int32_t p);
    void
    add_param(std::vector<wasmtime_val_t>& in, int64_t p);

    template <int NR, class... Types>
    inline wsm_res
    call(std::string_view func, int m, int i, Types... args);

    template <int NR, class... Types>
    inline wsm_res
    call(wasmtime_func_t& func, int m, int i, Types... args);

    template <int NR, class... Types>
    inline wsm_res
    call(wasmtime_func_t& f, int m, int i, std::vector<wasmtime_val_t>& in);

    template <int NR, class... Types>
    inline wsm_res
    call(
        wasmtime_func_t& func,
        int m,
        int i,
        std::vector<wasmtime_val_t>& in,
        std::int32_t p,
        Types... args);

    template <int NR, class... Types>
    inline wsm_res
    call(
        wasmtime_func_t& func,
        int m,
        int i,
        std::vector<wasmtime_val_t>& in,
        std::int64_t p,
        Types... args);

    template <int NR, class... Types>
    inline wsm_res
    call(
        wasmtime_func_t& func,
        int m,
        int i,
        std::vector<wasmtime_val_t>& in,
        uint8_t const* d,
        std::size_t sz,
        Types... args);

    template <int NR, class... Types>
    inline wsm_res
    call(
        wasmtime_func_t& func,
        int m,
        int i,
        std::vector<wasmtime_val_t>& in,
        vbytes const& p,
        Types... args);
};

WasmEngineTimeImpl::WasmEngineTimeImpl()
    : engine(wasmtime2_engine_new(), &wasmtime2_engine_delete)
    , store(
          wasmtime3_store_new(engine.get(), nullptr, nullptr),
          &wasmtime3_store_delete)
    , context(wasmtime3_store_context(store.get()))
{
}

int
WasmEngineTimeImpl::makeModule(
    vbytes const& wasmCode,
    std::vector<wasmtime_extern_t> const& imports)
{
    modules.emplace_back(engine.get(), context, wasmCode, true, imports);
    return static_cast<int>(modules.size()) - 1;
}

int
WasmEngineTimeImpl::addModule(vbytes const& wasmCode, bool instantiate)
{
    modules.emplace_back(engine.get(), context, wasmCode, instantiate);
    return static_cast<int>(modules.size()) - 1;
}

int
WasmEngineTimeImpl::addInstance(int m)
{
    return modules[m].addInstance();
}

wasmtime_func_t
WasmEngineTimeImpl::getFunc(std::string_view funcName, int m, int i)
{
    return modules[m].getFunc(funcName, i);
}

vmem
WasmEngineTimeImpl::getMem(int m, int i)
{
    return modules[m].getMem(i);
}

void
WasmEngineTimeImpl::add_param(std::vector<wasmtime_val_t>& in, int32_t p)
{
    in.emplace_back(wasmtime_val_t WASM_I32_VAL(p));
}

void
WasmEngineTimeImpl::add_param(std::vector<wasmtime_val_t>& in, int64_t p)
{
    in.emplace_back(wasmtime_val_t WASM_I64_VAL(p));
}

template <int NR, class... Types>
wsm_res
WasmEngineTimeImpl::call(std::string_view func, int m, int i, Types... args)
{
    // Lookup our export function
    auto f = getFunc(func, m, i);
    return call<NR>(f, m, i, std::forward<Types>(args)...);
}

template <int NR, class... Types>
wsm_res
WasmEngineTimeImpl::call(wasmtime_func_t& func, int m, int i, Types... args)
{
    std::vector<wasmtime_val_t> in;
    return call<NR>(func, m, i, in, std::forward<Types>(args)...);
}

template <int NR, class... Types>
wsm_res
WasmEngineTimeImpl::call(
    wasmtime_func_t& func,
    int m,
    int i,
    std::vector<wasmtime_val_t>& in)
{
    wsm_res ret(NR);

    auto* err = wasmtime3_func_call(
        context,
        &func,
        in.data(),
        in.size(),
        ret.r.data(),
        ret.r.size(),
        &trap);
    if (err || trap)
        print_wasm_error("failed to call func", trap);

    // assert(results[0].kind == WASM_I32);
    // if (NR) printf("Result P5: %d\n", ret[0].of.i32);

    return ret;
}

template <int NR, class... Types>
wsm_res
WasmEngineTimeImpl::call(
    wasmtime_func_t& func,
    int m,
    int i,
    std::vector<wasmtime_val_t>& in,
    std::int32_t p,
    Types... args)
{
    add_param(in, p);
    return call<NR>(func, m, i, in, std::forward<Types>(args)...);
}

template <int NR, class... Types>
wsm_res
WasmEngineTimeImpl::call(
    wasmtime_func_t& func,
    int m,
    int i,
    std::vector<wasmtime_val_t>& in,
    std::int64_t p,
    Types... args)
{
    add_param(in, p);
    return call<NR>(func, m, i, in, std::forward<Types>(args)...);
}

template <int NR, class... Types>
wsm_res
WasmEngineTimeImpl::call(
    wasmtime_func_t& func,
    int m,
    int i,
    std::vector<wasmtime_val_t>& in,
    uint8_t const* d,
    std::size_t sz,
    Types... args)
{
    auto const res = call<1>(V_ALLOC, m, i, static_cast<int32_t>(sz));
    if (trap || (res.r[0].kind != WASMTIME_I32))
        return {};
    auto const ptr = res.r[0].of.i32;

    auto mem = getMem(m, i);
    memcpy(mem.p + ptr, d, sz);

    add_param(in, ptr);
    add_param(in, static_cast<int32_t>(sz));
    return call<NR>(func, m, i, in, std::forward<Types>(args)...);
}

template <int NR, class... Types>
wsm_res
WasmEngineTimeImpl::call(
    wasmtime_func_t& func,
    int m,
    int i,
    std::vector<wasmtime_val_t>& in,
    vbytes const& p,
    Types... args)
{
    return call<NR>(
        func, m, i, in, p.data(), p.size(), std::forward<Types>(args)...);
}

Expected<bool, TER>
WasmEngineTimeImpl::run(
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
    auto f = getFunc(funcName, m, i);
    auto res = call<1>(f, m, i, input);
    if (res.r.empty() || trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    assert(res.r[0].kind == WASMTIME_I32);
    // printf("Result: %d\n", results[0].of.i32);
    return res.r[0].of.i32 != 0;
}

Expected<bool, TER>
WasmEngineTimeImpl::run(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& accountID)
{
    // Create and instantiate the module.
    int const m = makeModule(wasmCode);
    int const i = 0;
    if (m < 0)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    auto f = getFunc(funcName, m, i);
    auto res = call<1>(f, m, i, accountID);
    if (trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    assert(res.r[0].kind == WASMTIME_I32);
    // printf("Result: %d\n", results[0].of.i32);
    return res.r[0].of.i32 == 1;
}

Expected<bool, TER>
WasmEngineTimeImpl::run(
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

    auto f = getFunc(funcName, m, i);
    auto res = call<1>(f, m, i, escrow_tx_json_data, escrow_lo_json_data);
    if (trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    assert(res.r[0].kind == WASMTIME_I32);
    // printf("Result: %d\n", results[0].of.i32);
    return res.r[0].of.i32 == 1;
}

Expected<std::pair<bool, std::string>, TER>
WasmEngineTimeImpl::runP4(
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
WasmEngineTimeImpl::justRunP4(
    std::string_view funcName,
    vbytes const& escrow_tx_json_data,
    vbytes const& escrow_lo_json_data,
    int m,
    int i)
{
    auto f = getFunc(funcName, m, i);
    auto res = call<1>(f, m, i, escrow_tx_json_data, escrow_lo_json_data);
    if (trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    assert(res.r[0].kind == WASMTIME_I32);
    // printf("Result: %d\n", results[0].of.i32);
    // return res.r[0].of.i32 == 1;
    auto const ptr = res.r[0].of.i32;
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
WasmEngineTimeImpl::run(
    vbytes const& wasmCode,
    std::string_view funcName,
    LedgerDataProvider* ledgerDataProvider)
{
    wasm_valtype_t* vtype(wasmtime2_valtype_new_i32());

    std::unique_ptr<wasm_functype_t, decltype(&wasmtime2_functype_delete)>
        ftype(wasmtime2_functype_new_0_1(vtype), &wasmtime2_functype_delete);

    wasmtime_func_t func;
    wasmtime3_func_new(
        context,
        ftype.get(),
        &get_ledger_sqn,
        ledgerDataProvider,
        nullptr,
        &func);

    std::vector<wasmtime_extern_t> import = {
        {.kind = WASMTIME_EXTERN_FUNC, .of = {.func = func}}};

    int const m = makeModule(wasmCode, import);
    int const i = 0;
    if (m < 0)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    auto f = getFunc(funcName, m, i);
    auto res = call<1>(f, m, i);
    if (trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    return res.r[0].kind == WASMTIME_I32 && res.r[0].of.i32;
}

int32_t
WasmEngineTimeImpl::runFunc(
    std::string_view const funcName,
    int32_t p,
    int m,
    int i)
{
    auto f = getFunc(funcName, m, i);
    auto res = call<1>(f, m, i, p);
    if (trap)
        return -1;

    return res.r[0].kind == WASMTIME_I32 ? res.r[0].of.i32 : -1;
}

int64_t
WasmEngineTimeImpl::runFunc64(
    std::string_view const funcName,
    int64_t p,
    int m,
    int i)
{
    auto f = getFunc(funcName, m, i);
    auto res = call<1>(f, m, i, p);
    if (trap)
        return -1;

    return res.r[0].kind == WASM_I64 ? res.r[0].of.i64 : -1;
}

std::vector<uint64_t>
WasmEngineTimeImpl::runSha(std::string_view const data, int m, int i)
{
    std::string_view funcName = "sha512_process";
    auto f = getFunc(funcName, m, i);
    auto res = call<1>(
        f, m, i, reinterpret_cast<uint8_t const*>(data.data()), data.size());
    if (trap)
        return {};

    auto const ptr = res.r[0].of.i32;
    std::uint64_t buf[8];
    memset(buf, 0, sizeof(buf));

    auto const mem = getMem(m, i);
    memcpy(buf, mem.p + ptr, 8 * sizeof(std::uint64_t));
    return {&buf[0], &buf[8]};
}

std::int64_t
WasmEngineTimeImpl::setMeter(std::int64_t def)
{
    modules.clear();
    store.reset();
    engine.reset();
    context = nullptr;

    wasm_config_t* config = wasmtime2_config_new();
    wasmtime3_config_consume_fuel_set(config, true);

    engine = {
        wasmtime2_engine_new_with_config(config), &wasmtime2_engine_delete};
    store = {
        wasmtime3_store_new(engine.get(), nullptr, nullptr),
        &wasmtime3_store_delete};
    context = wasmtime3_store_context(store.get());
    wasmtime3_context_set_fuel(context, static_cast<std::uint64_t>(def));

    // assert(wasmer_metering_get_remaining_points(instance) == 6);
    // assert(wasmer_metering_points_are_exhausted(instance) == false);

    return 0;
}

std::int64_t
WasmEngineTimeImpl::setGas(std::int64_t gas, int m, int i)
{
    wasmtime3_context_set_fuel(context, static_cast<std::uint64_t>(gas));
    return gas;
}

std::int64_t
WasmEngineTimeImpl::getRemainingGas(int m, int i)
{
    std::uint64_t gas = 0;
    wasmtime3_context_get_fuel(context, &gas);
    return static_cast<std::int64_t>(gas);
}

//////////////////////////////////////////////////////////////////////////////////////////

WasmEngineTime::WasmEngineTime()
    : WasmEngine({1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0})
    , impl(std::make_unique<WasmEngineTimeImpl>())
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

Expected<std::pair<bool, std::string>, TER>
WasmEngineTime::justRunP4(
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

int
WasmEngineTime::addModule(vbytes const& wasmCode, bool instantiate)
{
    try
    {
        return impl->addModule(wasmCode, instantiate);
    }
    catch (std::exception const& e)
    {
        std::cerr << engineName(wasmEngines::Time) << ": " << e.what()
                  << std::endl;
    }
    return -1;
}

void
WasmEngineTime::clearModules()
{
    return impl->clearModules();
}

int
WasmEngineTime::addInstance(int m)
{
    try
    {
        return impl->addInstance(m);
    }
    catch (std::exception const& e)
    {
        std::cerr << engineName(wasmEngines::Time) << ": " << e.what()
                  << std::endl;
    }
    return -1;
}

int32_t
WasmEngineTime::runFunc(
    std::string_view const funcName,
    int32_t p,
    int m,
    int i)
{
    return impl->runFunc(funcName, p, m, i);
}

int64_t
WasmEngineTime::runFunc64(
    std::string_view const funcName,
    int64_t p,
    int m,
    int i)
{
    return impl->runFunc64(funcName, p, m, i);
}

std::vector<uint64_t>
WasmEngineTime::runSha(std::string_view const data, int m, int i)
{
    return impl->runSha(data, m, i);
}

std::int64_t
WasmEngineTime::setMeter(std::int64_t def)
{
    return impl->setMeter(def);
}

std::int64_t
WasmEngineTime::setGas(std::int64_t gas, int m, int i)
{
    return impl->setGas(gas, m, i);
}

std::int64_t
WasmEngineTime::getRemainingGas(int m, int i)
{
    return impl->getRemainingGas(m, i);
}

}  // namespace ripple
