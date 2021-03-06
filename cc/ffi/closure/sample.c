#include <stdio.h>
#include <ffi.h>

void puts_binding(ffi_cif *cif, unsigned int *ret, void* args[], FILE *stream)
{
	*ret = fputs(*(char **)args[0], stream);
}

int main()
{
	ffi_cif cif;
	ffi_type *args[1];
	ffi_closure *closure;

	int (*bound_puts)(char *);
	int rc;

	closure = ffi_closure_alloc(sizeof(ffi_closure), (void **)&bound_puts);
	if (closure == NULL) {
		return 1;
	}

	args[0] = &ffi_type_pointer;
	if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, &ffi_type_uint, args) != FFI_OK) {
		return 1;
	}

	if (ffi_prep_closure_loc(closure, &cif, puts_binding, stdout, bound_puts) != FFI_OK) {
		return 1;
	}

	rc = bound_puts("Hello World!");

	printf("rc = %d\n", rc);

	ffi_closure_free(closure);

	return 0;
}

