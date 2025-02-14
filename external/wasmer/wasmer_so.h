#pragma once

#include <wasmer.h>

#if !defined(wasmer_so_EXPORTS) && defined(__cplusplus)
extern "C" {
#endif

wasm_engine_t*
wasmer_engine_new(void);

void
wasmer_engine_delete(wasm_engine_t* o);

wasm_store_t*
wasmer_store_new(wasm_engine_t*);
void
wasmer_store_delete(wasm_store_t* o);

wasm_module_t*
wasmer_module_new2(wasm_store_t*, wasm_byte_vec_t const*);
void
wasmer_module_delete(wasm_module_t* o);
void
wasmer_module_imports(const wasm_module_t*, wasm_importtype_vec_t* out);
void
wasmer_module_exports(const wasm_module_t*, wasm_exporttype_vec_t* out);

wasm_instance_t*
wasmer_instance_new(
    wasm_store_t*,
    wasm_module_t const*,
    wasm_extern_vec_t const*,
    wasm_trap_t**);
void
wasmer_instance_delete(wasm_instance_t* o);
void
wasmer_instance_exports(const wasm_instance_t*, wasm_extern_vec_t* out);

byte_t*
wasmer_memory_data(wasm_memory_t*);
size_t
wasmer_memory_data_size(const wasm_memory_t*);

void
wasmer_exporttype_vec_delete(wasm_exporttype_vec_t*);
void
wasmer_extern_vec_delete(wasm_extern_vec_t*);

const wasm_name_t*
wasmer_importtype_name(const wasm_importtype_t*);
const wasm_externtype_t*
wasmer_importtype_type(const wasm_importtype_t*);

const wasm_name_t*
wasmer_exporttype_name(const wasm_exporttype_t*);
const wasm_externtype_t*
wasmer_exporttype_type(const wasm_exporttype_t*);

wasm_externkind_t
wasmer_externtype_kind(const wasm_externtype_t*);
wasm_externkind_t
wasmer_extern_kind(const wasm_extern_t*);
wasm_externtype_t*
wasmer_extern_type(const wasm_extern_t*);

wasm_func_t*
wasmer_extern_as_func(wasm_extern_t*);
wasm_global_t*
wasmer_extern_as_global(wasm_extern_t*);
wasm_table_t*
wasmer_extern_as_table(wasm_extern_t*);
wasm_memory_t*
wasmer_extern_as_memory(wasm_extern_t*);

wasm_extern_t*
wasmer_func_as_extern(wasm_func_t*);
wasm_extern_t*
wasmer_global_as_extern(wasm_global_t*);
wasm_extern_t*
wasmer_table_as_extern(wasm_table_t*);
wasm_extern_t*
wasmer_memory_as_extern(wasm_memory_t*);

void
wasmer_val_vec_new_empty(wasm_val_vec_t* out);
void
wasmer_val_vec_new_uninitialized(wasm_val_vec_t* out, size_t);
void
wasmer_val_vec_new(wasm_val_vec_t* out, size_t, wasm_val_t const* ptr_or_none);
void
wasmer_val_vec_copy(wasm_val_vec_t* out, const wasm_val_vec_t*);
void
wasmer_val_vec_delete(wasm_val_vec_t*);

void
wasmer_byte_vec_delete(wasm_byte_vec_t*);

wasm_valtype_t* wasmer_valtype_new(wasm_valkind_t);
void
wasmer_valtype_delete(wasm_valtype_t*);

inline wasm_valtype_t*
wasmer_valtype_new_i32()
{
    return wasmer_valtype_new(WASM_I32);
}
inline wasm_valtype_t*
wasmer_valtype_new_i64()
{
    return wasmer_valtype_new(WASM_I64);
}
inline wasm_valtype_t*
wasmer_valtype_new_f32()
{
    return wasmer_valtype_new(WASM_F32);
}
inline wasm_valtype_t*
wasmer_valtype_new_f64()
{
    return wasmer_valtype_new(WASM_F64);
}
inline wasm_valtype_t*
wasmer_valtype_new_externref()
{
    return wasmer_valtype_new(WASM_EXTERNREF);
}
inline wasm_valtype_t*
wasmer_valtype_new_funcref()
{
    return wasmer_valtype_new(WASM_FUNCREF);
}

wasm_functype_t*
wasmer_functype_new_0_1(wasm_valtype_t* r);
void
wasmer_functype_delete(wasm_functype_t*);

wasm_trap_t*
wasmer_func_call(
    const wasm_func_t*,
    const wasm_val_vec_t* args,
    wasm_val_vec_t* results);
wasm_func_t*
wasmer_func_new(wasm_store_t*, const wasm_functype_t*, wasm_func_callback_t);
wasm_func_t*
wasmer_func_new_with_env(
    wasm_store_t*,
    const wasm_functype_t* type,
    wasm_func_callback_with_env_t,
    void* env,
    void (*finalizer)(void*));
void
wasmer_func_delete(wasm_func_t* f);

wasm_functype_t*
wasmer_func_type(const wasm_func_t*);

void
wasmer_trap_message(const wasm_trap_t*, wasm_message_t* out);
void
wasmer_trap_delete(wasm_trap_t*);

#if !defined(wasmer_so_EXPORTS) && defined(__cplusplus)
}
#endif
