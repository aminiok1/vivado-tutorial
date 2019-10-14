#include <stdio.h>
#include <stdlib.h>

#include "vec_add.h"

typedef double T;

// software function to compare the results of the HLS against it
void accelerator_sw(T A[DIM],
	T B[DIM],
	T C[DIM])
{
	int i;
	for (i = 0; i < DIM; ++i)
		C[i] = A[i] + B[i];
}

void init_arrays(T A[DIM],
	T B[DIM])
{
	int i;
	for (i = 0; i < DIM; ++i)
	{
		A[i] = (double)i;
		B[i] = (double) 2 * i;
	}
}

int main(void)
{
	int err = 0;
	int i;
	T A_sw[DIM];
	T A_hw[DIM];
	T B_sw[DIM];
	T B_hw[DIM];
	T C_sw[DIM];
	T C_hw[DIM];

	//Initialize Software and HW Arrays
	init_arrays(A_sw, B_sw);
	init_arrays(A_hw, B_hw);

	//Call Software Accelerator
	accelerator_sw(A_sw, B_sw, C_sw);

	//Setup Hardware Call
	//Initialize Input and output Streams
	AXI_VAL in_stream[DIM + DIM];
	AXI_VAL out_stream[DIM];

	//Stream in A
	for (i = 0; i < DIM; ++i)
		in_stream[i] = push_stream<double, 4, 5, 5>(A_hw[i], i == (DIM - 1));

	//Stream in B
	for (i = 0; i < DIM; ++i)
		in_stream[i + DIM] = push_stream<double, 4, 5, 5>(B_hw[i], i == (DIM - 1));

	//Call Wrapped accelerator
	accelerator_kernel(in_stream, out_stream);

	//Extract C from Stream
	for (i = 0; i < DIM; ++i)
		C_hw[i] = pop_stream<double, 4, 5, 5>(out_stream[i]);

	//Compare Results of Add
	for (i = 0; i < DIM; ++i)
	{
		if (abs(C_sw[i] - C_hw[i]) != 0)
		{
			printf("%f\n", C_sw[i] - C_hw[i]);
			++err;
		}
	}
	if (err == 0)
		printf("\n ----- No Errors!, HW/SW Results Match! ----- ");
	return err;
}
