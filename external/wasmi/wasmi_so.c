#include "wasmi_so.h"

wasmi_error_t *wasmi3_error_new(const char *s)
{
    return wasmi_error_new(s);
}

void wasmi3_error_delete( wasmi_error_t *error)
{
    // return wasmi_error_delete(error);
}

void wasmi3_error_message(const wasmi_error_t *error,
                                            wasm_name_t *message)
{
    // return wasmi_error_message(error, message);
}

wasm_config_t*
wasmi2_config_new(void)
{
    return wasm_config_new();
}

void
wasmi2_config_delete(wasm_config_t* c)
{
    return wasm_config_delete(c);
}

void
wasmi3_config_consume_fuel_set(wasm_config_t* c, bool v)
{
    return wasmi_config_consume_fuel_set(c, v);
}
void
wasmi3_config_wasm_sign_extension_set(wasm_config_t* c, bool v)
{
    return wasmi_config_wasm_sign_extension_set(c, v);
}
void
wasmi3_config_wasm_saturating_float_to_int_set(wasm_config_t* c, bool v)
{
    return wasmi_config_wasm_saturating_float_to_int_set(c, v);
}
void
wasmi3_config_wasm_bulk_memory_set(wasm_config_t* c, bool v)
{
    return wasmi_config_wasm_bulk_memory_set(c, v);
}
void
wasmi3_config_wasm_reference_types_set(wasm_config_t* c, bool v)
{
    return wasmi_config_wasm_reference_types_set(c, v);
}
void
wasmi3_config_wasm_tail_call_set(wasm_config_t* c, bool v)
{
    return wasmi_config_wasm_tail_call_set(c, v);
}
void
wasmi3_config_wasm_extended_const_set(wasm_config_t* c, bool v)
{
    return wasmi_config_wasm_extended_const_set(c, v);
}
void
wasmi3_config_floats_set(wasm_config_t* c, bool v)
{
    return wasmi_config_floats_set(c, v);
}
void
wasmi3_config_compilation_mode_set(wasm_config_t* c, bool v)
{
    return wasmi_config_compilation_mode_set(c, v);
}
void
wasmi3_config_ignore_custom_sections_set(wasm_config_t* c, bool v)
{
    return wasmi_config_ignore_custom_sections_set(c, v);
}
void
wasmi3_config_wasm_mutable_globals_set(wasm_config_t* c, bool v)
{
    return wasmi_config_wasm_mutable_globals_set(c, v);
}
void
wasmi3_config_wasm_multi_value_set(wasm_config_t* c, bool v)
{
    return wasmi_config_wasm_multi_value_set(c, v);
}

wasm_engine_t*
wasmi2_engine_new(void)
{
    return wasm_engine_new();
}

wasm_engine_t*
wasmi2_engine_new_with_config(wasm_config_t* c)
{
    return wasm_engine_new_with_config(c);
}

void
wasmi2_engine_delete(wasm_engine_t* o)
{
    return wasm_engine_delete(o);
}

wasm_store_t*
wasmi2_store_new(wasm_engine_t* e)
{
    return wasm_store_new(e);
}

void
wasmi2_store_delete(wasm_store_t* o)
{
    return wasm_store_delete(o);
}

wasmi_store_t*
wasmi3_store_new(wasm_engine_t* e, void *data, void (*finalizer)(void *))
{
    return wasmi_store_new(e, data, finalizer);
}

void
wasmi3_store_delete(wasmi_store_t* o)
{
    return wasmi_store_delete(o);
}

wasm_module_t*
wasmi2_module_new(wasm_store_t* s, wasm_byte_vec_t const* c)
{
    return wasm_module_new(s, c);
}

void
wasmi2_module_delete(wasm_module_t* o)
{
    return wasm_module_delete(o);
}

void
wasmi2_module_imports(const wasm_module_t* m, wasm_importtype_vec_t* out)
{
    return wasm_module_imports(m, out);
}
void
wasmi2_module_exports(const wasm_module_t* m, wasm_exporttype_vec_t* out)
{
    return wasm_module_exports(m, out);
}

wasm_instance_t*
wasmi2_instance_new(
    wasm_store_t* s,
    wasm_module_t const* m,
    wasm_extern_vec_t const* imports,
    wasm_trap_t** t)
{
    return wasm_instance_new(s, m, imports, t);
}
void
wasmi2_instance_delete(wasm_instance_t* o)
{
    return wasm_instance_delete(o);
}

void
wasmi2_instance_exports(const wasm_instance_t* i, wasm_extern_vec_t* out)
{
    return wasm_instance_exports(i, out);
}

byte_t*
wasmi2_memory_data(wasm_memory_t* m)
{
    return wasm_memory_data(m);
}

size_t
wasmi2_memory_data_size(const wasm_memory_t* m)
{
    return wasm_memory_data_size(m);
}

void
wasmi2_exporttype_vec_delete(wasm_exporttype_vec_t* o)
{
    return wasm_exporttype_vec_delete(o);
}
void
wasmi2_extern_vec_delete(wasm_extern_vec_t* o)
{
    return wasm_extern_vec_delete(o);
}

const wasm_name_t*
wasmi2_importtype_name(const wasm_importtype_t* o)
{
    return wasm_importtype_name(o);
}
const wasm_externtype_t*
wasmi2_importtype_type(const wasm_importtype_t* o)
{
    return wasm_importtype_type(o);
}

const wasm_name_t*
wasmi2_exporttype_name(const wasm_exporttype_t* o)
{
    return wasm_exporttype_name(o);
}
const wasm_externtype_t*
wasmi2_exporttype_type(const wasm_exporttype_t* o)
{
    return wasm_exporttype_type(o);
}

wasm_externkind_t
wasmi2_externtype_kind(const wasm_externtype_t* t)
{
    return wasm_externtype_kind(t);
}

wasm_externkind_t
wasmi2_extern_kind(const wasm_extern_t* t)
{
    return wasm_extern_kind(t);
}

wasm_externtype_t*
wasmi2_extern_type(const wasm_extern_t* t)
{
    return wasm_extern_type(t);
}

wasm_func_t*
wasmi2_extern_as_func(wasm_extern_t* t)
{
    return wasm_extern_as_func(t);
}
wasm_global_t*
wasmi2_extern_as_global(wasm_extern_t* t)
{
    return wasm_extern_as_global(t);
}
wasm_table_t*
wasmi2_extern_as_table(wasm_extern_t* t)
{
    return wasm_extern_as_table(t);
}
wasm_memory_t*
wasmi2_extern_as_memory(wasm_extern_t* t)
{
    return wasm_extern_as_memory(t);
}

wasm_extern_t*
wasmi2_func_as_extern(wasm_func_t* t)
{
    return wasm_func_as_extern(t);
}
wasm_extern_t*
wasmi2_global_as_extern(wasm_global_t* t)
{
    return wasm_global_as_extern(t);
}
wasm_extern_t*
wasmi2_table_as_extern(wasm_table_t* t)
{
    return wasm_table_as_extern(t);
}
wasm_extern_t*
wasmi2_memory_as_extern(wasm_memory_t* t)
{
    return wasm_memory_as_extern(t);
}

void
wasmi2_val_vec_new_empty(wasm_val_vec_t* out)
{
    return wasm_val_vec_new_empty(out);
}
void
wasmi2_val_vec_new_uninitialized(wasm_val_vec_t* out, size_t n)
{
    return wasm_val_vec_new_uninitialized(out, n);
}
void
wasmi2_val_vec_new(wasm_val_vec_t* out, size_t n, wasm_val_t const* ptr_or_none)
{
    return wasm_val_vec_new(out, n, ptr_or_none);
}
void
wasmi2_val_vec_copy(wasm_val_vec_t* out, const wasm_val_vec_t* v)
{
    return wasm_val_vec_copy(out, v);
}
void
wasmi2_val_vec_delete(wasm_val_vec_t* v)
{
    return wasm_val_vec_delete(v);
}

void
wasmi2_byte_vec_delete(wasm_byte_vec_t* o)
{
    return wasm_byte_vec_delete(o);
}

wasm_valtype_t*
wasmi2_valtype_new(wasm_valkind_t vk)
{
    return wasm_valtype_new(vk);
}
void
wasmi2_valtype_delete(wasm_valtype_t* t)
{
    return wasm_valtype_delete(t);
}

wasm_functype_t*
wasmi2_functype_new_0_1(wasm_valtype_t* r)
{
    return wasm_functype_new_0_1(r);
}
wasm_functype_t*
wasmi2_functype_new_1_0(wasm_valtype_t* r)
{
    return wasm_functype_new_1_0(r);
}
void
wasmi2_functype_delete(wasm_functype_t* r)
{
    return wasm_functype_delete(r);
}

wasm_trap_t*
wasmi2_func_call(
    const wasm_func_t* f,
    const wasm_val_vec_t* args,
    wasm_val_vec_t* results)
{
    return wasm_func_call(f, args, results);
}

wasm_func_t*
wasmi2_func_new(
    wasm_store_t* s,
    const wasm_functype_t* ft,
    wasm_func_callback_t cb)
{
    return wasm_func_new(s, ft, cb);
}
void
wasmi2_func_delete(wasm_func_t* f)
{
    return wasm_func_delete(f);
}

wasm_func_t*
wasmi2_func_new_with_env(
    wasm_store_t* s,
    const wasm_functype_t* type,
    wasm_func_callback_with_env_t cb,
    void* env,
    void (*finalizer)(void*))
{
    return wasm_func_new_with_env(s, type, cb, env, finalizer);
}

wasm_functype_t*
wasmi2_func_type(const wasm_func_t* f)
{
    return wasm_func_type(f);
}

void
wasmi2_trap_message(const wasm_trap_t* t, wasm_message_t* out)
{
    return wasm_trap_message(t, out);
}

void
wasmi2_trap_delete(wasm_trap_t* t)
{
    return wasm_trap_delete(t);
}


wasmi_context_t*
wasmi3_store_context(wasmi_store_t* store)
{
    return wasmi_store_context(store);
}

wasmi_error_t*
wasmi3_store_set_fuel(wasm_store_t* ctx, uint64_t fuel)
{
    return wasm_store_set_fuel(ctx, fuel);
}

wasmi_error_t*
wasmi3_store_get_fuel( wasm_store_t const* ctx, uint64_t* fuel)
{
    return wasm_store_get_fuel(ctx, fuel);
}
