#include "wasmer_so.h"

wasm_engine_t*
wasmer_engine_new(void)
{
    return wasm_engine_new();
}

void
wasmer_engine_delete(wasm_engine_t* o)
{
    return wasm_engine_delete(o);
}

wasm_store_t*
wasmer_store_new(wasm_engine_t* e)
{
    return wasm_store_new(e);
}

void
wasmer_store_delete(wasm_store_t* o)
{
    return wasm_store_delete(o);
}

wasm_module_t*
wasmer_module_new2(wasm_store_t* s, wasm_byte_vec_t const* c)
{
    return wasm_module_new(s, c);
}

void
wasmer_module_delete(wasm_module_t* o)
{
    return wasm_module_delete(o);
}

void
wasmer_module_imports(const wasm_module_t* m, wasm_importtype_vec_t* out)
{
    return wasm_module_imports(m, out);
}
void
wasmer_module_exports(const wasm_module_t* m, wasm_exporttype_vec_t* out)
{
    return wasm_module_exports(m, out);
}

wasm_instance_t*
wasmer_instance_new(
    wasm_store_t* s,
    wasm_module_t const* m,
    wasm_extern_vec_t const* imports,
    wasm_trap_t** t)
{
    return wasm_instance_new(s, m, imports, t);
}
void
wasmer_instance_delete(wasm_instance_t* o)
{
    return wasm_instance_delete(o);
}

void
wasmer_instance_exports(const wasm_instance_t* i, wasm_extern_vec_t* out)
{
    return wasm_instance_exports(i, out);
}

byte_t*
wasmer_memory_data(wasm_memory_t* m)
{
    return wasm_memory_data(m);
}

size_t
wasmer_memory_data_size(const wasm_memory_t* m)
{
    return wasm_memory_data_size(m);
}

void
wasmer_exporttype_vec_delete(wasm_exporttype_vec_t* o)
{
    return wasm_exporttype_vec_delete(o);
}
void
wasmer_extern_vec_delete(wasm_extern_vec_t* o)
{
    return wasm_extern_vec_delete(o);
}

const wasm_name_t*
wasmer_importtype_name(const wasm_importtype_t* o)
{
    return wasm_importtype_name(o);
}
const wasm_externtype_t*
wasmer_importtype_type(const wasm_importtype_t* o)
{
    return wasm_importtype_type(o);
}

const wasm_name_t*
wasmer_exporttype_name(const wasm_exporttype_t* o)
{
    return wasm_exporttype_name(o);
}
const wasm_externtype_t*
wasmer_exporttype_type(const wasm_exporttype_t* o)
{
    return wasm_exporttype_type(o);
}

wasm_externkind_t
wasmer_externtype_kind(const wasm_externtype_t* t)
{
    return wasm_externtype_kind(t);
}

wasm_externkind_t
wasmer_extern_kind(const wasm_extern_t* t)
{
    return wasm_extern_kind(t);
}

wasm_externtype_t*
wasmer_extern_type(const wasm_extern_t* t)
{
    return wasm_extern_type(t);
}

wasm_func_t*
wasmer_extern_as_func(wasm_extern_t* t)
{
    return wasm_extern_as_func(t);
}
wasm_global_t*
wasmer_extern_as_global(wasm_extern_t* t)
{
    return wasm_extern_as_global(t);
}
wasm_table_t*
wasmer_extern_as_table(wasm_extern_t* t)
{
    return wasm_extern_as_table(t);
}
wasm_memory_t*
wasmer_extern_as_memory(wasm_extern_t* t)
{
    return wasm_extern_as_memory(t);
}

wasm_extern_t*
wasmer_func_as_extern(wasm_func_t* t)
{
    return wasm_func_as_extern(t);
}
wasm_extern_t*
wasmer_global_as_extern(wasm_global_t* t)
{
    return wasm_global_as_extern(t);
}
wasm_extern_t*
wasmer_table_as_extern(wasm_table_t* t)
{
    return wasm_table_as_extern(t);
}
wasm_extern_t*
wasmer_memory_as_extern(wasm_memory_t* t)
{
    return wasm_memory_as_extern(t);
}

void
wasmer_val_vec_new_empty(wasm_val_vec_t* out)
{
    return wasm_val_vec_new_empty(out);
}
void
wasmer_val_vec_new_uninitialized(wasm_val_vec_t* out, size_t n)
{
    return wasm_val_vec_new_uninitialized(out, n);
}
void
wasmer_val_vec_new(wasm_val_vec_t* out, size_t n, wasm_val_t const* ptr_or_none)
{
    return wasm_val_vec_new(out, n, ptr_or_none);
}
void
wasmer_val_vec_copy(wasm_val_vec_t* out, const wasm_val_vec_t* v)
{
    return wasm_val_vec_copy(out, v);
}
void
wasmer_val_vec_delete(wasm_val_vec_t* v)
{
    return wasm_val_vec_delete(v);
}

void
wasmer_byte_vec_delete(wasm_byte_vec_t* o)
{
    return wasm_byte_vec_delete(o);
}

wasm_valtype_t*
wasmer_valtype_new(wasm_valkind_t vk)
{
    return wasm_valtype_new(vk);
}
void
wasmer_valtype_delete(wasm_valtype_t* t)
{
    return wasm_valtype_delete(t);
}

wasm_functype_t*
wasmer_functype_new_0_1(wasm_valtype_t* r)
{
    return wasm_functype_new_0_1(r);
}
void
wasmer_functype_delete(wasm_functype_t* r)
{
    return wasm_functype_delete(r);
}

wasm_trap_t*
wasmer_func_call(
    const wasm_func_t* f,
    const wasm_val_vec_t* args,
    wasm_val_vec_t* results)
{
    return wasm_func_call(f, args, results);
}

wasm_func_t*
wasmer_func_new(
    wasm_store_t* s,
    const wasm_functype_t* ft,
    wasm_func_callback_t cb)
{
    return wasm_func_new(s, ft, cb);
}
void
wasmer_func_delete(wasm_func_t* f)
{
    return wasm_func_delete(f);
}

wasm_func_t*
wasmer_func_new_with_env(
    wasm_store_t* s,
    const wasm_functype_t* type,
    wasm_func_callback_with_env_t cb,
    void* env,
    void (*finalizer)(void*))
{
    return wasm_func_new_with_env(s, type, cb, env, finalizer);
}

wasm_functype_t*
wasmer_func_type(const wasm_func_t* f)
{
    return wasm_func_type(f);
}

void
wasmer_trap_message(const wasm_trap_t* t, wasm_message_t* out)
{
    return wasm_trap_message(t, out);
}

void
wasmer_trap_delete(wasm_trap_t* t)
{
    return wasm_trap_delete(t);
}
