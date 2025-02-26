#pragma once

#include <wasmedge/wasmedge.h>

#if !defined(wasmedge_so_EXPORTS) && defined(__cplusplus)
extern "C" {
#endif

WasmEdge_Value
WasmEdge2_ValueGenI32(const int32_t Val);
WasmEdge_ValType
WasmEdge2_ValTypeGenI32(void);

int32_t
WasmEdge2_ValueGetI32(const WasmEdge_Value Val);

WasmEdge_String
WasmEdge2_StringCreateByCString(const char* Str);
void
WasmEdge2_StringDelete(WasmEdge_String Str);

WasmEdge_VMContext*
WasmEdge2_VMCreate(
    const WasmEdge_ConfigureContext* ConfCtx,
    WasmEdge_StoreContext* StoreCtx);
void
WasmEdge2_VMDelete(WasmEdge_VMContext* Ctx);

const WasmEdge_ModuleInstanceContext*
WasmEdge2_VMGetActiveModule(const WasmEdge_VMContext* Ctx);
WasmEdge_MemoryInstanceContext*
WasmEdge2_ModuleInstanceFindMemory(
    const WasmEdge_ModuleInstanceContext* Ctx,
    const WasmEdge_String Name);
WasmEdge_ModuleInstanceContext*
WasmEdge2_ModuleInstanceCreate(const WasmEdge_String ModuleName);
void
WasmEdge2_ModuleInstanceAddFunction(
    WasmEdge_ModuleInstanceContext* Cxt,
    const WasmEdge_String Name,
    WasmEdge_FunctionInstanceContext* FuncCxt);
WasmEdge_Result
WasmEdge2_VMRegisterModuleFromImport(
    WasmEdge_VMContext* Cxt,
    const WasmEdge_ModuleInstanceContext* ImportCxt);

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

bool
WasmEdge2_ResultOK(const WasmEdge_Result Res);
const char*
WasmEdge2_ResultGetMessage(const WasmEdge_Result Res);

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

#if !defined(wasmedge_so_EXPORTS) && defined(__cplusplus)
}
#endif
