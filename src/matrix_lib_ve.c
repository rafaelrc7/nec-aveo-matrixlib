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

uint64_t matrix_matrix_mult(int num_threads,
							unsigned long int m,
							unsigned long int n, // p
							unsigned long int k, // q
							float *mA_rows,
							float *mB_rows,
							float *mC_rows)
{
	int tid;
	const unsigned long int els = m / num_threads;
	const unsigned long int rest = m % num_threads;
	const unsigned long int start_extra_thread = num_threads - rest;

	mA_rows = (float *)veo_get_hmem_addr(mA_rows);
	if (!mA_rows)
		return 0;

	mB_rows = (float *)veo_get_hmem_addr(mB_rows);
	if (!mB_rows)
		return 0;

	mC_rows = (float *)veo_get_hmem_addr(mC_rows);
	if (!mC_rows)
		return 0;

	#pragma omp parallel private (num_threads, tid)
	{
		unsigned long int first_line, last_line, ln, cl, ij;
		tid = omp_get_thread_num();

		first_line = tid * els + ((tid - rest) * ((tid > start_extra_thread) * (tid - start_extra_thread)));
		last_line = first_line + els + (tid >= start_extra_thread);

		for (ln = first_line; ln < last_line; ++ln) {
			for (cl = 0; cl < k; ++cl) {
				float sum = 0.0f;
				for (ij = 0; ij < n; ++ij) {
					sum += mA_rows[ln * n + ij] * mB_rows[ij * k + cl];
				}
				mC_rows[ln * k + cl] = sum;
			}

		}
	}

	return 1;

}

