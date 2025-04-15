#pragma once

#include <wasm_c_api.h>
#include <wasm_export.h>

// #pragma GCC visibility push(default)

#if !defined(wamr_so_EXPORTS) && defined(__cplusplus)
extern "C" {
#endif

void
wamr_log_to_rippled(uint32_t logLevel, char const* file, int line, char const* fmt, ...);

wasm_engine_t*
wamr_engine_new(void);

void
wamr_engine_delete(wasm_engine_t* o);

wasm_store_t*
wamr_store_new(wasm_engine_t*);
void
wamr_store_delete(wasm_store_t* o);

wasm_module_t*
wamr_module_new(wasm_store_t*, wasm_byte_vec_t const*);
void
wamr_module_delete(wasm_module_t* o);
void
wamr_module_imports(const wasm_module_t*, wasm_importtype_vec_t* out);
void
wamr_module_exports(const wasm_module_t*, wasm_exporttype_vec_t* out);

wasm_instance_t*
wamr_instance_new(
    wasm_store_t*,
    wasm_module_t const*,
    wasm_extern_vec_t const*,
    wasm_trap_t**);
void
wamr_instance_delete(wasm_instance_t* o);
void
wamr_instance_exports(const wasm_instance_t*, wasm_extern_vec_t* out);

byte_t*
wamr_memory_data(wasm_memory_t*);
size_t
wamr_memory_data_size(const wasm_memory_t*);

void
wamr_exporttype_vec_delete(wasm_exporttype_vec_t*);
void
wamr_extern_vec_delete(wasm_extern_vec_t*);

const wasm_name_t*
wamr_importtype_name(const wasm_importtype_t*);
const wasm_externtype_t*
wamr_importtype_type(const wasm_importtype_t*);

const wasm_name_t*
wamr_exporttype_name(const wasm_exporttype_t*);
const wasm_externtype_t*
wamr_exporttype_type(const wasm_exporttype_t*);

wasm_externkind_t
wamr_externtype_kind(const wasm_externtype_t*);
wasm_externkind_t
wamr_extern_kind(const wasm_extern_t*);
wasm_externtype_t*
wamr_extern_type(const wasm_extern_t*);

wasm_func_t*
wamr_extern_as_func(wasm_extern_t*);
wasm_global_t*
wamr_extern_as_global(wasm_extern_t*);
wasm_table_t*
wamr_extern_as_table(wasm_extern_t*);
wasm_memory_t*
wamr_extern_as_memory(wasm_extern_t*);

wasm_extern_t*
wamr_func_as_extern(wasm_func_t*);
wasm_extern_t*
wamr_global_as_extern(wasm_global_t*);
wasm_extern_t*
wamr_table_as_extern(wasm_table_t*);
wasm_extern_t*
wamr_memory_as_extern(wasm_memory_t*);

void
wamr_val_vec_new_empty(wasm_val_vec_t* out);
void
wamr_val_vec_new_uninitialized(wasm_val_vec_t* out, size_t);
void
wamr_val_vec_new(wasm_val_vec_t* out, size_t, wasm_val_t const* ptr_or_none);
void
wamr_val_vec_copy(wasm_val_vec_t* out, const wasm_val_vec_t*);
void
wamr_val_vec_delete(wasm_val_vec_t*);

void
wamr_byte_vec_delete(wasm_byte_vec_t*);

wasm_valtype_t* wamr_valtype_new(wasm_valkind_t);
void
wamr_valtype_delete(wasm_valtype_t*);

inline wasm_valtype_t*
wamr_valtype_new_i32()
{
    return wamr_valtype_new(WASM_I32);
}
inline wasm_valtype_t*
wamr_valtype_new_i64()
{
    return wamr_valtype_new(WASM_I64);
}
inline wasm_valtype_t*
wamr_valtype_new_f32()
{
    return wamr_valtype_new(WASM_F32);
}
inline wasm_valtype_t*
wamr_valtype_new_f64()
{
    return wamr_valtype_new(WASM_F64);
}
inline wasm_valtype_t*
wamr_valtype_new_externref()
{
    return wamr_valtype_new(WASM_EXTERNREF);
}
inline wasm_valtype_t*
wamr_valtype_new_funcref()
{
    return wamr_valtype_new(WASM_FUNCREF);
}

wasm_functype_t*
wamr_functype_new_0_1(wasm_valtype_t* r);
void
wamr_functype_delete(wasm_functype_t*);

wasm_trap_t*
wamr_func_call(
    const wasm_func_t*,
    const wasm_val_vec_t* args,
    wasm_val_vec_t* results);
wasm_func_t*
wamr_func_new(wasm_store_t*, const wasm_functype_t*, wasm_func_callback_t);
wasm_func_t*
wamr_func_new_with_env(
    wasm_store_t*,
    const wasm_functype_t* type,
    wasm_func_callback_with_env_t,
    void* env,
    void (*finalizer)(void*));
void
wamr_func_delete(wasm_func_t* f);

wasm_functype_t*
wamr_func_type(const wasm_func_t*);

void
wamr_trap_message(const wasm_trap_t*, wasm_message_t* out);
void
wamr_trap_delete(wasm_trap_t*);

bool
wamr_runtime_set_default_running_mode(RunningMode running_mode);
bool
wamr_runtime_set_running_mode(
    wasm_module_inst_t module_inst,
    RunningMode running_mode);
RunningMode
wamr_runtime_get_running_mode(wasm_module_inst_t module_inst);

void
wamr_runtime_set_log_level(log_level_t level);

wasm_exec_env_t
wamr_instance_exec_env(const wasm_instance_t*);
void
wamr_runtime_set_instruction_count_limit(
    wasm_exec_env_t exec_env,
    int64_t instruction_count);
int64_t
wamr_runtime_get_instruction_count_limit(wasm_exec_env_t exec_env);

#if !defined(wamr_so_EXPORTS) && defined(__cplusplus)
}
#endif

// #pragma GCC visibility pop
