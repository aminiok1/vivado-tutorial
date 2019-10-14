/*
 * accelerator.c
 *
 *  Created on: Feb 28, 2019
 *      Author: Amin
 */
#include "accelerator.h"

extern XAxiDma AxiDma;

// accelerator instance
XAccelerator_kernel xaccel_kernel;

XAccelerator_kernel_Config xaccel_kernel_config = {
		0,	//device id
		XPAR_ACCELERATOR_KERNEL_0_S_AXI_CONTROL_BUS_BASEADDR //base address for the control bus (taken from xparameters.h)
};

// interrupt handler
XScuGic ScuGic;


// for a detailed implementation of the following functions refer to xaccelerator_kernel.c file
int XAccel_kernelSetup()
{
	// this function sets the xaccel_kernel base address and sets its state to ready for execution
	return XAccelerator_kernel_CfgInitialize(&xaccel_kernel, &xaccel_kernel_config);
}

void XAccel_kernelStart(void *InstancePtr)
{
	XAccelerator_kernel *pExample = (XAccelerator_kernel *) InstancePtr;
	XAccelerator_kernel_InterruptEnable(pExample,1);
	XAccelerator_kernel_InterruptGlobalEnable(pExample);
	
	// This function sets ap_start signal to 1 that initiates the execution of the accelerator
	XAccelerator_kernel_Start(pExample);
}

void XAccel_kernelISR(void *InstancePtr)
{
	XAccelerator_kernel *pExample = (XAccelerator_kernel *) InstancePtr;
	//Disable GLobal Interrupts
	XAccelerator_kernel_InterruptGlobalDisable(pExample);
	//Disable Local Interrupts
	XAccelerator_kernel_InterruptDisable(pExample, 0xffffffff);

	//Clear Interrupt
	XAccelerator_kernel_InterruptClear(pExample,1);
}

int XAccel_kernelSetupInterrupt()
{
	int result;
	XScuGic_Config *pCfg = XScuGic_LookupConfig(XPAR_SCUGIC_SINGLE_DEVICE_ID);
	if(pCfg == NULL)
	{
		printf("Interrupt Config Look Up Failed\n");
		return XST_FAILURE;
	}
	result = XScuGic_CfgInitialize(&ScuGic, pCfg, pCfg->CpuBaseAddress);
	if(result != XST_SUCCESS)
		return result;

	//self test
	result = XScuGic_SelfTest(&ScuGic);
	if(result != XST_SUCCESS)
		return result;

	//Initialize Exception Handler
	Xil_ExceptionInit();
	//Register Exception Handler with Interrupt Service Routine
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler, &ScuGic);
	Xil_ExceptionEnable();
	result = XScuGic_Connect(&ScuGic, XPAR_FABRIC_ACCELERATOR_KERNEL_0_INTERRUPT_INTR, (Xil_ExceptionHandler)XAccel_kernelISR, &xaccel_kernel);
	if(result != XST_SUCCESS)
		return result;

	XScuGic_Enable(&ScuGic, XPAR_FABRIC_ACCELERATOR_KERNEL_0_INTERRUPT_INTR);
	return XST_SUCCESS;
}

int Setup_HW_Accelerator(double A[ST_SIZE], double B[ST_SIZE], double C[ST_SIZE])
{
	int status = XAccel_kernelSetup();
	if(status != XST_SUCCESS)
	{
		printf("Error: Accelerator Setup Failed\n");
		return XST_FAILURE;
	}
	status = XAccel_kernelSetupInterrupt();
	if(status != XST_SUCCESS)
	{
		printf("Error: Interrupt Setup Failed\n");
		return XST_FAILURE;
	}
	XAccel_kernelStart(&xaccel_kernel);

	//Cache Flush
	Xil_DCacheFlushRange((unsigned int)A, sizeof(double)*ST_SIZE);
	Xil_DCacheFlushRange((unsigned int)B, sizeof(double)*ST_SIZE);
	Xil_DCacheFlushRange((unsigned int)C, sizeof(double)*ST_SIZE);

	printf("Cache Cleared\n");

	return 0;
}

int Start_HW_Accelerator(void)
{
	int status = XAccel_kernelSetup();
	if(status != XST_SUCCESS)
	{
		printf("Error: Accelerator Setup Failed\n");
		return XST_FAILURE;
	}
	status = XAccel_kernelSetupInterrupt();
	if(status != XST_SUCCESS)
	{
		printf("Error: Interrupt Setup Failed\n");
		return XST_FAILURE;
	}

	XAccel_kernelStart(&xaccel_kernel);
	return XST_SUCCESS;
}

int Run_HW_Accelerator(double A[ST_SIZE], double B[ST_SIZE], double C[ST_SIZE])
{
	int status;
	
	//Transfer A to Hls block
	status = XAxiDma_SimpleTransfer(&AxiDma, (unsigned int) A, sizeof(double)*ST_SIZE, XAXIDMA_DMA_TO_DEVICE);
	if(status != XST_SUCCESS)
	{
		printf("Error: Transferring A to the Accelerator");
		return XST_FAILURE;
	}
	while(XAxiDma_Busy(&AxiDma, XAXIDMA_DMA_TO_DEVICE));

	//Transfer B to Hls block
	status = XAxiDma_SimpleTransfer(&AxiDma, (unsigned int) B, sizeof(double)*ST_SIZE, XAXIDMA_DMA_TO_DEVICE);
	if(status != XST_SUCCESS)
	{
		printf("Error: Transferring B to the Accelerator");
		return XST_FAILURE;
	}
	while(XAxiDma_Busy(&AxiDma, XAXIDMA_DMA_TO_DEVICE));


	//Get Results of C from Hls Block
	status = XAxiDma_SimpleTransfer(&AxiDma, (unsigned int) C, sizeof(double)*ST_SIZE, XAXIDMA_DEVICE_TO_DMA);
	if(status != XST_SUCCESS)
	{
		printf("Error: Receiving C from the Accelerator");
	}
	while (XAxiDma_Busy(&AxiDma, XAXIDMA_DEVICE_TO_DMA));

	//Poll DMA engine to ensure transfers are complete.
	while((XAxiDma_Busy(&AxiDma, XAXIDMA_DEVICE_TO_DMA)) || XAxiDma_Busy(&AxiDma, XAXIDMA_DMA_TO_DEVICE));

	return XST_SUCCESS;

}

void accelerator_ref(double A[ST_SIZE], double B[ST_SIZE], double C[ST_SIZE])
{
	int i;

	for(i = 0; i < ST_SIZE; ++i)
		C[i] = A[i] + B[i];
}
