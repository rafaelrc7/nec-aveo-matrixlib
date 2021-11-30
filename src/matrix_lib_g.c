#include <immintrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct matrix {
	unsigned long int height;
	unsigned long int width;
	float *rows;
};

typedef struct matrix Matrix;

#define MATRIX_EL(m, r, c) ((float *)&m->rows[c + r * m->width])

void set_number_threads(int num_threads)
{

}

int scalar_matrix_mult(float scalar_value, struct matrix *matrix) {
  unsigned long int i;
  unsigned long int N;

  /* Check the numbers of the elements of the matrix */
  N = matrix->height * matrix->width;

  /* Check the integrity of the matrix */
  if (N == 0 || matrix->rows == NULL) return 0;

  for (i = 0; i < N; ++i) {
        matrix->rows[i] = matrix->rows[i] * scalar_value;
  }

  return 1;
}

int matrix_matrix_mult(struct matrix *a, struct matrix *b, struct matrix *c) {
  unsigned long int NA, NB, NC, c_line, a_col, b_col;
  float *first_c_i_j, *next_a_i_j, *next_b_i_j, *next_c_i_j;

  /* Check the numbers of the elements of the matrix */
  NA = a->height * a->width;
  NB = b->height * b->width;
  NC = c->height * c->width;

  /* Check the integrity of the matrix */
  if ( (NA == 0 || a->rows == NULL) ||
       (NB == 0 || b->rows == NULL) ||
       (NC == 0 || c->rows == NULL) ) return 0;

  /* Check if we can execute de product of matrix A and matrix B */
  if ( (a->width != b->height) ||
       (c->height != a->height) ||
       (c->width != b->width) ) return 0;

  /* Compute the product of matrix A and B using the optimized algorithm. */
  /* Compute the result for each line of C on each iteration of the loop. */
  /* Each aij of the correspondent line of cij execute an scalar product  */
  /* on each bij of the correspondent line of matrix B to calcule the     */
  /* partial value of one cij (one of the factors) of the current line of */
  /* matrix C.                   					  */
  for (c_line = 0; c_line < c->height; ++c_line) {
	first_c_i_j = c->rows + (c_line * c->width);
	next_a_i_j = a->rows + (c_line * a->width);
	next_b_i_j = b->rows;
	for (a_col = 0; a_col < a->width; ++a_col, ++next_a_i_j) {
		next_c_i_j = first_c_i_j;
		for (b_col = 0; b_col < b->width; ++b_col, ++next_b_i_j, ++next_c_i_j) {
			if (a_col == 0) *(next_c_i_j) = 0.0f;
			*(next_c_i_j) += *(next_a_i_j) * *(next_b_i_j);
		}

	}
  }

  return 1;
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

	matrix->rows = (float *)malloc(sizeof(float) * height * width);
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

	if (matrix)
		memcpy(matrix->rows, rows, sizeof(float) * height * width);

	return matrix;
}

Matrix *zero_matrix(unsigned long int height, unsigned long int width)
{
	Matrix *matrix = build_matrix(height, width);

	if (matrix)
		memset(matrix->rows, 0, sizeof(float) * height * width);

	return matrix;
}

Matrix *read_matrix_binfile(const char *file_name, unsigned long int m_width, unsigned long int m_height)
{
	Matrix *matrix;
	unsigned long int matrix_size = m_width * m_height;
	float *rows = (float *)malloc(sizeof(float) * matrix_size);
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
