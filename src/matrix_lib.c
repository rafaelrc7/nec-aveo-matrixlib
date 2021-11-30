#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <immintrin.h>

#include "matrix_lib_o.h"

static unsigned int op_thread_num = 1;

static
Matrix *build_matrix(unsigned long int height, unsigned long int width);

/* Macro for accessing a matrix m at row r and column c */
#define MATRIX_EL(m, r, c) ((float *)&m->rows[c + r * m->width])

void set_number_threads(int num_threads)
{
	if (num_threads < 1)
		return;

	op_thread_num = (unsigned int)num_threads;
}

typedef struct scalar_matrix_mult_data {
	float *lines;
	unsigned long int length;
	float scalar;
} _scalar_data;

static
void *scalar_matrix_mult_thread(void *args)
{
	unsigned long int i;
	__m256 vec_scalar, vec_line;
	_scalar_data *data;
	float *arr_lines;
	unsigned long int length;

	if (!args)
		pthread_exit((void *)-1);

	data = (_scalar_data *)args;
	arr_lines = data->lines;
	length = data->length;

	vec_scalar = _mm256_set1_ps(data->scalar);
	for (i = 0; i != length; i += 8, arr_lines += 8) {
		vec_line = _mm256_load_ps(arr_lines);
		vec_line = _mm256_mul_ps(vec_line, vec_scalar);
		_mm256_store_ps(arr_lines, vec_line);
	}

	pthread_exit(0);
}

int scalar_matrix_mult(float scalar_value, Matrix *matrix)
{
	pthread_t *threads;
	pthread_attr_t p_attr;
	_scalar_data *threads_data;
	unsigned long int t, length;
	float *arr_m_rows;
	void *status;
	int ret;

	/* Check if matrix exists and has a valid number of valid rows */
	if (!matrix || !matrix->rows || !matrix->height || !matrix->width)
		goto fail1;

	/* Check if matrix has a number of lines divisable by the number of threads
	 * and a number of columns divisable by 8, size of AVX operations */
	if ((matrix->height % op_thread_num != 0) || (matrix->width % 8 != 0))
		goto fail1;

	/* Allocate memory for threads and the structs used for arguments */
	threads = (pthread_t *)calloc(op_thread_num, sizeof(pthread_t));
	if (!threads)
		goto fail1;

	threads_data = (_scalar_data *)calloc(op_thread_num, sizeof(_scalar_data));
	if (!threads_data)
		goto fail2;

	pthread_attr_init(&p_attr);
	pthread_attr_setdetachstate(&p_attr, PTHREAD_CREATE_JOINABLE);

	/* Calculate the number of numbers each thread will process */
	length = matrix->height / op_thread_num * matrix->width;

	/* Initialise threads with the proper arguments */
	arr_m_rows = matrix->rows;
	for (t = 0; t != op_thread_num; ++t, arr_m_rows += length) {
		threads_data[t].length = length;
		threads_data[t].lines = arr_m_rows;
		threads_data[t].scalar = scalar_value;
		ret = pthread_create(&threads[t], &p_attr, scalar_matrix_mult_thread, (void *)&threads_data[t]);
		if (ret)
			goto fail4;
	}

	/* Wait for threads to finish while checking if they terminated ok */
	for (t = 0; t != op_thread_num; ++t) {
		ret = pthread_join(threads[t], &status);
		if (ret || (long)(status))
			goto fail4;
	}

	pthread_attr_destroy(&p_attr);
	free(threads_data);
	free(threads);

	return 1;

	/* ERROR CLEANUP */
fail4:
	for (t = 0; t != op_thread_num; ++t) {
		if (threads[t])
			pthread_join(threads[t], &status);
	}
	pthread_attr_destroy(&p_attr);
	free(threads_data);
fail2:
	free(threads);
fail1:
	return 0;
}

typedef struct matrix_matrix_mult_data {
	float *arr_rows_a, *arr_rows_c;
	Matrix *matrixA, *matrixB, *matrixC;
	unsigned long int lines;
} _matrix_matrix_data;

void *matrix_matrix_mult_thread(void *args)
{
	unsigned long int i, j, k;
	__m256 vec_rows_b, vec_rows_c, vec_maij;
	float *arr_rows_b, *arr_ik_c;

	_matrix_matrix_data *data = (_matrix_matrix_data *)args;
	Matrix *matrixA = data->matrixA;
	Matrix *matrixB = data->matrixB;
	Matrix *matrixC = data->matrixC;
	float *arr_rows_a = data->arr_rows_a;
	float *arr_rows_c = data->arr_rows_c;
	unsigned long int lines = data->lines;

	/* i = linhas da matriz A
	 * j = colunas da matriz B
	 * k = colunas da matriz C (igual ao num de colunas de B)
	 */
	for (i = 0; i < lines; ++i, arr_rows_a += matrixA->width, arr_rows_c += matrixC->width) {
		arr_rows_b = matrixB->rows; /* primeira linha de B */
		for (j = 0; j < matrixA->width; ++j) {
			vec_maij = _mm256_set1_ps(arr_rows_a[j]);
			arr_ik_c = arr_rows_c;
			for (k = 0; k < matrixB->width; k += 8, arr_rows_b += 8, arr_ik_c += 8) {
				vec_rows_b = _mm256_load_ps(arr_rows_b);
				vec_rows_c = _mm256_load_ps(arr_ik_c);

				if (j == 0)
					vec_rows_c = _mm256_mul_ps(vec_maij, vec_rows_b);
				else
					vec_rows_c = _mm256_fmadd_ps(vec_maij, vec_rows_b, vec_rows_c);

				_mm256_store_ps(arr_ik_c, vec_rows_c);
			}
		}
	}

	pthread_exit(0);
}

int matrix_matrix_mult(Matrix *matrixA, Matrix *matrixB, Matrix *matrixC)
{
	pthread_t *threads;
	pthread_attr_t p_attr;
	_matrix_matrix_data *threads_data;
	unsigned long int t, lines;
	float *arr_rows_a;
	float *arr_rows_c;
	void *status;
	int ret;

	/* Check if matrices are valid */
	if (!matrixA || !matrixB || !matrixC)
		goto fail1;

	/* Check if matrices have valid array of rows */
	if (!matrixA->rows || !matrixB->rows || !matrixC->rows)
		goto fail1;

	/* Check if dimensions of matrices match according to multiplication rules */
	if (matrixC->height != matrixA->height || matrixC->width != matrixB->width
			|| matrixA->width != matrixB->height) {
		goto fail1;
	}

	/* Check if the final height is divisible by the number of threads
	 * and if the final width is divisibel by 8, size of AVX operations */
	if ((matrixA->height % op_thread_num != 0) || (matrixB->width % 8 != 0))
		goto fail1;

	threads = (pthread_t *)calloc(op_thread_num, sizeof(pthread_t));
	if (!threads)
		goto fail1;

	threads_data = (_matrix_matrix_data *)calloc(op_thread_num, sizeof(_matrix_matrix_data));
	if (!threads_data)
		goto fail2;

	pthread_attr_init(&p_attr);
	pthread_attr_setdetachstate(&p_attr, PTHREAD_CREATE_JOINABLE);

	/* Calculate how many lines each thread will calculate */
	lines = matrixA->height / op_thread_num;

	/* Initialise threads with the proper arguments */
	arr_rows_a = matrixA->rows;
	arr_rows_c = matrixC->rows;
	for (t = 0; t != op_thread_num; ++t, arr_rows_a += matrixA->width*lines, arr_rows_c += matrixC->width*lines) {
		threads_data[t].lines = lines;
		threads_data[t].matrixA = matrixA;
		threads_data[t].matrixB = matrixB;
		threads_data[t].matrixC = matrixC;
		threads_data[t].arr_rows_a = arr_rows_a;
		threads_data[t].arr_rows_c = arr_rows_c;
		ret = pthread_create(&threads[t], &p_attr, matrix_matrix_mult_thread, (void *)&threads_data[t]);
		if (ret)
			goto fail3;
	}

	/* Wait for threads to finish while checking if they terminated ok */
	for (t = 0; t != op_thread_num; ++t) {
		ret = pthread_join(threads[t], &status);
		if (ret || (long)(status))
			goto fail3;
	}

	pthread_attr_destroy(&p_attr);
	free(threads_data);
	free(threads);

	return 1;

	/* ERROR CLEANUP */
fail3:
	for (t = 0; t != op_thread_num; ++t) {
		if (threads[t])
			pthread_join(threads[t], &status);
	}
	pthread_attr_destroy(&p_attr);
	free(threads_data);
fail2:
	free(threads);
fail1:
	return 0;
}

void print_matrix(Matrix *matrix)
{
	register unsigned long int lin, col;
	for (lin = 0; lin < matrix->height; ++lin) {
		for (col = 0; col < matrix->width; ++col) {
			printf("%5.2f\t", *MATRIX_EL(matrix, lin, col));
		}
		printf("\n");
	}
	printf("\n");
}

static
Matrix *build_matrix(unsigned long int height, unsigned long int width)
{
	Matrix *matrix = (Matrix *)malloc(sizeof(Matrix));
	if (!matrix) {
		return NULL;
	}

	matrix->rows = (float *)aligned_alloc(32, sizeof(float) * height * width);
	if (!matrix->rows) {
		free(matrix);
		return NULL;
	}

	matrix->width = width;
	matrix->height = height;

	return matrix;
}

Matrix *new_matrix(unsigned long int height, unsigned long int width, float *rows)
{
	Matrix *matrix = build_matrix(height, width);

	if (matrix) {
		__m256 vec_rows;
		unsigned long int i;
		float *arr_rows = rows, *arr_m_rows = matrix->rows;

		for (i = 0; i < height * width; i += 8, arr_rows += 8, arr_m_rows += 8) {
			vec_rows = _mm256_load_ps(arr_rows);
			_mm256_store_ps(arr_m_rows, vec_rows);
		}
	}

	return matrix;
}

Matrix *zero_matrix(unsigned long int height, unsigned long int width)
{
	Matrix *matrix = build_matrix(height, width);

	if (matrix) {
		__m256 vec_zero;
		unsigned long int i;
		float *arr_m_rows = matrix->rows;

		vec_zero = _mm256_set1_ps(0.0f);
		for (i = 0; i < height * width; i += 8, arr_m_rows += 8) {
			_mm256_store_ps(arr_m_rows, vec_zero);
		}
	}

	return matrix;
}

Matrix *read_matrix_binfile(const char *file_name, unsigned long int m_width, unsigned long int m_height)
{
	Matrix *matrix;
	unsigned long int matrix_size = m_width * m_height;
	float *rows = (float *)aligned_alloc(32, sizeof(float) * matrix_size);
	FILE *bf = fopen(file_name, "rb");
	if (rows == NULL || bf == NULL) return NULL;

	fread(rows, sizeof(float), matrix_size, bf);
	fclose(bf);
	matrix = new_matrix(m_height, m_width, rows);
	free(rows);
	return matrix;
}

void dump_matrix_binfile(const char *file_name, Matrix *matrix)
{
	FILE *bf = fopen(file_name, "wb");
	if (!bf) {
		fprintf(stderr, "ERRO: não foi possível abrir arquivo \"%s\"\n", file_name);
		exit(EXIT_FAILURE);
	}

	fwrite(matrix->rows, sizeof(float), matrix->height * matrix->width, bf);
	fclose(bf);
}

void delete_matrix(Matrix *matrix)
{
	free(matrix->rows);
	free(matrix);
}

