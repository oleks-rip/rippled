#include "wasmedge_so.h"

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

const WasmEdge_ModuleInstanceContext*
WasmEdge2_VMGetActiveModule(const WasmEdge_VMContext* Ctx)
{
    return WasmEdge_VMGetActiveModule(Ctx);
}
WasmEdge_MemoryInstanceContext*
WasmEdge2_ModuleInstanceFindMemory(
    const WasmEdge_ModuleInstanceContext* Ctx,
    const WasmEdge_String Name)
{
    return WasmEdge_ModuleInstanceFindMemory(Ctx, Name);
}
WasmEdge_ModuleInstanceContext*
WasmEdge2_ModuleInstanceCreate(const WasmEdge_String ModuleName)
{
    return WasmEdge_ModuleInstanceCreate(ModuleName);
}
void
WasmEdge2_ModuleInstanceAddFunction(
    WasmEdge_ModuleInstanceContext* Cxt,
    const WasmEdge_String Name,
    WasmEdge_FunctionInstanceContext* FuncCxt)
{
    return WasmEdge_ModuleInstanceAddFunction(Cxt, Name, FuncCxt);
}
WasmEdge_Result
WasmEdge2_VMRegisterModuleFromImport(
    WasmEdge_VMContext* Cxt,
    const WasmEdge_ModuleInstanceContext* ImportCxt)
{
    return WasmEdge_VMRegisterModuleFromImport(Cxt, ImportCxt);
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
