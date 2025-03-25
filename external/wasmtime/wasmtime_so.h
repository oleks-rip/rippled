#pragma once

#include <wasmtime.h>

#if !defined(wasmtime2_so_EXPORTS) && defined(__cplusplus)
extern "C" {
#endif

wasm_engine_t*
wasmtime2_engine_new(void);

void
wasmtime2_engine_delete(wasm_engine_t* o);

wasm_store_t*
wasmtime2_store_new(wasm_engine_t*);
void
wasmtime2_store_delete(wasm_store_t* o);

wasm_module_t*
wasmtime2_module_new(wasm_store_t*, wasm_byte_vec_t const*);
void
wasmtime2_module_delete(wasm_module_t* o);
void
wasmtime2_module_imports(const wasm_module_t*, wasm_importtype_vec_t* out);
void
wasmtime2_module_exports(const wasm_module_t*, wasm_exporttype_vec_t* out);

wasm_instance_t*
wasmtime2_instance_new(
    wasm_store_t*,
    wasm_module_t const*,
    wasm_extern_vec_t const*,
    wasm_trap_t**);
void
wasmtime2_instance_delete(wasm_instance_t* o);
void
wasmtime2_instance_exports(const wasm_instance_t*, wasm_extern_vec_t* out);

byte_t*
wasmtime2_memory_data(wasm_memory_t*);
size_t
wasmtime2_memory_data_size(const wasm_memory_t*);

void
wasmtime2_exporttype_vec_delete(wasm_exporttype_vec_t*);
void
wasmtime2_extern_vec_delete(wasm_extern_vec_t*);

const wasm_name_t*
wasmtime2_importtype_name(const wasm_importtype_t*);
const wasm_externtype_t*
wasmtime2_importtype_type(const wasm_importtype_t*);

const wasm_name_t*
wasmtime2_exporttype_name(const wasm_exporttype_t*);
const wasm_externtype_t*
wasmtime2_exporttype_type(const wasm_exporttype_t*);

wasm_externkind_t
wasmtime2_externtype_kind(const wasm_externtype_t*);
wasm_externkind_t
wasmtime2_extern_kind(const wasm_extern_t*);
wasm_externtype_t*
wasmtime2_extern_type(const wasm_extern_t*);

wasm_func_t*
wasmtime2_extern_as_func(wasm_extern_t*);
wasm_global_t*
wasmtime2_extern_as_global(wasm_extern_t*);
wasm_table_t*
wasmtime2_extern_as_table(wasm_extern_t*);
wasm_memory_t*
wasmtime2_extern_as_memory(wasm_extern_t*);

wasm_extern_t*
wasmtime2_func_as_extern(wasm_func_t*);
wasm_extern_t*
wasmtime2_global_as_extern(wasm_global_t*);
wasm_extern_t*
wasmtime2_table_as_extern(wasm_table_t*);
wasm_extern_t*
wasmtime2_memory_as_extern(wasm_memory_t*);

void
wasmtime2_val_vec_new_empty(wasm_val_vec_t* out);
void
wasmtime2_val_vec_new_uninitialized(wasm_val_vec_t* out, size_t);
void
wasmtime2_val_vec_new(
    wasm_val_vec_t* out,
    size_t,
    wasm_val_t const* ptr_or_none);
void
wasmtime2_val_vec_copy(wasm_val_vec_t* out, const wasm_val_vec_t*);
void
wasmtime2_val_vec_delete(wasm_val_vec_t*);

void
wasmtime2_byte_vec_delete(wasm_byte_vec_t*);

wasm_valtype_t* wasmtime2_valtype_new(wasm_valkind_t);
void
wasmtime2_valtype_delete(wasm_valtype_t*);

inline wasm_valtype_t*
wasmtime2_valtype_new_i32()
{
    return wasmtime2_valtype_new(WASM_I32);
}
inline wasm_valtype_t*
wasmtime2_valtype_new_i64()
{
    return wasmtime2_valtype_new(WASM_I64);
}
inline wasm_valtype_t*
wasmtime2_valtype_new_f32()
{
    return wasmtime2_valtype_new(WASM_F32);
}
inline wasm_valtype_t*
wasmtime2_valtype_new_f64()
{
    return wasmtime2_valtype_new(WASM_F64);
}
inline wasm_valtype_t*
wasmtime2_valtype_new_externref()
{
    return wasmtime2_valtype_new(WASM_EXTERNREF);
}
inline wasm_valtype_t*
wasmtime2_valtype_new_funcref()
{
    return wasmtime2_valtype_new(WASM_FUNCREF);
}

wasm_functype_t*
wasmtime2_functype_new_0_1(wasm_valtype_t* r);
wasm_functype_t*
wasmtime2_functype_new_1_0(wasm_valtype_t* r);

void
wasmtime2_functype_delete(wasm_functype_t*);

wasm_trap_t*
wasmtime2_func_call(
    const wasm_func_t*,
    const wasm_val_vec_t* args,
    wasm_val_vec_t* results);
wasm_func_t*
wasmtime2_func_new(wasm_store_t*, const wasm_functype_t*, wasm_func_callback_t);
wasm_func_t*
wasmtime2_func_new_with_env(
    wasm_store_t*,
    const wasm_functype_t* type,
    wasm_func_callback_with_env_t,
    void* env,
    void (*finalizer)(void*));
void
wasmtime2_func_delete(wasm_func_t* f);

wasm_functype_t*
wasmtime2_func_type(const wasm_func_t*);

void
wasmtime2_trap_message(const wasm_trap_t*, wasm_message_t* out);
void
wasmtime2_trap_delete(wasm_trap_t*);

wasmtime_store_t*
wasmtime3_store_new(wasm_engine_t* engine, void* data, void (*final)(void*));
wasmtime_context_t*
wasmtime3_store_context(wasmtime_store_t* store);
wasmtime_linker_t*
wasmtime3_linker_new(wasm_engine_t* engine);
wasmtime_error_t*
wasmtime3_linker_define_wasi(wasmtime_linker_t* linker);

#if !defined(wasmtime2_so_EXPORTS) && defined(__cplusplus)
}
#endif
