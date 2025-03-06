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

#include <xrpld/app/misc/WasmEdgeVM.h>

#include <wasmedge_so.h>

#include <atomic>
#include <memory>

namespace ripple {

namespace {

static WasmEdge_Result
get_ledger_sqn(
    void* data,
    const WasmEdge_CallingFrameContext*,
    const WasmEdge_Value* In,
    WasmEdge_Value* Out)
{
    Out[0] =
        WasmEdge2_ValueGenI32(((LedgerDataProvider*)data)->get_ledger_sqn());
    return WasmEdge_Result_Success;
}

inline WasmEdge_String
to_edge(std::string_view s)
{
    return {static_cast<std::uint32_t>(s.size()), s.data()};
}

using mod_inst_t = std::unique_ptr<
    WasmEdge_ModuleInstanceContext,
    decltype(&WasmEdge2_ModuleInstanceDelete)>;
using module_t = std::
    unique_ptr<WasmEdge_ASTModuleContext, decltype(&WasmEdge2_ASTModuleDelete)>;
using config_t = std::
    unique_ptr<WasmEdge_ConfigureContext, decltype(&WasmEdge2_ConfigureDelete)>;
using store_t =
    std::unique_ptr<WasmEdge_StoreContext, decltype(&WasmEdge2_StoreDelete)>;
using validator_t = std::
    unique_ptr<WasmEdge_ValidatorContext, decltype(&WasmEdge2_ValidatorDelete)>;
using loader_t =
    std::unique_ptr<WasmEdge_LoaderContext, decltype(&WasmEdge2_LoaderDelete)>;
using executor_t = std::
    unique_ptr<WasmEdge_ExecutorContext, decltype(&WasmEdge2_ExecutorDelete)>;
using engine_t =
    std::unique_ptr<WasmEdge_VMContext, decltype(&WasmEdge2_VMDelete)>;
using imports_t =
    std::vector<std::pair<std::string_view, WasmEdge_FunctionInstanceContext*>>;

struct my_mod_inst_t
{
    mod_inst_t mod_inst;

private:
    static mod_inst_t
    init(
        WasmEdge_StoreContext* s,
        WasmEdge_ExecutorContext* x,
        WasmEdge_ASTModuleContext* m)
    {
        WasmEdge_ModuleInstanceContext* mi = nullptr;
        auto const res = WasmEdge2_ExecutorInstantiate(x, &mi, s, m);
        if (!WasmEdge2_ResultOK(res))
            throw std::runtime_error(
                std::string(engineName(wasmEngines::Er)) +
                ": can't create instance, e:" +
                WasmEdge2_ResultGetMessage(res));
        return {mi, &WasmEdge2_ModuleInstanceDelete};
    }

public:
    my_mod_inst_t() : mod_inst(nullptr, &WasmEdge2_ModuleInstanceDelete)
    {
    }

    my_mod_inst_t(my_mod_inst_t&& o)
        : mod_inst(nullptr, &WasmEdge2_ModuleInstanceDelete)
    {
        std::swap(mod_inst, o.mod_inst);
    }

    my_mod_inst_t&
    operator=(my_mod_inst_t&& o)
    {
        if (this == &o)
            return *this;

        std::swap(mod_inst, o.mod_inst);
        return *this;
    }

    my_mod_inst_t(
        WasmEdge_StoreContext* s,
        WasmEdge_ExecutorContext* x,
        WasmEdge_ASTModuleContext* m)
        : mod_inst(init(s, x, m))
    {
    }

    ~my_mod_inst_t() = default;

    operator bool() const
    {
        return static_cast<bool>(mod_inst);
    }

    WasmEdge_FunctionInstanceContext*
    getFunc(std::string_view funcName) const
    {
        return WasmEdge2_ModuleInstanceFindFunction(
            mod_inst.get(), to_edge(funcName));
    }

    WasmEdge_MemoryInstanceContext*
    getMem() const
    {
        return WasmEdge2_ModuleInstanceFindMemory(
            mod_inst.get(), to_edge(V_MEM));
    }
};

struct my_module_t
{
    module_t module;
    std::vector<my_mod_inst_t> mod_inst;

private:
    static module_t
    init(WasmEdge_LoaderContext* loader, vbytes const& wasmBin)
    {
        WasmEdge_ASTModuleContext* m = nullptr;
        WasmEdge_Result Res = WasmEdge2_LoaderParseFromBuffer(
            loader, &m, wasmBin.data(), wasmBin.size());
        if (!WasmEdge2_ResultOK(Res))
            throw std::runtime_error(
                std::string(engineName(wasmEngines::Edge)) +
                " can't parse module");

        return module_t(m, &WasmEdge2_ASTModuleDelete);
    }

public:
    my_module_t() : module(nullptr, &WasmEdge2_ASTModuleDelete)
    {
    }

    my_module_t(my_module_t&& o) : module(nullptr, &WasmEdge2_ASTModuleDelete)
    {
        std::swap(module, o.module);
        std::swap(mod_inst, o.mod_inst);
    }

    my_module_t(
        WasmEdge_StoreContext* s,
        WasmEdge_LoaderContext* loader,
        WasmEdge_ValidatorContext* validator,
        WasmEdge_ExecutorContext* x,
        vbytes const& wasmBin,
        imports_t const& imports = {})
        : module(init(loader, wasmBin))

    {
        if (!module)
            throw std::runtime_error(
                std::string(engineName(wasmEngines::Edge)) +
                " + can't create module");

        WasmEdge_Result Res =
            WasmEdge2_ValidatorValidate(validator, module.get());
        if (!WasmEdge2_ResultOK(Res))
            throw std::runtime_error(
                std::string(engineName(wasmEngines::Edge)) +
                " + can't validate module");

        if (!imports.empty())
        {
            WasmEdge_ModuleInstanceContext* hostMI =
                WasmEdge2_ModuleInstanceCreate(to_edge("host_lib"));
            if (!hostMI)
                throw std::runtime_error(
                    std::string(engineName(wasmEngines::Edge)) +
                    " + can't create import instance");

            for (auto const& [n, f] : imports)
                WasmEdge2_ModuleInstanceAddFunction(hostMI, to_edge(n), f);

            Res = WasmEdge2_ExecutorRegisterImport(x, s, hostMI);
            if (!WasmEdge2_ResultOK(Res))
                throw std::runtime_error(
                    std::string(engineName(wasmEngines::Edge)) +
                    " + can't register import instance");
        }

        mod_inst.emplace_back(s, x, module.get());
    }

    my_module_t&
    operator=(my_module_t&& o)
    {
        if (this == &o)
            return *this;
        std::swap(module, o.module);
        std::swap(mod_inst, o.mod_inst);

        return *this;
    }

    ~my_module_t() = default;

    WasmEdge_FunctionInstanceContext*
    getFunc(std::string_view funcName, int i) const
    {
        return mod_inst[i].getFunc(funcName);
    }

    WasmEdge_MemoryInstanceContext*
    getMem(int i) const
    {
        return mod_inst[i].getMem();
    }

    int
    addInstance(WasmEdge_StoreContext* s, WasmEdge_ExecutorContext* x)
    {
        for (int i = 0, e = mod_inst.size(); i < e; ++i)
        {
            auto& ins(mod_inst[i]);
            if (!ins)
            {
                ins = {s, x, module.get()};
                return i;
            }
        }
        mod_inst.emplace_back(s, x, module.get());
        return static_cast<int>(mod_inst.size());
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

class WasmEngineEdgeImpl
{
    config_t config;
    store_t store;
    validator_t validator;
    loader_t loader;
    executor_t executor;

    std::vector<my_module_t> modules;

    engine_t engine;

    // std::atomic_int ctr;
    WasmEdge_Result funcRes{0};

    static config_t
    initConfig()
    {
        config_t c(WasmEdge2_ConfigureCreate(), &WasmEdge2_ConfigureDelete);
        WasmEdge2_ConfigureAddHostRegistration(
            c.get(), WasmEdge_HostRegistration_Wasi);
        return c;
    }

public:
    WasmEngineEdgeImpl();
    ~WasmEngineEdgeImpl() = default;

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
        vbytes const& wasmCode,
        std::string_view funcName,
        vbytes const& escrow_tx_json_data,
        vbytes const& escrow_lo_json_data);

    Expected<bool, TER>
    run(vbytes const& wasmCode,
        std::string_view funcName,
        LedgerDataProvider* ledgerDataProvider);

    int
    addModule(vbytes const& wasmCode);
    int
    addInstance(int m);

    int64_t
    runFunc(std::string_view const funcName, int32_t p, int m, int i);

protected:
    bool
    makeModule(
        // std::string_view name,
        vbytes const& wasmCode,
        imports_t const& imports = {});

    WasmEdge_FunctionInstanceContext*
    getFunc(std::string_view funcName, int m = 0, int i = 0);

    WasmEdge_MemoryInstanceContext*
    getMem(int m = 0, int i = 0);

    void
    add_param(std::vector<WasmEdge_Value>& in, int32_t p);
    void
    add_param(std::vector<WasmEdge_Value>& in, int64_t p);

    template <int NR, class... Types>
    inline std::vector<WasmEdge_Value>
    call(std::string_view func, Types... args);

    template <int NR, class... Types>
    inline std::vector<WasmEdge_Value>
    call(WasmEdge_FunctionInstanceContext* func, Types... args);

    template <int NR, class... Types>
    std::vector<WasmEdge_Value>
    call(WasmEdge_FunctionInstanceContext* f, std::vector<WasmEdge_Value>& in);

    template <int NR, class... Types>
    inline std::vector<WasmEdge_Value>
    call(
        WasmEdge_FunctionInstanceContext* func,
        std::vector<WasmEdge_Value>& in,
        std::int32_t p,
        Types... args);

    template <int NR, class... Types>
    inline std::vector<WasmEdge_Value>
    call(
        WasmEdge_FunctionInstanceContext* func,
        std::vector<WasmEdge_Value>& in,
        std::int64_t p,
        Types... args);

    template <int NR, class... Types>
    inline std::vector<WasmEdge_Value>
    call(
        WasmEdge_FunctionInstanceContext* func,
        std::vector<WasmEdge_Value>& in,
        uint8_t const* m,
        std::size_t sz,
        Types... args);

    template <int NR, class... Types>
    inline std::vector<WasmEdge_Value>
    call(
        WasmEdge_FunctionInstanceContext* func,
        std::vector<WasmEdge_Value>& in,
        vbytes const& p,
        Types... args);
};

WasmEngineEdgeImpl::WasmEngineEdgeImpl()
    : config(initConfig())
    , store(WasmEdge2_StoreCreate(), &WasmEdge2_StoreDelete)
    , validator(
          WasmEdge2_ValidatorCreate(config.get()),
          &WasmEdge2_ValidatorDelete)
    , loader(WasmEdge2_LoaderCreate(config.get()), &WasmEdge2_LoaderDelete)
    , executor(
          WasmEdge2_ExecutorCreate(config.get(), nullptr),
          &WasmEdge2_ExecutorDelete)
    , engine(WasmEdge2_VMCreate(config.get(), store.get()), &WasmEdge2_VMDelete)
{
}

Expected<bool, TER>
WasmEngineEdgeImpl::run(
    vbytes const& wasmCode,
    std::string_view funcName,
    int32_t input)
{
    if (makeModule(  //"mod01",
            wasmCode))
        return Unexpected<TER>(tecFAILED_PROCESSING);

    auto const Returns = call<1>(funcName, input);
    if (!WasmEdge2_ResultOK(funcRes))
        return Unexpected<TER>(tecFAILED_PROCESSING);

    auto const result = WasmEdge2_ValueGetI32(Returns[0]);
    // printf("Get the result: %d\n", result);

    return result != 0;
}

Expected<bool, TER>
WasmEngineEdgeImpl::run(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& accountID)
{
    if (makeModule(  //"mod01",
            wasmCode))
        return Unexpected<TER>(tecFAILED_PROCESSING);

    auto const Returns = call<1>(funcName, accountID);
    if (!WasmEdge2_ResultOK(funcRes))
        return Unexpected<TER>(tecFAILED_PROCESSING);

    auto const result = WasmEdge2_ValueGetI32(Returns[0]);
    // printf("Get the result: %d\n", result);

    return result == 1;
}

Expected<bool, TER>
WasmEngineEdgeImpl::run(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& escrow_tx_json_data,
    vbytes const& escrow_lo_json_data)
{
    if (makeModule(wasmCode))
        return Unexpected<TER>(tecFAILED_PROCESSING);

    auto const Returns =
        call<1>(funcName, escrow_tx_json_data, escrow_lo_json_data);
    if (!WasmEdge2_ResultOK(funcRes))
        return Unexpected<TER>(tecFAILED_PROCESSING);

    auto const result = WasmEdge2_ValueGetI32(Returns[0]);
    // printf("Get the result: %d\n", result);

    return result == 1;
}

Expected<std::pair<bool, std::string>, TER>
WasmEngineEdgeImpl::runP4(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& escrow_tx_json_data,
    vbytes const& escrow_lo_json_data)
{
    // Create and instantiate the module.
    if (makeModule(wasmCode))
        return Unexpected<TER>(tecFAILED_PROCESSING);

    return justRunP4(
        wasmCode, funcName, escrow_tx_json_data, escrow_lo_json_data);
}

Expected<std::pair<bool, std::string>, TER>
WasmEngineEdgeImpl::justRunP4(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& escrow_tx_json_data,
    vbytes const& escrow_lo_json_data)
{
    auto const Returns =
        call<1>(funcName, escrow_tx_json_data, escrow_lo_json_data);
    if (!WasmEdge2_ResultOK(funcRes))
        return Unexpected<TER>(tecFAILED_PROCESSING);

    auto const ptr = WasmEdge2_ValueGetI32(Returns[0]);
    std::uint8_t buf[16];
    memset(buf, 0, sizeof(buf));

    // memcpy(buf, mem.p + ptr, 9);
    auto const* mem = getMem();
    WasmEdge2_MemoryInstanceGetData(mem, buf, ptr, 9);

    auto const flag = buf[0];
    auto const ret_ptr = *reinterpret_cast<int32_t const*>(buf + 1);
    auto const ret_len = *reinterpret_cast<int32_t const*>(buf + 5);
    // printf("re flag %d, ptr %d, len %d\n", flag, ret_pointer, ret_len);

    // memcpy(buf2.data(), mem.p + ret_pointer, ret_len);
    vbytes buf2(ret_len);
    WasmEdge2_MemoryInstanceGetData(mem, buf2.data(), ret_ptr, ret_len);

    std::string newData(buf2.begin(), buf2.end());

    call<0>(V_DEALLOC, ret_ptr, ret_len);
    if (!WasmEdge2_ResultOK(funcRes))
        return Unexpected<TER>(tecFAILED_PROCESSING);
    call<0>(V_DEALLOC, ptr, 9);
    if (!WasmEdge2_ResultOK(funcRes))
        return Unexpected<TER>(tecFAILED_PROCESSING);

    return std::pair<bool, std::string>(flag == 1, newData);
}

Expected<bool, TER>
WasmEngineEdgeImpl::run(
    vbytes const& wasmCode,
    std::string_view funcName,
    LedgerDataProvider* ledgerDataProvider)
{
    WasmEdge_ValType rtype[] = {WasmEdge2_ValTypeGenI32()};
    std::unique_ptr<
        WasmEdge_FunctionTypeContext,
        decltype(&WasmEdge2_FunctionTypeDelete)>
        ftype{
            WasmEdge2_FunctionTypeCreate(nullptr, 0, rtype, 1),
            &WasmEdge2_FunctionTypeDelete};
    WasmEdge_FunctionInstanceContext* func = WasmEdge2_FunctionInstanceCreate(
        ftype.get(), &get_ledger_sqn, ledgerDataProvider, 0);

    if (makeModule(wasmCode, {{"get_ledger_sqn", func}}))
        return Unexpected<TER>(tecFAILED_PROCESSING);

    auto const Returns = call<1>(funcName);
    if (!WasmEdge2_ResultOK(funcRes))
        return Unexpected<TER>(tecFAILED_PROCESSING);

    auto const result = WasmEdge2_ValueGetI32(Returns[0]);
    // printf("Get the result: %d\n", result);

    return result;
}

bool
WasmEngineEdgeImpl::makeModule(
    // std::string_view name,
    vbytes const& wasmCode,
    imports_t const& imports)
{
    modules.emplace_back(
        // name,
        store.get(),
        loader.get(),
        validator.get(),
        executor.get(),
        wasmCode,
        imports);
    return false;  // to be compatible with other VMs
}

int
WasmEngineEdgeImpl::addModule(vbytes const& wasmCode)
{
    // std::string mn = "module_" + std::to_string(ctr++);
    modules.emplace_back(
        store.get(), loader.get(), validator.get(), executor.get(), wasmCode);
    return static_cast<int>(modules.size());
}

int
WasmEngineEdgeImpl::addInstance(int m)
{
    return modules[m].addInstance(store.get(), executor.get());
}

WasmEdge_FunctionInstanceContext*
WasmEngineEdgeImpl::getFunc(std::string_view funcName, int m, int i)
{
    return modules[m].getFunc(funcName, i);
}

WasmEdge_MemoryInstanceContext*
WasmEngineEdgeImpl::getMem(int m, int i)
{
    return modules[m].getMem(i);
}

void
WasmEngineEdgeImpl::add_param(std::vector<WasmEdge_Value>& in, int32_t p)
{
    in.emplace_back();
    auto& el(in.back());
    memset(&el, 0, sizeof(el));
    el = WasmEdge2_ValueGenI32(p);  // WASM_I32;
}

void
WasmEngineEdgeImpl::add_param(std::vector<WasmEdge_Value>& in, int64_t p)
{
    in.emplace_back();
    auto& el(in.back());
    memset(&el, 0, sizeof(el));
    el = WasmEdge2_ValueGenI64(p);
}

template <int NR, class... Types>
inline std::vector<WasmEdge_Value>
WasmEngineEdgeImpl::call(std::string_view func, Types... args)
{
    // Lookup our export function
    auto* f = getFunc(func);
    if (!f)
    {
        throw std::runtime_error(
            std::string(engineName(wasmEngines::Edge)) +
            std::string("Can't find ") + func.data());
    }

    return call<NR>(f, std::forward<Types>(args)...);
}

template <int NR, class... Types>
std::vector<WasmEdge_Value>
WasmEngineEdgeImpl::call(WasmEdge_FunctionInstanceContext* func, Types... args)
{
    std::vector<WasmEdge_Value> in;
    return call<NR>(func, in, std::forward<Types>(args)...);
}

template <int NR, class... Types>
std::vector<WasmEdge_Value>
WasmEngineEdgeImpl::call(
    WasmEdge_FunctionInstanceContext* func,
    std::vector<WasmEdge_Value>& in)
{
    std::vector<WasmEdge_Value> ret;
    if (NR)
    {
        ret.resize(NR);
        memset(ret.data(), 0, NR * sizeof(ret[0]));
    }

    funcRes = WasmEdge2_ExecutorInvoke(
        executor.get(), func, in.data(), in.size(), ret.data(), ret.size());
    if (!WasmEdge2_ResultOK(funcRes))
    {
        std::cerr << std::string("failed to call func ") +
                WasmEdge2_ResultGetMessage(funcRes)
                  << std::endl;
        return {};
    }

    // assert(results[0].kind == WASM_I32);
    // if (NR) printf("Result P5: %d\n", ret[0].of.i32);

    return ret;
}

template <int NR, class... Types>
std::vector<WasmEdge_Value>
WasmEngineEdgeImpl::call(
    WasmEdge_FunctionInstanceContext* func,
    std::vector<WasmEdge_Value>& in,
    std::int32_t p,
    Types... args)
{
    add_param(in, p);
    return call<NR>(func, in, std::forward<Types>(args)...);
}

template <int NR, class... Types>
std::vector<WasmEdge_Value>
WasmEngineEdgeImpl::call(
    WasmEdge_FunctionInstanceContext* func,
    std::vector<WasmEdge_Value>& in,
    std::int64_t p,
    Types... args)
{
    add_param(in, p);
    return call<NR>(func, in, std::forward<Types>(args)...);
}

template <int NR, class... Types>
std::vector<WasmEdge_Value>
WasmEngineEdgeImpl::call(
    WasmEdge_FunctionInstanceContext* func,
    std::vector<WasmEdge_Value>& in,
    uint8_t const* m,
    std::size_t sz,
    Types... args)
{
    auto const res = call<1>(V_ALLOC, static_cast<int32_t>(sz));
    if (!WasmEdge2_ResultOK(funcRes))
    {
        std::cerr << std::string("failed to call func ") +
                WasmEdge2_ResultGetMessage(funcRes)
                  << std::endl;
        return {};
    }

    auto const ptr = WasmEdge2_ValueGetI32(res[0]);
    auto* mem = getMem();
    if (!mem)
    {
        throw std::runtime_error(
            std::string(engineName(wasmEngines::Edge)) + " Can't find memory");
    }

    // memcpy(mem.p + ptr, m, sz);
    funcRes = WasmEdge2_MemoryInstanceSetData(mem, m, ptr, sz);
    if (!WasmEdge2_ResultOK(funcRes))
    {
        std::cerr << std::string("failed to call func ") +
                WasmEdge2_ResultGetMessage(funcRes)
                  << std::endl;
        return {};
    }

    add_param(in, ptr);
    add_param(in, static_cast<int32_t>(sz));
    return call<NR>(func, in, std::forward<Types>(args)...);
}

template <int NR, class... Types>
std::vector<WasmEdge_Value>
WasmEngineEdgeImpl::call(
    WasmEdge_FunctionInstanceContext* func,
    std::vector<WasmEdge_Value>& in,
    vbytes const& p,
    Types... args)
{
    return call<NR>(func, in, p.data(), p.size(), std::forward<Types>(args)...);
}

int64_t
WasmEngineEdgeImpl::runFunc(
    std::string_view const funcName,
    int32_t p,
    int m,
    int i)
{
    auto* f = getFunc(funcName, m, i);
    if (!f)
        throw std::runtime_error(
            std::string(engineName(wasmEngines::Edge)) +
            std::string(" Can't find ") + funcName.data());

    auto res = call<1>(f, p);
    if (!res.size() || !WasmEdge2_ResultOK(funcRes))
        return -1;

    auto const result = WasmEdge2_ValueGetI64(res[0]);
    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////

WasmEngineEdge::WasmEngineEdge() : impl(std::make_unique<WasmEngineEdgeImpl>())
{
}

WasmEngineEdge::~WasmEngineEdge() = default;

Expected<bool, TER>
WasmEngineEdge::run(
    vbytes const& wasmCode,
    std::string_view funcName,
    int32_t input)
{
    try
    {
        return impl->run(wasmCode, funcName, input);
    }
    catch (std::exception const& e)
    {
        // std::cerr << "EXC: " << e.what() << std::endl;
    }
    return Unexpected<TER>(tecFAILED_PROCESSING);
}

Expected<bool, TER>
WasmEngineEdge::run(
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
WasmEngineEdge::run(
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
WasmEngineEdge::runP4(
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
WasmEngineEdge::justRunP4(
    vbytes const& wasmCode,
    std::string_view funcName,
    vbytes const& escrow_tx_json_data,
    vbytes const& escrow_lo_json_data)
{
    try
    {
        return impl->justRunP4(
            wasmCode, funcName, escrow_tx_json_data, escrow_lo_json_data);
    }
    catch (std::exception const&)
    {
    }
    return Unexpected<TER>(tecFAILED_PROCESSING);
}

Expected<bool, TER>
WasmEngineEdge::run(
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
WasmEngineEdge::addModule(vbytes const& wasmCode)
{
    try
    {
        return impl->addModule(wasmCode);
    }
    catch (std::exception const& e)
    {
        std::cerr << engineName(wasmEngines::Edge) << ": " << e.what()
                  << std::endl;
    }
    return -1;
}

int
WasmEngineEdge::addInstance(int m)
{
    try
    {
        return impl->addInstance(m);
    }
    catch (std::exception const& e)
    {
        std::cerr << engineName(wasmEngines::Edge) << ": " << e.what()
                  << std::endl;
    }
    return -1;
}

int64_t
WasmEngineEdge::runFunc(
    std::string_view const funcName,
    int32_t p,
    int m,
    int i)
{
    return impl->runFunc(funcName, p, m, i);
}

}  // namespace ripple
