#include "wasmedge_so.h"

// VALUES
WasmEdge_Value
WasmEdge2_ValueGenI32(const int32_t Val)
{
    return WasmEdge_ValueGenI32(Val);
}

WasmEdge_ValType
WasmEdge2_ValTypeGenI32(void)
{
    return WasmEdge_ValTypeGenI32();
}

int32_t
WasmEdge2_ValueGetI32(const WasmEdge_Value Val)
{
    return WasmEdge_ValueGetI32(Val);
}

int64_t
WasmEdge2_ValueGetI64(const WasmEdge_Value Val)
{
    return WasmEdge_ValueGetI64(Val);
}

bool
WasmEdge2_ValTypeIsI64(const WasmEdge_ValType ValType)
{
    return WasmEdge_ValTypeIsI64(ValType);
}

WasmEdge_Value
WasmEdge2_ValueGenI64(const int64_t Val)
{
    return WasmEdge_ValueGenI64(Val);
}

WasmEdge_ValType
WasmEdge2_ValTypeGenI64(void)
{
    return WasmEdge_ValTypeGenI64();
}

WasmEdge_String
WasmEdge2_StringCreateByCString(const char* Val)
{
    return WasmEdge_StringCreateByCString(Val);
}
void
WasmEdge2_StringDelete(WasmEdge_String Val)
{
    return WasmEdge_StringDelete(Val);
}

// CONFIG
WasmEdge_ConfigureContext*
WasmEdge2_ConfigureCreate(void)
{
    return WasmEdge_ConfigureCreate();
}
void
WasmEdge2_ConfigureDelete(WasmEdge_ConfigureContext* Ctx)
{
    return WasmEdge_ConfigureDelete(Ctx);
}

void
WasmEdge2_ConfigureAddHostRegistration(
    WasmEdge_ConfigureContext* Cxt,
    const enum WasmEdge_HostRegistration Host)
{
    return WasmEdge_ConfigureAddHostRegistration(Cxt, Host);
}

void
WasmEdge2_ConfigureStatisticsSetInstructionCounting(
    WasmEdge_ConfigureContext* Cxt,
    const bool IsCount)
{
    return WasmEdge_ConfigureStatisticsSetInstructionCounting(Cxt, IsCount);
}

void
WasmEdge2_ConfigureStatisticsSetCostMeasuring(
    WasmEdge_ConfigureContext* Cxt,
    const bool IsMeasure)
{
    return WasmEdge_ConfigureStatisticsSetCostMeasuring(Cxt, IsMeasure);
}

// STORE
WasmEdge_StoreContext*
WasmEdge2_StoreCreate()
{
    return WasmEdge_StoreCreate();
}
void
WasmEdge2_StoreDelete(WasmEdge_StoreContext* Ctx)
{
    return WasmEdge_StoreDelete(Ctx);
}

// LOADER
WasmEdge_LoaderContext*
WasmEdge2_LoaderCreate(const WasmEdge_ConfigureContext* ConfCxt)
{
    return WasmEdge_LoaderCreate(ConfCxt);
}
void
WasmEdge2_LoaderDelete(WasmEdge_LoaderContext* Ctx)
{
    return WasmEdge_LoaderDelete(Ctx);
}

// VM
WasmEdge_VMContext*
WasmEdge2_VMCreate(
    const WasmEdge_ConfigureContext* ConfCtx,
    WasmEdge_StoreContext* StoreCtx)
{
    return WasmEdge_VMCreate(ConfCtx, StoreCtx);
}
void
WasmEdge2_VMDelete(WasmEdge_VMContext* Ctx)
{
    return WasmEdge_VMDelete(Ctx);
}

WasmEdge_Result
WasmEdge2_VMLoadWasmFromBuffer(
    WasmEdge_VMContext* Ctx,
    const uint8_t* Buf,
    const uint32_t BufLen)
{
    return WasmEdge_VMLoadWasmFromBuffer(Ctx, Buf, BufLen);
}

WasmEdge_Result
WasmEdge2_VMRunWasmFromBuffer(
    WasmEdge_VMContext* Ctx,
    const uint8_t* Buf,
    const uint32_t BufLen,
    const WasmEdge_String FuncName,
    const WasmEdge_Value* Params,
    const uint32_t ParamLen,
    WasmEdge_Value* Returns,
    const uint32_t ReturnLen)
{
    return WasmEdge_VMRunWasmFromBuffer(
        Ctx, Buf, BufLen, FuncName, Params, ParamLen, Returns, ReturnLen);
}

WasmEdge_Result
WasmEdge2_VMExecute(
    WasmEdge_VMContext* Ctx,
    const WasmEdge_String FuncName,
    const WasmEdge_Value* Params,
    const uint32_t ParamLen,
    WasmEdge_Value* Returns,
    const uint32_t ReturnLen)
{
    return WasmEdge_VMExecute(
        Ctx, FuncName, Params, ParamLen, Returns, ReturnLen);
}

WasmEdge_Result
WasmEdge2_VMValidate(WasmEdge_VMContext* Ctx)
{
    return WasmEdge_VMValidate(Ctx);
}

WasmEdge_Result
WasmEdge2_VMInstantiate(WasmEdge_VMContext* Ctx)
{
    return WasmEdge_VMInstantiate(Ctx);
}

WasmEdge_Result
WasmEdge2_VMRegisterModuleFromImport(
    WasmEdge_VMContext* Cxt,
    const WasmEdge_ModuleInstanceContext* ImportCxt)
{
    return WasmEdge_VMRegisterModuleFromImport(Cxt, ImportCxt);
}

const WasmEdge_ModuleInstanceContext*
WasmEdge2_VMGetActiveModule(const WasmEdge_VMContext* Ctx)
{
    return WasmEdge_VMGetActiveModule(Ctx);
}

// MODULE
WasmEdge_Result
WasmEdge2_LoaderParseFromBuffer(
    WasmEdge_LoaderContext* Cxt,
    WasmEdge_ASTModuleContext** Module,
    const uint8_t* Buf,
    const uint32_t BufLen)
{
    return WasmEdge_LoaderParseFromBuffer(Cxt, Module, Buf, BufLen);
}

void
WasmEdge2_ASTModuleDelete(WasmEdge_ASTModuleContext* Cxt)
{
    return WasmEdge_ASTModuleDelete(Cxt);
}

uint32_t
WasmEdge2_ASTModuleListImportsLength(const WasmEdge_ASTModuleContext* Cxt)
{
    return WasmEdge_ASTModuleListImportsLength(Cxt);
}

uint32_t
WasmEdge2_ASTModuleListImports(
    const WasmEdge_ASTModuleContext* Cxt,
    const WasmEdge_ImportTypeContext** Imports,
    const uint32_t Len)
{
    return WasmEdge_ASTModuleListImports(Cxt, Imports, Len);
}

uint32_t
WasmEdge2_ASTModuleListExportsLength(const WasmEdge_ASTModuleContext* Cxt)
{
    return WasmEdge_ASTModuleListExportsLength(Cxt);
}

uint32_t
WasmEdge2_ASTModuleListExports(
    const WasmEdge_ASTModuleContext* Cxt,
    const WasmEdge_ExportTypeContext** Exports,
    const uint32_t Len)
{
    return WasmEdge_ASTModuleListExports(Cxt, Exports, Len);
}

// VALIDATOR
WasmEdge_ValidatorContext*
WasmEdge2_ValidatorCreate(const WasmEdge_ConfigureContext* Cxt)
{
    return WasmEdge_ValidatorCreate(Cxt);
}

WasmEdge_Result
WasmEdge2_ValidatorValidate(
    WasmEdge_ValidatorContext* Cxt,
    const WasmEdge_ASTModuleContext* ModuleCxt)
{
    return WasmEdge_ValidatorValidate(Cxt, ModuleCxt);
}

void
WasmEdge2_ValidatorDelete(WasmEdge_ValidatorContext* Cxt)
{
    return WasmEdge_ValidatorDelete(Cxt);
}

// MOD INST
WasmEdge_ModuleInstanceContext*
WasmEdge2_ModuleInstanceCreate(const WasmEdge_String ModuleName)
{
    return WasmEdge_ModuleInstanceCreate(ModuleName);
}

void
WasmEdge2_ModuleInstanceDelete(WasmEdge_ModuleInstanceContext* Cxt)
{
    return WasmEdge_ModuleInstanceDelete(Cxt);
}

uint32_t
WasmEdge2_ModuleInstanceListFunctionLength(
    const WasmEdge_ModuleInstanceContext* Cxt)
{
    return WasmEdge_ModuleInstanceListFunctionLength(Cxt);
}

uint32_t
WasmEdge2_ModuleInstanceListFunction(
    const WasmEdge_ModuleInstanceContext* Cxt,
    WasmEdge_String* Names,
    const uint32_t Len)
{
    return WasmEdge_ModuleInstanceListFunction(Cxt, Names, Len);
}

void
WasmEdge2_ModuleInstanceAddFunction(
    WasmEdge_ModuleInstanceContext* Cxt,
    const WasmEdge_String Name,
    WasmEdge_FunctionInstanceContext* FuncCxt)
{
    return WasmEdge_ModuleInstanceAddFunction(Cxt, Name, FuncCxt);
}

// EXTERN
WasmEdge_MemoryInstanceContext*
WasmEdge2_ModuleInstanceFindMemory(
    const WasmEdge_ModuleInstanceContext* Ctx,
    const WasmEdge_String Name)
{
    return WasmEdge_ModuleInstanceFindMemory(Ctx, Name);
}

WasmEdge_FunctionInstanceContext*
WasmEdge2_ModuleInstanceFindFunction(
    const WasmEdge_ModuleInstanceContext* Cxt,
    const WasmEdge_String Name)
{
    return WasmEdge_ModuleInstanceFindFunction(Cxt, Name);
}

WasmEdge_Result
WasmEdge2_MemoryInstanceSetData(
    WasmEdge_MemoryInstanceContext* Ctx,
    const uint8_t* Data,
    const uint32_t Offset,
    const uint32_t Length)
{
    return WasmEdge_MemoryInstanceSetData(Ctx, Data, Offset, Length);
}

WasmEdge_Result
WasmEdge2_MemoryInstanceGetData(
    const WasmEdge_MemoryInstanceContext* Ctx,
    uint8_t* Data,
    const uint32_t Offset,
    const uint32_t Length)
{
    return WasmEdge_MemoryInstanceGetData(Ctx, Data, Offset, Length);
}

// EXECUTOR
WasmEdge_ExecutorContext*
WasmEdge2_ExecutorCreate(
    const WasmEdge_ConfigureContext* ConfCxt,
    WasmEdge_StatisticsContext* StatCxt)
{
    return WasmEdge_ExecutorCreate(ConfCxt, StatCxt);
}
void
WasmEdge2_ExecutorDelete(WasmEdge_ExecutorContext* Cxt)
{
    return WasmEdge_ExecutorDelete(Cxt);
}

WasmEdge_Result
WasmEdge2_ExecutorInstantiate(
    WasmEdge_ExecutorContext* Cxt,
    WasmEdge_ModuleInstanceContext** ModuleCxt,
    WasmEdge_StoreContext* StoreCxt,
    const WasmEdge_ASTModuleContext* ASTCxt)
{
    return WasmEdge_ExecutorInstantiate(Cxt, ModuleCxt, StoreCxt, ASTCxt);
}

WasmEdge_Result
WasmEdge2_ExecutorInvoke(
    WasmEdge_ExecutorContext* Cxt,
    const WasmEdge_FunctionInstanceContext* FuncCxt,
    const WasmEdge_Value* Params,
    const uint32_t ParamLen,
    WasmEdge_Value* Returns,
    const uint32_t ReturnLen)
{
    return WasmEdge_ExecutorInvoke(
        Cxt, FuncCxt, Params, ParamLen, Returns, ReturnLen);
}

WasmEdge_Result
WasmEdge2_ExecutorRegisterImport(
    WasmEdge_ExecutorContext* Cxt,
    WasmEdge_StoreContext* StoreCxt,
    const WasmEdge_ModuleInstanceContext* ImportCxt)
{
    return WasmEdge_ExecutorRegisterImport(Cxt, StoreCxt, ImportCxt);
}

// ERRORS
bool
WasmEdge2_ResultOK(const WasmEdge_Result Res)
{
    return WasmEdge_ResultOK(Res);
}
const char*
WasmEdge2_ResultGetMessage(const WasmEdge_Result Res)
{
    return WasmEdge_ResultGetMessage(Res);
}

// IMPORT FUNCTIONS
WasmEdge_FunctionTypeContext*
WasmEdge2_FunctionTypeCreate(
    const WasmEdge_ValType* ParamList,
    const uint32_t ParamLen,
    const WasmEdge_ValType* ReturnList,
    const uint32_t ReturnLen)
{
    return WasmEdge_FunctionTypeCreate(
        ParamList, ParamLen, ReturnList, ReturnLen);
}
WasmEdge_FunctionInstanceContext*
WasmEdge2_FunctionInstanceCreate(
    const WasmEdge_FunctionTypeContext* Type,
    WasmEdge_HostFunc_t HostFunc,
    void* Data,
    const uint64_t Cost)
{
    return WasmEdge_FunctionInstanceCreate(Type, HostFunc, Data, Cost);
}
void
WasmEdge2_FunctionTypeDelete(WasmEdge_FunctionTypeContext* Cxt)
{
    return WasmEdge_FunctionTypeDelete(Cxt);
}

WasmEdge_StatisticsContext*
WasmEdge2_StatisticsCreate(void)
{
    return WasmEdge_StatisticsCreate();
}

uint64_t
WasmEdge2_StatisticsGetInstrCount(const WasmEdge_StatisticsContext* Cxt)
{
    return WasmEdge_StatisticsGetInstrCount(Cxt);
}
uint64_t
WasmEdge2_StatisticsGetTotalCost(const WasmEdge_StatisticsContext* Cxt)
{
    return WasmEdge_StatisticsGetTotalCost(Cxt);
}
void
WasmEdge2_StatisticsSetCostTable(
    WasmEdge_StatisticsContext* Cxt,
    uint64_t* CostArr,
    const uint32_t Len)
{
    return WasmEdge_StatisticsSetCostTable(Cxt, CostArr, Len);
}
void
WasmEdge2_StatisticsSetCostLimit(
    WasmEdge_StatisticsContext* Cxt,
    const uint64_t Limit)
{
    return WasmEdge_StatisticsSetCostLimit(Cxt, Limit);
}
void
WasmEdge2_StatisticsClear(WasmEdge_StatisticsContext* Cxt)
{
    return WasmEdge_StatisticsClear(Cxt);
}
void
WasmEdge2_StatisticsDelete(WasmEdge_StatisticsContext* Cxt)
{
    return WasmEdge_StatisticsDelete(Cxt);
}

void
WasmEdge3_PluginLoadWithDefaultPaths(void)
{
    return WasmEdge_PluginLoadWithDefaultPaths();
}
