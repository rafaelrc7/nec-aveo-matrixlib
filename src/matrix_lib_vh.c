#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ve_offload.h>

#include "matrix_lib.h"

static int _ve_num_node = 0;
static int _ve_num_threads = 1;
static uint64_t _ve_lib_handle = 0;
static struct veo_proc_handle *_ve_proc = NULL;
static struct veo_thr_ctxt *_veo_ctxt = NULL;
static struct veo_args *_veo_argp = NULL;

static const char *_ve_lib_path = "./matrix_lib_ve.so";
static const char *_lib_scalar_matrix_mult = "scalar_matrix_mult";
static const char *_lib_matrix_matrix_mult = "matrix_matrix_mult";

int scalar_matrix_mult(float scalar_value, struct matrix *matrix)
{
	uint64_t veo_call_handle, veo_ret;
	int ret;

	if (!_ve_proc)
		return 0;

	if (!matrix || !matrix->vh_rows || !matrix->ve_rows)
		return 0;

	veo_args_clear(_veo_argp);

	ret = veo_args_set_i32(_veo_argp, 0, _ve_num_threads);
	if (ret != 0)
		return 0;

	ret = veo_args_set_u64(_veo_argp, 1, matrix->height);
	if (ret != 0)
		return 0;

	ret = veo_args_set_u64(_veo_argp, 2, matrix->width);
	if (ret != 0)
		return 0;

	ret = veo_args_set_hmem(_veo_argp, 3, matrix->ve_rows);
	if (ret != 0)
		return 0;

	ret = veo_args_set_float(_veo_argp, 4, scalar_value);
	if (ret != 0)
		return 0;

	veo_call_handle = veo_call_async_by_name(_veo_ctxt, _ve_lib_handle, _lib_scalar_matrix_mult, _veo_argp);
	if (veo_call_handle == VEO_REQUEST_ID_INVALID)
		return 0;

	ret = veo_call_wait_result(_veo_ctxt, veo_call_handle, &veo_ret);
	if (ret != VEO_COMMAND_OK)
		return 0;

	return veo_ret == 1;
}

int matrix_matrix_mult(struct matrix *matrixA, struct matrix * matrixB, struct matrix * matrixC)
{
	uint64_t veo_call_handle, veo_ret;
	unsigned long int m, n, k;
	int ret;

	if (!_ve_proc)
		return 0;

	if (!matrixA || !matrixA->vh_rows || !matrixA->ve_rows
			|| !matrixB || !matrixB->vh_rows || !matrixB->ve_rows
			|| !matrixC || !matrixC->vh_rows || !matrixC->ve_rows )
		return 0;

	if (matrixC->height != matrixA->height || matrixC->width != matrixB->width
			|| matrixA->width != matrixB->height)
		return 0;

	m = matrixA->height;
	n = matrixA->width;
	k = matrixB->width;

	veo_args_clear(_veo_argp);
	ret = veo_args_set_i32(_veo_argp, 0, _ve_num_threads);
	if (ret != 0)
		return 0;

	ret = veo_args_set_u64(_veo_argp, 1, m);
	if (ret != 0)
		return 0;

	ret = veo_args_set_u64(_veo_argp, 2, n);
	if (ret != 0)
		return 0;

	ret = veo_args_set_u64(_veo_argp, 3, k);
	if (ret != 0)
		return 0;

	ret = veo_args_set_hmem(_veo_argp, 4, matrixA->ve_rows);
	if (ret != 0)
		return 0;

	ret = veo_args_set_hmem(_veo_argp, 5, matrixB->ve_rows);
	if (ret != 0)
		return 0;

	ret = veo_args_set_hmem(_veo_argp, 6, matrixC->ve_rows);
	if (ret != 0)
		return 0;

	veo_call_handle = veo_ca_ll_async_by_name(_veo_ctxt, _ve_lib_handle, _lib_matrix_matrix_mult, _veo_argp);
	if (veo_call_handle == VEO_REQUEST_ID_INVALID)
		return 0;

	ret = veo_call_wait_result(_veo_ctxt, veo_call_handle, &veo_ret);
	if (ret != VEO_COMMAND_OK)
		return 0;

	return veo_ret == 1;
}

void set_ve_execution_node(int num_node)
{
	_ve_num_node = (num_node < 0 || num_node > 3) ? 0 : num_node;
}

void set_number_threads(int num_threads)
{
	if (num_threads > 8)
		_ve_num_threads = 8;
	else if (num_threads < 1)
		_ve_num_threads = 1;
	else
		_ve_num_threads = num_threads;
}

int init_proc_ve_node(void)
{
	if (_ve_proc)
		goto fail1;

	_ve_proc = veo_proc_create(_ve_num_node);
	if (!_ve_proc)
		goto fail1;

	_ve_lib_handle = veo_load_library(_ve_proc, _ve_lib_path);
	if (!_ve_lib_handle)
		goto fail2;

	_veo_ctxt = veo_context_open(_ve_proc);
	if (!_veo_ctxt)
		goto fail3;

	_veo_argp = veo_args_alloc();
	if (!_veo_argp)
		goto fail4;

	return 1;

	/* ERROR CLEANUP */
fail4:
	veo_context_close(_veo_ctxt);
fail3:
	veo_unload_library(_ve_proc, _ve_lib_handle);
fail2:
	veo_proc_destroy(_ve_proc);
	_ve_proc = NULL;
fail1:
	return 0;
}

int close_proc_ve_node(void)
{
	int ret = 1;
	if (!_ve_proc)
		return 0;

	veo_args_free(_veo_argp);

	if (veo_context_close(_veo_ctxt) == 0)
		ret = 0;

	if (veo_unload_library(_ve_proc, _ve_lib_handle) != 0)
		ret = 0;

	if (veo_proc_destroy(_ve_proc) < 0)
		ret = 0;

	_ve_proc = NULL;
	return ret;
}

int load_ve_matrix(struct matrix *matrix)
{
	int ret;

	if (!_ve_proc || !matrix || !matrix->vh_rows || matrix->ve_rows)
		return 0;

	ret = veo_alloc_hmem(_ve_proc, &matrix->ve_rows, sizeof(float) * matrix->height * matrix->width);

	if (ret != 0)
		return 0;
	else
		return sync_vh_ve_matrix(matrix);
}

int unload_ve_matrix(struct matrix *matrix)
{
	int ret;

	if (!_ve_proc || !matrix || !matrix->vh_rows || !matrix->ve_rows)
		return 0;

	ret = sync_ve_vh_matrix(matrix);
	ret &= veo_free_hmem(matrix->ve_rows) == 0;
	matrix->ve_rows = NULL;

	return ret;
}

int sync_vh_ve_matrix(struct matrix *matrix)
{
	if (!_ve_proc || !matrix || !matrix->ve_rows || !matrix->vh_rows)
		return 0;

	return veo_hmemcpy(matrix->ve_rows, matrix->vh_rows, sizeof(float) * matrix->height * matrix->width) == 0;
}

int sync_ve_vh_matrix(struct matrix *matrix)
{
	if (!_ve_proc || !matrix || !matrix->ve_rows || !matrix->vh_rows)
		return 0;

	return veo_hmemcpy(matrix->vh_rows, matrix->ve_rows, sizeof(float) * matrix->height * matrix->width) == 0;
}

struct matrix *zero_matrix(unsigned long int height, unsigned long int width)
{
	struct matrix *matrix = (struct matrix *)malloc(sizeof(struct matrix));
	if (!matrix)
		return NULL;

	matrix->width = width;
	matrix->height = height;

	matrix->ve_rows = NULL;
	matrix->vh_rows = (float *)calloc(height * width, sizeof(float));
	if (!matrix->vh_rows) {
		free(matrix);
		return NULL;
	}

	return matrix;
}

struct matrix *new_matrix(unsigned long int height, unsigned long int width, float *rows)
{
	struct matrix *matrix = zero_matrix(height, width);

	if (matrix)
		memcpy(matrix->vh_rows, rows, height * width * sizeof(float));

	return matrix;
}

struct matrix *read_matrix_binfile(const char *file_name, unsigned long int m_width, unsigned long int m_height)
{
	FILE *handle;
	struct matrix *matrix;
	unsigned long int matrix_size = m_width * m_height;

	matrix = zero_matrix(m_height, m_width);
	if (!matrix)
		goto fail1;

	handle = fopen(file_name, "rb");
	if (!handle) {
		fprintf(stderr, "ERRO: não foi possível abrir arquivo \"%s\"\n", file_name);
		goto fail2;
	}

	fread(matrix->vh_rows, sizeof(float), matrix_size, handle);
	fclose(handle);

	return matrix;

	/* ERROR CLEANUP */
fail2:
	delete_matrix(matrix);
fail1:
	return NULL;
}

void dump_matrix_binfile(const char *file_name, struct matrix *matrix)
{
	FILE *handle = fopen(file_name, "wb");
	if (!handle) {
		fprintf(stderr, "ERRO: não foi possível abrir arquivo \"%s\"\n", file_name);
		return;
	}

	fwrite(matrix->vh_rows, sizeof(float), matrix->height * matrix->width, handle);
	fclose(handle);
}

void delete_matrix(struct matrix *matrix)
{
	if (!matrix)
		return;

	if (matrix->vh_rows)
		free(matrix->vh_rows);

	if (matrix->ve_rows)
		veo_free_hmem(matrix->ve_rows);

	free(matrix);
}

