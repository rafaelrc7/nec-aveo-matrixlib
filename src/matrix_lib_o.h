#ifndef _MATRIX_LIB_H
#define _MATRIX_LIB_H

typedef struct matrix {
	unsigned long int height; /* rows    */
	unsigned long int width;  /* columns */
	float *rows;
} Matrix;

int scalar_matrix_mult(float scalar_value, Matrix *matrix);
int matrix_matrix_mult(Matrix *matrixA, Matrix *matrixB, Matrix *matrixC);
void set_number_threads(int num_threads);

void print_matrix(Matrix *matrix);
Matrix *new_matrix(unsigned long int height, unsigned long int width, float *rows);
Matrix *zero_matrix(unsigned long int height, unsigned long int width);
Matrix *read_matrix_binfile(const char *file_name, unsigned long int m_width, unsigned long int m_height);
void dump_matrix_binfile(const char *file_name, Matrix *matrix);
void delete_matrix(Matrix *matrix);

#endif /* #ifndef _MATRIX_LIB_H */
