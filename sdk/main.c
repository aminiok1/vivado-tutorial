#include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include "xparameters.h"
#include "xaxidma.h"
#include "accelerator.h"
#include "xil_printf.h"

#define NUM_TESTS 1

//AXI DMA Instance
XAxiDma AxiDma;				

int init_dma() {
	XAxiDma_Config *CfgPtr;
	int status;

	// check xparameters.h to see where XPAR_AXI_DMA_0_DEVICE_ID comes from
	CfgPtr = XAxiDma_LookupConfig((XPAR_AXI_DMA_0_DEVICE_ID));
	if (!CfgPtr) {
		print("Error: Failed to find DMA Config\n");
		return XST_FAILURE;
	}
	status = XAxiDma_CfgInitialize(&AxiDma, CfgPtr);
	if (status != XST_SUCCESS) {
		print("Error: Failed to Initialize DMA\n");
		return XST_FAILURE;
	}
	
	//check for scatter gather mode
	if (XAxiDma_HasSg(&AxiDma)) {
		print("Error: DMA has Scatter Gather Enabled \n");
		return XST_FAILURE;
	}
	
	//Reset Dma
	XAxiDma_Reset(&AxiDma);
	
	//wait for the reset to finish
	while(!XAxiDma_ResetIsDone(&AxiDma));

	return XST_SUCCESS;
}

void init_arrays(double A[ST_SIZE], double B[ST_SIZE], double C[ST_SIZE])
{
	int i;

	for (i = 0; i < ST_SIZE; i++)
		A[i] = (double) (i);

	for (i = 0; i < ST_SIZE; i++)
		B[i] = (double) (2*i);

	/*for (i = 0; i < ST_SIZE; i++)
		C[i] = 0;*/
}

int main(int argc, char **argv)
{
	int i, k;
	int err = 0 ;
	int status;
	int num_of_trials = 1;
	double errAccum = 0;
	double A_sw[ST_SIZE], B_sw[ST_SIZE], C_sw[ST_SIZE];
	double A_hw[ST_SIZE], B_hw[ST_SIZE], C_hw[ST_SIZE];

	//enable caches and initialize uart
	init_platform();

	xil_printf("start\n\r");

	if (argc >= 2) {
		num_of_trials = atoi(argv[1]);
	}

	status = init_dma();

	if (status != XST_SUCCESS) {
		print("Error: Initializing DMA Failed\n");
		return XST_FAILURE;
	}

	print("DMA Initialized\n");

	init_arrays(A_sw, B_sw, C_sw);
	init_arrays(A_hw, B_hw, C_hw);

	for (k = 0; k < num_of_trials; k++) {
		//*******************************************************************************
		//Call software version of function
		xil_printf("Running vector add in SW\n");
		for (i = 0; i < NUM_TESTS; ++i)
			accelerator_ref(A_sw, B_sw, C_sw);

		//*******************************************************************************

		//Call Hardware version of accelerator
		xil_printf("Running vector add in HW\n");
		Setup_HW_Accelerator(A_hw, B_hw, C_hw);

		Start_HW_Accelerator();
		Run_HW_Accelerator(A_hw, B_hw, C_hw);

		//**************************************************************************
		//Compare Results of add
		for (i = 0; i < ST_SIZE; i++)
			if(C_sw[i] != C_hw[i])
			{
				printf("index = %d, sw = %lf, hw = %lf\n", i, C_sw[i], C_hw[i]);
				err++;
				errAccum += abs(C_sw[i] - C_hw[i]);
			}

		errAccum = errAccum / err;
	}

	if (err == 0)
		print("SW and HW Results Match!\n");

	else
	{
		printf("ERROR: Results Mismatch\n");
		printf("Errors: %d Average Error: %f\n", err, errAccum);
	}

	cleanup_platform();

	return 0;
}
