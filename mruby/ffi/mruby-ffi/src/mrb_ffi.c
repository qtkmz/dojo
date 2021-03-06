#include <mruby.h>
#include <mruby/variable.h>
#include <mruby/class.h>
#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/data.h>
#include <mruby/value.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <ffi.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
  void *handle;
} mrb_ffi_dl;

typedef struct {
  void *handle;
} mrb_ffi_func;

static void mrb_ffi_dl_free(mrb_state *mrb, void *p);
static mrb_value mrb_ffi_dl_new(mrb_state *mrb, mrb_value self);
static mrb_value mrb_ffi_dl_get_name(mrb_state *mrb, mrb_value self);
static mrb_value mrb_ffi_dl_find(mrb_state *mrb, mrb_value self);

static void mrb_ffi_func_free(mrb_state *mrb, void *p);
static mrb_value mrb_ffi_func_new(mrb_state *mrb, mrb_value self);
static mrb_value mrb_ffi_func_call(mrb_state *mrb, mrb_value self);
static ffi_type* sym_to_ffi_type(mrb_state *mrb, mrb_sym sym);

static const mrb_data_type mrb_ffi_dl_type = {
  "mrb_ffi_dl", mrb_ffi_dl_free,
};

static const mrb_data_type mrb_ffi_func_type = {
  "mrb_ffi_func", mrb_ffi_func_free,
};

static void
mrb_ffi_dl_free(mrb_state *mrb, void *p) {
  mrb_ffi_dl *dl = (mrb_ffi_dl *)p;

  if (dl->handle != NULL) {
    dlclose(dl->handle);
  }

  free(dl);
}

static mrb_value
mrb_ffi_dl_new(mrb_state *mrb, mrb_value self)
{
  mrb_ffi_dl *dl;
  unsigned char *s;
  mrb_int len;
  mrb_int flags;

  dl = (mrb_ffi_dl *)malloc(sizeof(mrb_ffi_dl));
  if (dl == NULL) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "cannot allocate memory");
  }
  dl->handle = NULL;

  DATA_PTR(self) = dl;
  DATA_TYPE(self) = &mrb_ffi_dl_type;

  mrb_get_args(mrb, "si", &s, &len, &flags);

  dl->handle = dlopen((const char*)s, RTLD_LAZY | RTLD_LOCAL);
  if (dl->handle == NULL) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "cannot find library");
  }
  dlerror();

  mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "@name"), mrb_str_new_cstr(mrb, (const char*)s));

  return self;
}

static mrb_value
mrb_ffi_dl_get_name(mrb_state *mrb, mrb_value self)
{
  mrb_value val = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@name"));

  return val;
}

static mrb_value
mrb_ffi_dl_find(mrb_state *mrb, mrb_value self)
{
  mrb_ffi_dl *dl;
  mrb_value ffi_function;

  dl = mrb_get_datatype(mrb, self, &mrb_ffi_dl_type);
  if (dl == NULL) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid argument");
  }

  {
    mrb_value argv[3];
    const char *func_name;
    void *f;

    mrb_sym name;
    mrb_value ary;
    mrb_sym ret_type;

    struct RClass *ffi;
    struct RClass *func;
    mrb_ffi_func *ffi_func;

    mrb_get_args(mrb, "nAn", &name, &ary, &ret_type);

    func_name = mrb_sym2name(mrb, name);

    {
      char *err_msg;

      f = dlsym(dl->handle, func_name);
      if ((err_msg = dlerror()) != NULL)  {
          printf("cannot find function. name=%s\n", func_name);
          dlerror();
          return mrb_nil_value();
      }
      dlerror();
    }

    argv[0] = mrb_symbol_value(name);
    argv[1] = ary;
    argv[2] = mrb_symbol_value(ret_type);

    ffi = mrb_module_get(mrb, "FFI");
    func = mrb_class_get_under(mrb, ffi, "Function");
    ffi_function = mrb_obj_new(mrb, func, 3, argv);

    ffi_func = mrb_get_datatype(mrb, ffi_function, &mrb_ffi_func_type);

    ffi_func->handle = f;
  }

  return ffi_function;
}

////////

static void
mrb_ffi_func_free(mrb_state *mrb, void *p)
{
  free(p);
}

static mrb_value
mrb_ffi_func_new(mrb_state *mrb, mrb_value self)
{
  mrb_ffi_func *func;

  func = (mrb_ffi_func *)malloc(sizeof(mrb_ffi_func));
  if (func == NULL) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "cannot allocate memory");
  }
  func->handle = NULL;

  DATA_PTR(self) = func;
  DATA_TYPE(self) = &mrb_ffi_func_type;

  {
    mrb_sym name;
    mrb_value arg_type;
    mrb_sym ret_type;

    mrb_get_args(mrb, "nAn", &name, &arg_type, &ret_type);

    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "@name"), mrb_symbol_value(name));
    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "@arg_type"), arg_type);
    mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "@ret_type"), mrb_symbol_value(ret_type));
  }

  return self;
}

static mrb_value
mrb_ffi_func_call(mrb_state *mrb, mrb_value self)
{
  mrb_ffi_func *ffi_func;
  mrb_value arg_type;
  int type_len;
  int rc;

  mrb_value *argv;
  mrb_int argc;

  mrb_get_args(mrb, "*", &argv, &argc);

  ffi_func = mrb_get_datatype(mrb, self, &mrb_ffi_func_type);

  arg_type = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@arg_type"));
  //type_len = mrb_ary_len(mrb, arg_type);
  struct RArray *a = mrb_ary_ptr(arg_type);
  type_len = ARY_LEN(a);

  if (argc < type_len) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "too few arguments");
  } else if (argc > type_len) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "argument line too long");
  }

  {
    ffi_cif cif;
    ffi_type **types;
    mrb_value v;
    void **values;
    void **values_at;

    types = (ffi_type **)malloc(sizeof(ffi_type*) * type_len);
    values = (void **)malloc(sizeof(void *) * type_len);
    values_at = (void **)malloc(sizeof(void *) * type_len);

    for (int i = 0; i < type_len; i++) {
      v = mrb_ary_ref(mrb, arg_type, i);
      types[i] = sym_to_ffi_type(mrb, mrb_symbol(v));

      values_at[i] = mrb_string_value_cstr(mrb, &argv[i]);
      values[i] = &values_at[i];
    }

    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, &ffi_type_uint, types) != FFI_OK) {
      mrb_raise(mrb, E_RUNTIME_ERROR, "cannot execute function");
    }

    ffi_call(&cif, ffi_func->handle, &rc, values);

    free(types);
    free(values);
    free(values_at);
  }

  {
    mrb_value ret_type;
    const char *n;

    ret_type = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@ret_type"));

    n = mrb_sym2name(mrb, mrb_symbol(ret_type));
    if (strcasecmp(n, "int") == 0) {
      return mrb_fixnum_value(rc);
    } else if (strcasecmp(n, "void") == 0) {
      return mrb_fixnum_value(rc);
    } else {
      return mrb_nil_value();
    }
  }
}

static ffi_type*
sym_to_ffi_type(mrb_state *mrb, mrb_sym sym)
{
  const char *name;
  ffi_type *type = NULL;

  name = mrb_sym2name(mrb, sym);
  if (strcasecmp(name, "void") == 0) {
    type = &ffi_type_void;
  } else if (strcasecmp(name, "double") == 0) {
    type = &ffi_type_double;
  } else if (strcasecmp(name, "float") == 0) {
    type = &ffi_type_float;
  } else if (strcasecmp(name, "pointer") == 0) {
    type = &ffi_type_pointer;
  } else if (strcasecmp(name, "schar") == 0) {
    type = &ffi_type_schar;
  } else if (strcasecmp(name, "uchar") == 0) {
    type = &ffi_type_uchar;
  } else if (strcasecmp(name, "sint8") == 0) {
    type = &ffi_type_sint8;
  } else if (strcasecmp(name, "uint8") == 0) {
    type = &ffi_type_uint8;
  } else if (strcasecmp(name, "sint16") == 0) {
    type = &ffi_type_sint16;
  } else if (strcasecmp(name, "uint16") == 0) {
    type = &ffi_type_uint16;
  } else if (strcasecmp(name, "sint32") == 0) {
    type = &ffi_type_sint32;
  } else if (strcasecmp(name, "uint32") == 0) {
    type = &ffi_type_uint32;
  } else if (strcasecmp(name, "sint64") == 0) {
    type = &ffi_type_sint64;
  } else if (strcasecmp(name, "uint64") == 0) {
    type = &ffi_type_uint64;
  } else if (strcasecmp(name, "string") == 0) {
    type = &ffi_type_pointer;
  }
  // ffi_type_complex_double
  // ffi_type_complex_float
  // ffi_type_complex_longdouble
  // ffi_type_longdouble

  return type;
}

static void
retval_to_mrbval(mrb_state *mrb, mrb_value *val)
{
}

////////

static mrb_value
func_ffi_lib(mrb_state *mrb, mrb_value self)
{
  mrb_value o;
  enum mrb_vtype type;

  mrb_get_args(mrb, "o", &o);

  type = mrb_type(o);
  if (type == MRB_TT_SYMBOL) {
  } else if (type == MRB_TT_STRING) {
  }

  return self;
}

static mrb_value
func_attach_function(mrb_state *mrb, mrb_value self)
{
  unsigned char *s;
  mrb_int len;

  mrb_get_args(mrb, "s", &s, &len);

  return self;
}

static mrb_value
func1(mrb_state *mrb, mrb_value self)
{
  mrb_value ary;
  mrb_value v;
  int len;

  mrb_get_args(mrb, "A", &ary);

  //len = mrb_ary_len(mrb, ary);
  struct RArray *a = mrb_ary_ptr(ary);
  len = ARY_LEN(a);

  for (int i = 0; i < len; i++) {
    v = mrb_ary_ref(mrb, ary, i);
  }

  return self;
}

void
mrb_mruby_ffi_gem_init(mrb_state* mrb) {
  struct RClass *ffi;
  struct RClass *library;
  struct RClass *ffi_dl;
  struct RClass *ffi_func;

  ffi = mrb_define_module(mrb, "FFI");
  library = mrb_define_module_under(mrb, ffi, "Library");

  ffi_dl = mrb_define_class_under(mrb, ffi, "DynamicLibrary", mrb->object_class);
  MRB_SET_INSTANCE_TT(ffi_dl, MRB_TT_DATA);

  mrb_define_method(mrb, ffi_dl, "initialize", mrb_ffi_dl_new, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, ffi_dl, "name", mrb_ffi_dl_get_name, MRB_ARGS_NONE());
  mrb_define_method(mrb, ffi_dl, "find", mrb_ffi_dl_find, MRB_ARGS_NONE());

  ffi_func = mrb_define_class_under(mrb, ffi, "Function", mrb->object_class);
  MRB_SET_INSTANCE_TT(ffi_func, MRB_TT_DATA);

  mrb_define_method(mrb, ffi_func, "initialize", mrb_ffi_func_new, MRB_ARGS_REQ(3));
  mrb_define_method(mrb, ffi_func, "call", mrb_ffi_func_call, MRB_ARGS_NONE());


  mrb_define_const(mrb, library, "CURRENT_PROCESS", mrb_symbol_value(mrb_intern_cstr(mrb, "current_process")));

  mrb_define_method(mrb, library, "ffi_lib", func_ffi_lib, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, library, "attach_function", func_attach_function, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, mrb->kernel_module, "foo", func1, MRB_ARGS_REQ(1));
}

void
mrb_mruby_ffi_gem_final(mrb_state* mrb) {
  /* finalizer */
}
