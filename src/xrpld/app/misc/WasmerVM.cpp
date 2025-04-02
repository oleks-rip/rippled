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

#include <xrpld/app/misc/WasmerVM.h>

#include <wasmer_so.h>

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
        wasmer2_trap_message(trap, &error_message);
        wasmer2_trap_delete(trap);
    }
    fprintf(stderr, "%.*s\n", (int)error_message.size, error_message.data);
    wasmer2_byte_vec_delete(&error_message);
}

// clang-format off
struct wsm_res
{
    wasm_val_vec_t r;
    wsm_res(unsigned N = 0):r{0, nullptr} {if (N) wasmer2_val_vec_new_uninitialized(&r, N);}
    ~wsm_res() { if (r.size) wasmer2_val_vec_delete(&r); }
    wsm_res(wsm_res const &) = delete;
    wsm_res& operator=(wsm_res const &) = delete;

    wsm_res(wsm_res &&o) {*this = std::move(o);}
    wsm_res& operator=(wsm_res  &&o){r = o.r; o.r = {0, nullptr}; return *this;}
    //operator wasm_val_vec_t &() {return r;}
};
// clang-format on

using module_t =
    std::unique_ptr<wasm_module_t, decltype(&wasmer2_module_delete)>;
using mod_inst_t =
    std::unique_ptr<wasm_instance_t, decltype(&wasmer2_instance_delete)>;

static wasm_trap_t*
_proc_exit(wasm_val_vec_t const* p, wasm_val_vec_t*)
{
    std::cout << "Exit called: " << std::to_string(p->data[0].of.i32)
              << std::endl;
    return nullptr;
}

struct my_mod_inst_t
{
    wasm_extern_vec_t exports;
    mod_inst_t mod_inst;

private:
    static void
    checkImport(
        wasm_extern_vec_t& out,
        wasm_store_t* s,
        wasm_module_t* m,
        wasm_extern_vec_t const& in)
    {
        wasm_importtype_vec_t impts = {0, nullptr};
        wasmer2_module_imports(m, &impts);

        unsigned inserted = 0;
        for (int i = 0; i < impts.size; ++i)
        {
            auto const* impt(impts.data[i]);

            wasm_name_t const* name = wasmer2_importtype_name(impt);
            wasm_externtype_t const* xtype = wasmer2_importtype_type(impt);
            if (wasmer2_externtype_kind(xtype) == WASM_EXTERN_FUNC)
            {
                if (VW_PROC_EXIT == std::string_view(name->data, name->size))
                {
                    std::unique_ptr<
                        wasm_functype_t,
                        decltype(&wasmer2_functype_delete)>
                        ftype(
                            wasmer2_functype_new_1_0(wasmer2_valtype_new_i32()),
                            &wasmer2_functype_delete);

                    wasm_func_t* func =
                        wasmer2_func_new(s, ftype.get(), &_proc_exit);

                    out.data[inserted++] = wasmer2_func_as_extern(func);
                    break;
                }
            }
        }

        if (inserted + in.size > MAX_IMPORT)
            throw std::runtime_error(
                std::string(engineName(wasmEngines::Time)) +
                std::string(" Too small import buffer "));

        for (int i = 0; i < in.size; ++i)
            out.data[inserted++] = in.data[i];

        out.size = inserted;
    }

    static mod_inst_t
    init(
        wasm_store_t* s,
        wasm_module_t* m,
        wasm_extern_vec_t* expt,
        wasm_extern_vec_t const& imports = WASM_EMPTY_VEC)
    {
        wasm_trap_t* trap = nullptr;

        ////////////////////////////////////////////////////////////////
        // check wasi

        wasm_extern_t* imp2_arr[MAX_IMPORT] = {nullptr};
        wasm_extern_vec_t imports2 = WASM_ARRAY_VEC(imp2_arr);
        checkImport(imports2, s, m, imports);

        ////////////////////////////////////////////////////////////////

        mod_inst_t mi = mod_inst_t(
            wasmer2_instance_new(s, m, &imports2, &trap),
            &wasmer2_instance_delete);
        if (!mi || trap)
        {
            print_wasm_error("can't create instance", trap);
            throw std::runtime_error(
                std::string(engineName(wasmEngines::Er)) +
                ": can't create instance");
        }
        wasmer2_instance_exports(mi.get(), expt);
        return mi;
    }

public:
    my_mod_inst_t()
        : exports{0, nullptr}, mod_inst(nullptr, &wasmer2_instance_delete)
    {
    }

    my_mod_inst_t(my_mod_inst_t&& o)
        : exports{0, nullptr}, mod_inst(nullptr, &wasmer2_instance_delete)
    {
        *this = std::move(o);
    }

    my_mod_inst_t&
    operator=(my_mod_inst_t&& o)
    {
        if (this == &o)
            return *this;

        if (exports.size)
            wasmer2_extern_vec_delete(&exports);
        exports = o.exports;
        o.exports = {0, nullptr};

        mod_inst = std::move(o.mod_inst);
        return *this;
    }

    my_mod_inst_t(
        wasm_store_t* s,
        wasm_module_t* m,
        wasm_extern_vec_t const& imports = WASM_EMPTY_VEC)
        : exports{0, nullptr}, mod_inst(init(s, m, &exports, imports))
    {
    }

    ~my_mod_inst_t()
    {
        if (exports.size)
            wasmer2_extern_vec_delete(&exports);
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
                std::string(engineName(wasmEngines::Er)) + ": no export");
        if (export_types.size != exports.size)
            throw std::runtime_error(
                std::string(engineName(wasmEngines::Er)) + ": invalid export");

        for (unsigned i = 0; i < export_types.size; ++i)
        {
            auto const* exp_type(export_types.data[i]);

            const wasm_externtype_t* exn_type =
                wasmer2_exporttype_type(exp_type);
            if (wasmer2_externtype_kind(exn_type) == WASM_EXTERN_FUNC)
            {
                wasm_name_t const* name = wasmer2_exporttype_name(exp_type);
                if (funcName == std::string_view(name->data, name->size))
                {
                    auto* exn(exports.data[i]);
                    if (wasmer2_extern_kind(exn) != WASM_EXTERN_FUNC)
                        throw std::runtime_error(
                            std::string(engineName(wasmEngines::Er)) +
                            ": invalid export");

                    f = wasmer2_extern_as_func(exn);
                    break;
                }
            }
        }

        if (!f)
            throw std::runtime_error(
                std::string(engineName(wasmEngines::Er)) +
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
            if (wasmer2_extern_kind(e) == WASM_EXTERN_MEMORY)
            {
                mem = wasmer2_extern_as_memory(e);
                break;
            }
        }

        if (!mem)
            throw std::runtime_error(
                std::string(engineName(wasmEngines::Er)) +
                ": no memory exported");

        return {
            reinterpret_cast<std::uint8_t*>(wasmer2_memory_data(mem)),
            wasmer2_memory_data_size(mem)};
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
        wasm_byte_vec_t const code{wasmBin.size(), (char*)(wasmBin.data())};
        module_t m =
            module_t(wasmer2_module_new(s, &code), &wasmer2_module_delete);
        return m;
    }

public:
    my_module_t()
        : module(nullptr, &wasmer2_module_delete), export_types{0, nullptr}
    {
    }

    my_module_t(my_module_t&& o)
        : module(nullptr, &wasmer2_module_delete), export_types{0, nullptr}
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
            wasmer2_exporttype_vec_delete(&export_types);
        export_types = o.export_types;
        o.export_types = {0, nullptr};

        return *this;
    }

    my_module_t(
        wasm_store_t* s,
        vbytes const& wasmBin,
        bool instantiate,
        wasm_extern_vec_t const& imports = WASM_EMPTY_VEC)
        : module(init(s, wasmBin)), export_types{0, nullptr}
    {
        if (!module)
            throw std::runtime_error(
                std::string(engineName(wasmEngines::Er)) +
                " + can't create module");

        wasmer2_module_exports(module.get(), &export_types);

        if (instantiate)
            mod_inst.emplace_back(s, module.get(), imports);
    }

    ~my_module_t()
    {
        if (export_types.size)
            wasmer2_exporttype_vec_delete(&export_types);
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

}  // namespace

class WasmEngineErImpl
{
    // std::unique_ptr<wasm_config_t, decltype(&wasmer2_config_delete)> config =
    // {nullptr, &wasmer2_config_delete};
    std::unique_ptr<wasm_engine_t, decltype(&wasmer2_engine_delete)> engine;
    std::unique_ptr<wasm_store_t, decltype(&wasmer2_store_delete)> store;
    std::vector<my_module_t> modules;
    // std::unique_ptr<wasmer_metering_t, decltype(&wasmer2_metering_delete)>
    //     meter = {nullptr, &wasmer2_metering_delete};

    wasm_trap_t* trap = nullptr;

public:
    WasmEngineErImpl();
    ~WasmEngineErImpl() = default;

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
        store.reset();
        store = {wasmer2_store_new(engine.get()), &wasmer2_store_delete};
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
        wasm_extern_vec_t const& imports = WASM_EMPTY_VEC);

    wasm_func_t*
    getFunc(std::string_view funcName, int m, int i = 0);

    vmem
    getMem(int m, int i = 0);

    inline void
    add_param(std::vector<wasm_val_t>& in, int32_t p);
    inline void
    add_param(std::vector<wasm_val_t>& in, int64_t p);

    template <int NR, class... Types>
    inline wsm_res
    call(std::string_view func, int m, int i, Types... args);

    template <int NR, class... Types>
    inline wsm_res
    call(wasm_func_t* func, int m, int i, Types... args);

    template <int NR, class... Types>
    wsm_res
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

WasmEngineErImpl::WasmEngineErImpl()
    : engine(wasmer2_engine_new(), &wasmer2_engine_delete)
    , store(wasmer2_store_new(engine.get()), &wasmer2_store_delete)
{
}

int
WasmEngineErImpl::makeModule(
    vbytes const& wasmCode,
    wasm_extern_vec_t const& imports)
{
    modules.emplace_back(store.get(), wasmCode, true, imports);
    return static_cast<int>(modules.size()) - 1;
}

int
WasmEngineErImpl::addModule(vbytes const& wasmCode, bool instantiate)
{
    modules.emplace_back(store.get(), wasmCode, instantiate);
    return static_cast<int>(modules.size()) - 1;
}

int
WasmEngineErImpl::addInstance(int m)
{
    return modules[m].addInstance(store.get());
}

wasm_func_t*
WasmEngineErImpl::getFunc(std::string_view funcName, int m, int i)
{
    return modules[m].getFunc(funcName, i);
}

vmem
WasmEngineErImpl::getMem(int m, int i)
{
    return modules[m].getMem(i);
}

void
WasmEngineErImpl::add_param(std::vector<wasm_val_t>& in, int32_t p)
{
    in.emplace_back(wasm_val_t WASM_I32_VAL(p));
}

void
WasmEngineErImpl::add_param(std::vector<wasm_val_t>& in, int64_t p)
{
    in.emplace_back(wasm_val_t WASM_I64_VAL(p));
}

template <int NR, class... Types>
inline wsm_res
WasmEngineErImpl::call(std::string_view func, int m, int i, Types... args)
{
    // Lookup our export function
    auto* f = getFunc(func, m, i);
    return call<NR>(f, m, i, std::forward<Types>(args)...);
}

template <int NR, class... Types>
wsm_res
WasmEngineErImpl::call(wasm_func_t* func, int m, int i, Types... args)
{
    std::vector<wasm_val_t> in;
    return call<NR>(func, m, i, in, std::forward<Types>(args)...);
}

template <int NR, class... Types>
wsm_res
WasmEngineErImpl::call(wasm_func_t* func, int, int, std::vector<wasm_val_t>& in)
{
    wsm_res ret(NR);

    wasm_val_vec_t const inv{in.size(), in.data()};
    trap = wasmer2_func_call(func, &inv, &ret.r);
    if (trap)
        print_wasm_error("failed to call func", trap);

    // assert(results[0].kind == WASM_I32);
    // if (NR) printf("Result P5: %d\n", ret[0].of.i32);

    return ret;
}

template <int NR, class... Types>
wsm_res
WasmEngineErImpl::call(
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
WasmEngineErImpl::call(
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
WasmEngineErImpl::call(
    wasm_func_t* func,
    int m,
    int i,
    std::vector<wasm_val_t>& in,
    uint8_t const* d,
    std::size_t sz,
    Types... args)
{
    auto const res = call<1>(V_ALLOC, m, i, static_cast<int32_t>(sz));
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
WasmEngineErImpl::call(
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
WasmEngineErImpl::run(
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
WasmEngineErImpl::run(
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
WasmEngineErImpl::run(
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
WasmEngineErImpl::runP4(
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
WasmEngineErImpl::justRunP4(
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
    // return res.r.data[0].of.i32 == 1;
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
WasmEngineErImpl::run(
    vbytes const& wasmCode,
    std::string_view funcName,
    LedgerDataProvider* ledgerDataProvider)
{
    wasm_valtype_t* vtype(wasmer2_valtype_new_i32());
    std::unique_ptr<wasm_functype_t, decltype(&wasmer2_functype_delete)> ftype(
        wasmer2_functype_new_0_1(vtype), &wasmer2_functype_delete);

    // std::unique_ptr<wasm_func_t, decltype(&wasmer2_func_delete)> func(
    //     wasmer2_func_new_with_env(store.get(),ftype.get(),
    //     &get_ledger_sqn, ledgerDataProvider, nullptr),
    //     &wasmer2_func_delete);

    wasm_func_t* func = wasmer2_func_new_with_env(
        store.get(), ftype.get(), &get_ledger_sqn, ledgerDataProvider, nullptr);

    wasm_extern_t* arr[] = {wasmer2_func_as_extern(func)};
    wasm_extern_vec_t imports = WASM_ARRAY_VEC(arr);
    int const m = makeModule(wasmCode, {imports});
    int const i = 0;
    if (m < 0)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    return justRun(funcName, ledgerDataProvider, m, i);
}

Expected<int, TER>
WasmEngineErImpl::preRun(
    vbytes const& wasmCode,
    LedgerDataProvider* ledgerDataProvider)
{
    wasm_valtype_t* vtype(wasmer2_valtype_new_i32());
    std::unique_ptr<wasm_functype_t, decltype(&wasmer2_functype_delete)> ftype(
        wasmer2_functype_new_0_1(vtype), &wasmer2_functype_delete);

    wasm_func_t* func = wasmer2_func_new_with_env(
        store.get(), ftype.get(), &get_ledger_sqn, ledgerDataProvider, nullptr);

    wasm_extern_t* arr[] = {wasmer2_func_as_extern(func)};
    wasm_extern_vec_t imports = WASM_ARRAY_VEC(arr);
    int const m = makeModule(wasmCode, {imports});
    if (m < 0)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    return m;
}

Expected<bool, TER>
WasmEngineErImpl::justRun(
    std::string_view funcName,
    LedgerDataProvider* ledgerDataProvider,
    int m,
    int i)
{
    auto* f = getFunc(funcName, m, i);
    auto res = call<1>(f, m, i);
    if (!res.r.size || trap)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    return res.r.data[0].kind == WASM_I32 && res.r.data[0].of.i32;
}

Expected<int, TER>
WasmEngineErImpl::justRun(std::string_view funcName, int m, int i)
{
    auto* f = getFunc(funcName, m, i);
    auto res = call<1>(f, m, i);
    if (!res.r.size || trap || res.r.data[0].kind != WASM_I32)
        return Unexpected<TER>(tecFAILED_PROCESSING);

    return res.r.data[0].of.i32;
}

int32_t
WasmEngineErImpl::runFunc(
    std::string_view const funcName,
    int32_t p,
    int m,
    int i)
{
    auto* f = getFunc(funcName, m, i);
    if (!f)
        throw std::runtime_error(
            std::string(engineName(wasmEngines::Er)) +
            std::string(" Can't find ") + funcName.data());

    auto res = call<1>(f, m, i, p);
    if (!res.r.size || trap)
        return -1;

    return res.r.data[0].kind == WASM_I32 ? res.r.data[0].of.i32 : -1;
}

int64_t
WasmEngineErImpl::runFunc64(
    std::string_view const funcName,
    int64_t p,
    int m,
    int i)
{
    auto* f = getFunc(funcName, m, i);
    if (!f)
        throw std::runtime_error(
            std::string(engineName(wasmEngines::Er)) +
            std::string(" Can't find ") + funcName.data());

    auto res = call<1>(f, m, i, p);
    if (!res.r.size || trap)
        return -1;

    return res.r.data[0].kind == WASM_I64 ? res.r.data[0].of.i64 : -1;
}

std::vector<uint64_t>
WasmEngineErImpl::runSha(std::string_view const data, int m, int i)
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

static std::uint64_t
cost_fun(wasmer_parser_operator_t wasm_operator)
{
    switch (wasm_operator)
    {
        // `local.get` and `i32.const` cost 1 unit.
        case LocalGet:
        case I32Const:
            return 1;

            // case I32Add:       return 2;

        default:
            return 1;
    }
}

std::int64_t
WasmEngineErImpl::setMeter(std::int64_t def)
{
    modules.clear();
    store.reset();
    engine.reset();

    wasmer_metering_t* meter =
        wasmer2_metering_new(static_cast<std::uint64_t>(def), &cost_fun);
    wasmer_middleware_t* middleware = wasmer2_metering_as_middleware(meter);
    wasm_config_t* config = wasmer2_config_new();
    wasmer2_config_push_middleware(config, middleware);

    engine = {wasmer2_engine_new_with_config(config), &wasmer2_engine_delete};
    store = {wasmer2_store_new(engine.get()), &wasmer2_store_delete};

    // assert(wasmer_metering_get_remaining_points(instance) == 6);
    // assert(wasmer_metering_points_are_exhausted(instance) == false);

    return 0;
}

std::int64_t
WasmEngineErImpl::setGas(std::int64_t gas, int m, int i)
{
    wasmer2_metering_set_remaining_points(
        modules[m].mod_inst[i].mod_inst.get(), static_cast<std::uint64_t>(gas));
    return gas;
}

std::int64_t
WasmEngineErImpl::getRemainingGas(int m, int i)
{
    return static_cast<std::int64_t>(wasmer2_metering_get_remaining_points(
        modules[m].mod_inst[i].mod_inst.get()));
}

//////////////////////////////////////////////////////////////////////////////////////////

WasmEngineEr::WasmEngineEr()
    : WasmEngine({1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0})
    , impl(std::make_unique<WasmEngineErImpl>())
{
}

WasmEngineEr::~WasmEngineEr() = default;

Expected<bool, TER>
WasmEngineEr::run(
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
WasmEngineEr::run(
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
WasmEngineEr::run(
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
WasmEngineEr::runP4(
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
WasmEngineEr::justRunP4(
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
WasmEngineEr::run(
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
WasmEngineEr::preRun(
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
WasmEngineEr::justRun(
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
WasmEngineEr::justRun(std::string_view funcName, int m, int i)
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
WasmEngineEr::addModule(vbytes const& wasmCode, bool instantiate)
{
    try
    {
        return impl->addModule(wasmCode, instantiate);
    }
    catch (std::exception const& e)
    {
        std::cerr << engineName(wasmEngines::Er) << ": " << e.what()
                  << std::endl;
    }
    return -1;
}

void
WasmEngineEr::clearModules()
{
    return impl->clearModules();
}

int
WasmEngineEr::addInstance(int m)
{
    try
    {
        return impl->addInstance(m);
    }
    catch (std::exception const& e)
    {
        std::cerr << engineName(wasmEngines::Er) << ": " << e.what()
                  << std::endl;
    }
    return -1;
}

int32_t
WasmEngineEr::runFunc(std::string_view const funcName, int32_t p, int m, int i)
{
    return impl->runFunc(funcName, p, m, i);
}

int64_t
WasmEngineEr::runFunc64(
    std::string_view const funcName,
    int64_t p,
    int m,
    int i)
{
    return impl->runFunc64(funcName, p, m, i);
}

std::vector<uint64_t>
WasmEngineEr::runSha(std::string_view const data, int m, int i)
{
    return impl->runSha(data, m, i);
}

std::int64_t
WasmEngineEr::setMeter(std::int64_t def)
{
    return impl->setMeter(def);
}

std::int64_t
WasmEngineEr::setGas(std::int64_t gas, int m, int i)
{
    return impl->setGas(gas, m, i);
}

std::int64_t
WasmEngineEr::getRemainingGas(int m, int i)
{
    return impl->getRemainingGas(m, i);
}

}  // namespace ripple
