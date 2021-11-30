#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/random.h>
#include <float.h>

#include "arg_lib.h"

int main(int argc, char *argv[])
{
	const char *bf_name;
	unsigned long int m_height, m_width;
	unsigned int is_random;
	unsigned int random_seed;
	float const_num;
	float *rows;
	FILE *bf;

	if (argc < 5) {
		fprintf(stderr,
			"USAGE: %s [bin file name] [matrix height] [matrix width] [is random] [if not random: matrix float constant]\n",
			argv[0]);
		exit(EXIT_FAILURE);
	}

	bf_name = argv[1];
	m_height = argtoul(argv[2]);
	m_width = argtoul(argv[3]);
	is_random = (unsigned int)argtoul(argv[4]);
	if (!is_random) {
		if (argc < 6) {
			fprintf(stderr, "For non-random matrix, you must provide a constant float as parameter.\n");
			exit(EXIT_FAILURE);
		} else {
			const_num = argtof(argv[5]);
		}
	} else {
		getrandom(&random_seed, sizeof(random_seed), 0);
		srandom(random_seed);
	}

	rows = (float *)malloc(sizeof(float) * m_height * m_width);
	if (!rows) {
		fprintf(stderr, "ERRO: Não foi possível alocar memória\n");
		exit(EXIT_FAILURE);
	}

	if (is_random) {
		unsigned int i;
		for (i = 0; i < m_height * m_width; ++i) {
			rows[i] = ((float)rand()) / (float)RAND_MAX;
		}
	} else {
		unsigned int i;
		for (i = 0; i < m_height * m_width; ++i) {
			rows[i] = const_num;
		}
	}

	bf = fopen(bf_name, "wb");
	if (!bf) {
		fprintf(stderr, "ERRO: Não foi possível criar o arquivo \"%s\"\n", bf_name);
		exit(EXIT_FAILURE);
	}
	fwrite(rows, sizeof(float), m_height * m_width, bf);
	fclose(bf);

	return 0;
}

