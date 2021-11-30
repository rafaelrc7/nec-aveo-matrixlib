#ifndef _MATRIX_LIB_H
#define _MATRIX_LIB_H

struct matrix {
	unsigned long int height;
	unsigned long int width;
	float *vh_rows;
	void *ve_rows;
};

int scalar_matrix_mult(float scalar_value, struct matrix *matrix);
int matrix_matrix_mult(struct matrix *matrixA, struct matrix * matrixB, struct matrix * matrixC);

void set_ve_execution_node(int num_node);
void set_number_threads(int num_threads);

int init_proc_ve_node(void);
int close_proc_ve_node(void);

int load_ve_matrix(struct matrix *matrix);
int unload_ve_matrix(struct matrix *matrix);

int sync_vh_ve_matrix(struct matrix *matrix);
int sync_ve_vh_matrix(struct matrix *matrix);

struct matrix *new_matrix(unsigned long int height, unsigned long int width, float *rows);
struct matrix *zero_matrix(unsigned long int height, unsigned long int width);
struct matrix *read_matrix_binfile(const char *file_name, unsigned long int m_width, unsigned long int m_height);

void dump_matrix_binfile(const char *file_name, struct matrix *matrix);

void delete_matrix(struct matrix *matrix);

#endif /* ifndef _Mstruct matrixLIB_H */

