#include <stdint.h>
#include <veo_hmem.h>

uint64_t scalar_matrix_mult(int num_threads, unsigned long int height, unsigned long int width, float *rows, float scalar)
{
	int tid;
	unsigned long int matrix_size = height * width;
	const unsigned long int n = matrix_size / num_threads;
	const unsigned long int rest = matrix_size % num_threads;
	const unsigned long int start_extra_thread = num_threads - rest;

	rows = (float *)veo_get_hmem_addr(rows);
	if (!rows)
		return 0;

	#pragma omp parallel private (num_threads, tid)
	{
		unsigned long int first_index, last_index, i;
		tid = omp_get_thread_num();

		first_index = tid * n + ((tid - rest) * ((tid > start_extra_thread) * (tid - start_extra_thread)));
		last_index = first_index + n + (tid >= start_extra_thread);

		for (int i = first_index; i < last_index; ++i)
			rows[i] *= scalar;
	}

	return 1;
}

