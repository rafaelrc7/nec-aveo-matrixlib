#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ve_offload.h>

#include "matrix_lib.h"

static int _ve_num_node = 0;
static int _ve_num_threads = 1;
static uint64_t _ve_lib_handle = 0;
static const char *_ve_lib_path = "matrix_lib_ve.so";
static struct veo_proc_handle *_ve_proc = NULL;

int scalar_matrix_mult(float scalar_value, struct matrix *matrix)
{
	struct veo_thr_ctxt *ctxt;
	struct veo_args *argp;

	if (!matrix || !matrix->vh_rows || !matrix->ve_rows)
		return 0;

}

int matrix_matrix_mult(struct matrix *matrixA, struct matrix * matrixB, struct matrix * matrixC);

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
		return 0;

	_ve_proc = veo_proc_create(_ve_num_node);
	if (!_ve_proc)
		return 0;

	_ve_lib_handle = veo_load_library(_ve_proc, _ve_lib_path);
	if (!_ve_lib_handle)
		return 0;

	return 1;
}

int close_proc_ve_node(void)
{
	if (!_ve_proc || !_ve_lib_handle)
		return 0;

	if (veo_unload_library(_ve_proc, _ve_lib_handle) != 0)
		return 0;

	if (veo_proc_destroy(_ve_proc) != 1)
		return 0;

	_ve_proc = NULL;
	return 1;
}

int load_ve_matrix(struct matrix *matrix)
{
	int ret;

	if (!_ve_proc || !matrix || !matrix->vh_rows || matrix->ve_rows)
		return 0;

	ret = veo_alloc_hmem(_ve_proc, matrix->ve_rows, sizeof(float) * matrix->height * matrix->width);

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

