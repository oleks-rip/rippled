#include "wasmtime_so.h"

wasm_config_t*
wasmtime2_config_new(void)
{
    return wasm_config_new();
}

void
wasmtime2_config_delete(wasm_config_t* c)
{
    return wasm_config_delete(c);
}

void
wasmtime3_config_consume_fuel_set(wasm_config_t* c, bool v)
{
    return wasmtime_config_consume_fuel_set(c, v);
}

wasm_engine_t*
wasmtime2_engine_new(void)
{
    return wasm_engine_new();
}

wasm_engine_t*
wasmtime2_engine_new_with_config(wasm_config_t* c)
{
    return wasm_engine_new_with_config(c);
}

void
wasmtime2_engine_delete(wasm_engine_t* o)
{
    return wasm_engine_delete(o);
}

wasm_store_t*
wasmtime2_store_new(wasm_engine_t* e)
{
    return wasm_store_new(e);
}

void
wasmtime2_store_delete(wasm_store_t* o)
{
    return wasm_store_delete(o);
}

wasmtime_store_t*
wasmtime3_store_new(wasm_engine_t* e, void* d, void (*fin)(void*))
{
    return wasmtime_store_new(e, d, fin);
}

void
wasmtime3_store_delete(wasmtime_store_t* s)
{
    return wasmtime_store_delete(s);
}

wasmtime_context_t*
wasmtime3_store_context(wasmtime_store_t* s)
{
    return wasmtime_store_context(s);
}

wasm_module_t*
wasmtime2_module_new(wasm_store_t* s, wasm_byte_vec_t const* c)
{
    return wasm_module_new(s, c);
}

wasmtime_error_t*
wasmtime3_module_new(
    wasm_engine_t* e,
    const uint8_t* wasm,
    size_t sz,
    wasmtime_module_t** m)
{
    return wasmtime_module_new(e, wasm, sz, m);
}

void
wasmtime2_module_delete(wasm_module_t* o)
{
    return wasm_module_delete(o);
}

void
wasmtime3_module_delete(wasmtime_module_t* m)
{
    return wasmtime_module_delete(m);
}

void
wasmtime2_module_imports(const wasm_module_t* m, wasm_importtype_vec_t* out)
{
    return wasm_module_imports(m, out);
}
void
wasmtime2_module_exports(const wasm_module_t* m, wasm_exporttype_vec_t* out)
{
    return wasm_module_exports(m, out);
}

void
wasmtime3_module_imports(
    const wasmtime_module_t* m,
    wasm_importtype_vec_t* out)
{
    return wasmtime_module_imports(m, out);
}

wasm_instance_t*
wasmtime2_instance_new(
    wasm_store_t* s,
    wasm_module_t const* m,
    wasm_extern_vec_t const* imports,
    wasm_trap_t** t)
{
    return wasm_instance_new(s, m, imports, t);
}

wasmtime_error_t*
wasmtime3_instance_new(
    wasmtime_context_t* ct,
    const wasmtime_module_t* m,
    const wasmtime_extern_t* imp,
    size_t sz,
    wasmtime_instance_t* i,
    wasm_trap_t** trap)
{
    return wasmtime_instance_new(ct, m, imp, sz, i, trap);
}

void
wasmtime2_instance_delete(wasm_instance_t* o)
{
    return wasm_instance_delete(o);
}

void
wasmtime2_instance_exports(const wasm_instance_t* i, wasm_extern_vec_t* out)
{
    return wasm_instance_exports(i, out);
}

bool
wasmtime3_instance_export_get(
    wasmtime_context_t* ct,
    const wasmtime_instance_t* i,
    const char* n,
    size_t sz,
    wasmtime_extern_t* item)
{
    return wasmtime_instance_export_get(ct, i, n, sz, item);
}

bool
wasmtime3_instance_export_nth(
    wasmtime_context_t* ct,
    const wasmtime_instance_t* i,
    size_t idx,
    char** name,
    size_t* sz,
    wasmtime_extern_t* item)
{
    return wasmtime_instance_export_nth(ct, i, idx, name, sz, item);
}

byte_t*
wasmtime2_memory_data(wasm_memory_t* m)
{
    return wasm_memory_data(m);
}

size_t
wasmtime2_memory_data_size(const wasm_memory_t* m)
{
    return wasm_memory_data_size(m);
}

uint8_t*
wasmtime3_memory_data(const wasmtime_context_t* ct, const wasmtime_memory_t* m)
{
    return wasmtime_memory_data(ct, m);
}

// the byte length
size_t
wasmtime3_memory_data_size(
    const wasmtime_context_t* ct,
    const wasmtime_memory_t* m)
{
    return wasmtime_memory_data_size(ct, m);
}

// WebAssembly pages
uint64_t
wasmtime3_memory_size(const wasmtime_context_t* ct, const wasmtime_memory_t* m)
{
    return wasmtime_memory_size(ct, m);
}

void
wasmtime2_exporttype_vec_delete(wasm_exporttype_vec_t* o)
{
    return wasm_exporttype_vec_delete(o);
}
void
wasmtime2_extern_vec_delete(wasm_extern_vec_t* o)
{
    return wasm_extern_vec_delete(o);
}

const wasm_name_t*
wasmtime2_importtype_name(const wasm_importtype_t* o)
{
    return wasm_importtype_name(o);
}
const wasm_externtype_t*
wasmtime2_importtype_type(const wasm_importtype_t* o)
{
    return wasm_importtype_type(o);
}

const wasm_name_t*
wasmtime2_exporttype_name(const wasm_exporttype_t* o)
{
    return wasm_exporttype_name(o);
}
const wasm_externtype_t*
wasmtime2_exporttype_type(const wasm_exporttype_t* o)
{
    return wasm_exporttype_type(o);
}

wasm_externkind_t
wasmtime2_externtype_kind(const wasm_externtype_t* t)
{
    return wasm_externtype_kind(t);
}

wasm_externkind_t
wasmtime2_extern_kind(const wasm_extern_t* t)
{
    return wasm_extern_kind(t);
}

wasm_externtype_t*
wasmtime2_extern_type(const wasm_extern_t* t)
{
    return wasm_extern_type(t);
}

wasm_func_t*
wasmtime2_extern_as_func(wasm_extern_t* t)
{
    return wasm_extern_as_func(t);
}
wasm_global_t*
wasmtime2_extern_as_global(wasm_extern_t* t)
{
    return wasm_extern_as_global(t);
}
wasm_table_t*
wasmtime2_extern_as_table(wasm_extern_t* t)
{
    return wasm_extern_as_table(t);
}
wasm_memory_t*
wasmtime2_extern_as_memory(wasm_extern_t* t)
{
    return wasm_extern_as_memory(t);
}

wasm_extern_t*
wasmtime2_func_as_extern(wasm_func_t* t)
{
    return wasm_func_as_extern(t);
}
wasm_extern_t*
wasmtime2_global_as_extern(wasm_global_t* t)
{
    return wasm_global_as_extern(t);
}
wasm_extern_t*
wasmtime2_table_as_extern(wasm_table_t* t)
{
    return wasm_table_as_extern(t);
}
wasm_extern_t*
wasmtime2_memory_as_extern(wasm_memory_t* t)
{
    return wasm_memory_as_extern(t);
}

void
wasmtime2_val_vec_new_empty(wasm_val_vec_t* out)
{
    return wasm_val_vec_new_empty(out);
}
void
wasmtime2_val_vec_new_uninitialized(wasm_val_vec_t* out, size_t n)
{
    return wasm_val_vec_new_uninitialized(out, n);
}
void
wasmtime2_val_vec_new(
    wasm_val_vec_t* out,
    size_t n,
    wasm_val_t const* ptr_or_none)
{
    return wasm_val_vec_new(out, n, ptr_or_none);
}
void
wasmtime2_val_vec_copy(wasm_val_vec_t* out, const wasm_val_vec_t* v)
{
    return wasm_val_vec_copy(out, v);
}
void
wasmtime2_val_vec_delete(wasm_val_vec_t* v)
{
    return wasm_val_vec_delete(v);
}

void
wasmtime2_byte_vec_delete(wasm_byte_vec_t* o)
{
    return wasm_byte_vec_delete(o);
}

wasm_valtype_t*
wasmtime2_valtype_new(wasm_valkind_t vk)
{
    return wasm_valtype_new(vk);
}
void
wasmtime2_valtype_delete(wasm_valtype_t* t)
{
    return wasm_valtype_delete(t);
}

wasm_functype_t*
wasmtime2_functype_new_0_1(wasm_valtype_t* r)
{
    return wasm_functype_new_0_1(r);
}

wasm_functype_t*
wasmtime2_functype_new_1_0(wasm_valtype_t* r)
{
    return wasm_functype_new_1_0(r);
}

void
wasmtime2_functype_delete(wasm_functype_t* r)
{
    return wasm_functype_delete(r);
}

wasm_trap_t*
wasmtime2_func_call(
    const wasm_func_t* f,
    const wasm_val_vec_t* args,
    wasm_val_vec_t* results)
{
    return wasm_func_call(f, args, results);
}

wasm_func_t*
wasmtime2_func_new(
    wasm_store_t* s,
    const wasm_functype_t* ft,
    wasm_func_callback_t cb)
{
    return wasm_func_new(s, ft, cb);
}
void
wasmtime2_func_delete(wasm_func_t* f)
{
    return wasm_func_delete(f);
}

wasm_func_t*
wasmtime2_func_new_with_env(
    wasm_store_t* s,
    const wasm_functype_t* type,
    wasm_func_callback_with_env_t cb,
    void* env,
    void (*finalizer)(void*))
{
    return wasm_func_new_with_env(s, type, cb, env, finalizer);
}

void
wasmtime3_func_new(
    wasmtime_context_t* ct,
    const wasm_functype_t* tp,
    wasmtime_func_callback_t cb,
    void* env,
    void (*fin)(void*),
    wasmtime_func_t* ret)
{
    return wasmtime_func_new(ct, tp, cb, env, fin, ret);
}

wasmtime_error_t*
wasmtime3_func_call(
    wasmtime_context_t* ct,
    const wasmtime_func_t* f,
    const wasmtime_val_t* a,
    size_t na,
    wasmtime_val_t* r,
    size_t nr,
    wasm_trap_t** trap)
{
    return wasmtime_func_call(ct, f, a, na, r, nr, trap);
}

wasm_functype_t*
wasmtime2_func_type(const wasm_func_t* f)
{
    return wasm_func_type(f);
}

void
wasmtime2_trap_message(const wasm_trap_t* t, wasm_message_t* out)
{
    return wasm_trap_message(t, out);
}

void
wasmtime2_trap_delete(wasm_trap_t* t)
{
    return wasm_trap_delete(t);
}

wasmtime_error_t*
wasmtime3_context_set_fuel(wasmtime_context_t* ct, uint64_t f)
{
    return wasmtime_context_set_fuel(ct, f);
}
wasmtime_error_t*
wasmtime3_context_get_fuel(const wasmtime_context_t* ct, uint64_t* f)
{
    return wasmtime_context_get_fuel(ct, f);
}
