#include "vec_add.h"

// example code for adding two vectors

void accelerator_kernel(AXI_VAL INPUT_STREAM[DIM + DIM], AXI_VAL OUTPUT_STREAM[DIM])
{

	// these pragmas indicate that the input and output follow axi stream protocol
	#pragma HLS INTERFACE axis port=INPUT_STREAM
	#pragma HLS INTERFACE axis port=OUTPUT_STREAM
	
	// axi lite interface is used by the ARM processor to control the execution of this accelerator
	// this line also creates an interrupt port for this module which later will be used in the SDK code
	#pragma HLS INTERFACE s_axilite port=return bundle=CONTROL_BUS


	//------------------------ streaming input -------------------------

	// in this part, we read the input stream from axi interface
	double A[DIM], B[DIM], C[DIM];

	//Retrieve A from stream
	for (int i = 0; i < DIM; ++i)
			A[i] = pop_stream<double, 4, 5, 5>(INPUT_STREAM[i]);

	//Retrieve B from stream
	for (int i = 0; i < DIM; ++i)
			B[i] = pop_stream<double, 4, 5, 5>(INPUT_STREAM[i + DIM]);

	//------------------------ computation -------------------------
	for (int i = 0; i < DIM; ++i)
		C[i] = A[i] + B[i];

	//------------------------ streaming output -------------------------
	for (int i = 0; i < DIM; ++i)
	{
		// i == (DIM - 1) is for indicating the last byte (TLAST signal in axi interface)
		OUTPUT_STREAM[i] = push_stream<double, 4, 5, 5>(C[i], i == (DIM - 1));
	}
	return;
}
