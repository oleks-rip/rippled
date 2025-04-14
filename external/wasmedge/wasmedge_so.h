#pragma once

#include <wasmedge/wasmedge.h>

#if !defined(wasmedge_so_EXPORTS) && defined(__cplusplus)
extern "C" {
#endif

// VALUES
WasmEdge_Value
WasmEdge2_ValueGenI32(const int32_t Val);
WasmEdge_ValType
WasmEdge2_ValTypeGenI32(void);
int32_t
WasmEdge2_ValueGetI32(const WasmEdge_Value Val);

WasmEdge_Value
WasmEdge2_ValueGenI64(const int64_t Val);
WasmEdge_ValType
WasmEdge2_ValTypeGenI64(void);
int64_t
WasmEdge2_ValueGetI64(const WasmEdge_Value Val);
bool
WasmEdge2_ValTypeIsI64(const WasmEdge_ValType ValType);

WasmEdge_String
WasmEdge2_StringCreateByCString(const char* Str);
void
WasmEdge2_StringDelete(WasmEdge_String Str);

// CONFIG
WasmEdge_ConfigureContext*
WasmEdge2_ConfigureCreate(void);
void
WasmEdge2_ConfigureDelete(WasmEdge_ConfigureContext* Ctx);
void
WasmEdge2_ConfigureAddHostRegistration(
    WasmEdge_ConfigureContext* Cxt,
    const enum WasmEdge_HostRegistration Host);
void
WasmEdge2_ConfigureStatisticsSetInstructionCounting(
    WasmEdge_ConfigureContext* Cxt,
    const bool IsCount);
void
WasmEdge2_ConfigureStatisticsSetCostMeasuring(
    WasmEdge_ConfigureContext* Cxt,
    const bool IsMeasure);

// STORE
WasmEdge_StoreContext*
WasmEdge2_StoreCreate();
void
WasmEdge2_StoreDelete(WasmEdge_StoreContext* Ctx);

// LOADER
WasmEdge_LoaderContext*
WasmEdge2_LoaderCreate(const WasmEdge_ConfigureContext* ConfCxt);
void
WasmEdge2_LoaderDelete(WasmEdge_LoaderContext* Ctx);

// VM
WasmEdge_VMContext*
WasmEdge2_VMCreate(
    const WasmEdge_ConfigureContext* ConfCtx,
    WasmEdge_StoreContext* StoreCtx);
void
WasmEdge2_VMDelete(WasmEdge_VMContext* Ctx);
WasmEdge_Result
WasmEdge2_VMLoadWasmFromBuffer(
    WasmEdge_VMContext* Ctx,
    const uint8_t* Buf,
    const uint32_t BufLen);
WasmEdge_Result
WasmEdge2_VMRunWasmFromBuffer(
    WasmEdge_VMContext* Ctx,
    const uint8_t* Buf,
    const uint32_t BufLen,
    const WasmEdge_String FuncName,
    const WasmEdge_Value* Params,
    const uint32_t ParamLen,
    WasmEdge_Value* Returns,
    const uint32_t ReturnLen);
WasmEdge_Result
WasmEdge2_VMExecute(
    WasmEdge_VMContext* Ctx,
    const WasmEdge_String FuncName,
    const WasmEdge_Value* Params,
    const uint32_t ParamLen,
    WasmEdge_Value* Returns,
    const uint32_t ReturnLen);
WasmEdge_Result
WasmEdge2_VMValidate(WasmEdge_VMContext* Ctx);
WasmEdge_Result
WasmEdge2_VMInstantiate(WasmEdge_VMContext* Ctx);
WasmEdge_Result
WasmEdge2_VMRegisterModuleFromImport(
    WasmEdge_VMContext* Cxt,
    const WasmEdge_ModuleInstanceContext* ImportCxt);
const WasmEdge_ModuleInstanceContext*
WasmEdge2_VMGetActiveModule(const WasmEdge_VMContext* Ctx);

// MODULE
WasmEdge_Result
WasmEdge2_LoaderParseFromBuffer(
    WasmEdge_LoaderContext* Cxt,
    WasmEdge_ASTModuleContext** Module,
    const uint8_t* Buf,
    const uint32_t BufLen);
void
WasmEdge2_ASTModuleDelete(WasmEdge_ASTModuleContext* Cxt);
uint32_t
WasmEdge2_ASTModuleListImportsLength(const WasmEdge_ASTModuleContext* Cxt);
uint32_t
WasmEdge2_ASTModuleListImports(
    const WasmEdge_ASTModuleContext* Cxt,
    const WasmEdge_ImportTypeContext** Imports,
    const uint32_t Len);
uint32_t
WasmEdge2_ASTModuleListExportsLength(const WasmEdge_ASTModuleContext* Cxt);
uint32_t
WasmEdge2_ASTModuleListExports(
    const WasmEdge_ASTModuleContext* Cxt,
    const WasmEdge_ExportTypeContext** Exports,
    const uint32_t Len);

// VALIDATOR
WasmEdge_ValidatorContext*
WasmEdge2_ValidatorCreate(const WasmEdge_ConfigureContext* ConfCxt);
WasmEdge_Result
WasmEdge2_ValidatorValidate(
    WasmEdge_ValidatorContext* Cxt,
    const WasmEdge_ASTModuleContext* ModuleCxt);
void
WasmEdge2_ValidatorDelete(WasmEdge_ValidatorContext* Cxt);

// MOD INST
WasmEdge_ModuleInstanceContext*
WasmEdge2_ModuleInstanceCreate(const WasmEdge_String ModuleName);
void
WasmEdge2_ModuleInstanceDelete(WasmEdge_ModuleInstanceContext* Cxt);
uint32_t
WasmEdge2_ModuleInstanceListFunctionLength(
    const WasmEdge_ModuleInstanceContext* Cxt);
uint32_t
WasmEdge2_ModuleInstanceListFunction(
    const WasmEdge_ModuleInstanceContext* Cxt,
    WasmEdge_String* Names,
    const uint32_t Len);
void
WasmEdge2_ModuleInstanceAddFunction(
    WasmEdge_ModuleInstanceContext* Cxt,
    const WasmEdge_String Name,
    WasmEdge_FunctionInstanceContext* FuncCxt);

// EXTERN
WasmEdge_MemoryInstanceContext*
WasmEdge2_ModuleInstanceFindMemory(
    const WasmEdge_ModuleInstanceContext* Ctx,
    const WasmEdge_String Name);
WasmEdge_FunctionInstanceContext*
WasmEdge2_ModuleInstanceFindFunction(
    const WasmEdge_ModuleInstanceContext* Cxt,
    const WasmEdge_String Name);
WasmEdge_Result
WasmEdge2_MemoryInstanceSetData(
    WasmEdge_MemoryInstanceContext* Ctx,
    const uint8_t* Data,
    const uint32_t Offset,
    const uint32_t Length);
WasmEdge_Result
WasmEdge2_MemoryInstanceGetData(
    const WasmEdge_MemoryInstanceContext* Ctx,
    uint8_t* Data,
    const uint32_t Offset,
    const uint32_t Length);

// EXECUTOR
WasmEdge_ExecutorContext*
WasmEdge2_ExecutorCreate(
    const WasmEdge_ConfigureContext* ConfCxt,
    WasmEdge_StatisticsContext* StatCxt);
void
WasmEdge2_ExecutorDelete(WasmEdge_ExecutorContext* Cxt);
WasmEdge_Result
WasmEdge2_ExecutorInstantiate(
    WasmEdge_ExecutorContext* Cxt,
    WasmEdge_ModuleInstanceContext** ModuleCxt,
    WasmEdge_StoreContext* StoreCxt,
    const WasmEdge_ASTModuleContext* ASTCxt);
WasmEdge_Result
WasmEdge2_ExecutorInvoke(
    WasmEdge_ExecutorContext* Cxt,
    const WasmEdge_FunctionInstanceContext* FuncCxt,
    const WasmEdge_Value* Params,
    const uint32_t ParamLen,
    WasmEdge_Value* Returns,
    const uint32_t ReturnLen);
WasmEdge_Result
WasmEdge2_ExecutorRegisterImport(
    WasmEdge_ExecutorContext* Cxt,
    WasmEdge_StoreContext* StoreCxt,
    const WasmEdge_ModuleInstanceContext* ImportCxt);

// ERRORS
bool
WasmEdge2_ResultOK(const WasmEdge_Result Res);
const char*
WasmEdge2_ResultGetMessage(const WasmEdge_Result Res);

// IMPORT FUNCTIONS
WasmEdge_FunctionTypeContext*
WasmEdge2_FunctionTypeCreate(
    const WasmEdge_ValType* ParamList,
    const uint32_t ParamLen,
    const WasmEdge_ValType* ReturnList,
    const uint32_t ReturnLen);
WasmEdge_FunctionInstanceContext*
WasmEdge2_FunctionInstanceCreate(
    const WasmEdge_FunctionTypeContext* Type,
    WasmEdge_HostFunc_t HostFunc,
    void* Data,
    const uint64_t Cost);
void
WasmEdge2_FunctionTypeDelete(WasmEdge_FunctionTypeContext* Cxt);

WasmEdge_StatisticsContext*
WasmEdge2_StatisticsCreate(void);
uint64_t
WasmEdge2_StatisticsGetInstrCount(const WasmEdge_StatisticsContext* Cxt);
uint64_t
WasmEdge2_StatisticsGetTotalCost(const WasmEdge_StatisticsContext* Cxt);
void
WasmEdge2_StatisticsSetCostTable(
    WasmEdge_StatisticsContext* Cxt,
    uint64_t* CostArr,
    const uint32_t Len);
void
WasmEdge2_StatisticsSetCostLimit(
    WasmEdge_StatisticsContext* Cxt,
    const uint64_t Limit);
void
WasmEdge2_StatisticsClear(WasmEdge_StatisticsContext* Cxt);
void
WasmEdge2_StatisticsDelete(WasmEdge_StatisticsContext* Cxt);

void
WasmEdge3_PluginLoadWithDefaultPaths(void);

#if !defined(wasmedge_so_EXPORTS) && defined(__cplusplus)
}
#endif
