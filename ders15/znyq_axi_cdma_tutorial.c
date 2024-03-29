/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "platform.h"
#include "xil_types.h"
#include "xaxicdma.h"
#include "xscugic.h"
#include "xstatus.h"
#include "xparameters.h"
#include "xtime_l.h"
#include "xil_printf.h"
#include "xdebug.h"
#include "xil_exception.h"
#include "xil_cache.h"

#define DMA_TRANSFER_SIZE 32768
//#define DMA_TRANSFER_SIZE 32
//#define SRC_DDR_MEMORY XPAR_PS7_DDR_0_S_AXI_BASEADDR+0x00200000 // pass all code and data sections	(2 MB)
//#define DST_DDR_MEMORY SRC_DDR_MEMORY+0x00200000 // (2 MB)
//#define DST_OCM_MEMORY XPAR_PS7_RAM_1_S_AXI_BASEADDR

// INTC
#define INTC_DEVICE_ID XPAR_SCUGIC_0_DEVICE_ID
//#define INTC_BASEADDR XPAR_PS7_SCUGIC_0_BASEADDR

// DMA
#define DMA_ID XPAR_AXI_CDMA_0_DEVICE_ID
#define DMA_CTRL_IRPT_INTR	XPAR_FABRIC_AXICDMA_0_VEC_ID

// driver instances
static XScuGic Intc; // The instance of the generic interrupt controller (GIC)
static XAxiCdma Dma;	/* Instance of the XAxiCdma */

#define CDMA_BRAM_MEMORY_0 0xC0000000	// portA of BRAM (connection from CDMA)
#define CDMA_BRAM_MEMORY_1 0x40000000	// portB of BRAM (connection from PS)
#define CDMA_OCM_MEMORY_0 0x00000000

// Source and Destination memory segments
unsigned char Src[DMA_TRANSFER_SIZE];						// source is always DRAM
unsigned char Dst_DDR[DMA_TRANSFER_SIZE];					// for CDMA to access the DRAM
unsigned char Dst_OCM[DMA_TRANSFER_SIZE]__attribute__((section(".mba_ocm_section")));
unsigned char Dst_BRAM[DMA_TRANSFER_SIZE]__attribute__((section(".mba_bram_section")));
cdma_memory_destination_bram = (u8 *)CDMA_BRAM_MEMORY_0;	// for CDMA to access the BRAM
cdma_memory_destination_ocm = (u8 *)CDMA_OCM_MEMORY_0;		// for CDMA to access the OCM

// AXI CDMA related definitions
XAxiCdma_Config *DmaCfg;

int Status = 0;
int i = 0;

XTime tStart, tEnd;
u32 totalCycle = 0;

int Done = 0;
int Error = 0;

// lfsr seed
uint8_t start_state = 0xACu;  /* Any nonzero start state will work. */
uint8_t lfsr = 0xACu;

// GIC Setup
int SetupInterruptSystem(XScuGic *GicInstancePtr,XAxiCdma *DmaPtr);
// DMA done interrupt handler
static void Example_CallBack(void *CallBackRef, u32 IrqMask, int *IgnorePtr);
// function decleration for simple 8-bit lfsr for pseudo-random number generator
unsigned char lfsr_rand();


int main()
{
    init_platform();

	/* Initialize the XAxiCdma device.
	 */
    DmaCfg = XAxiCdma_LookupConfig(DMA_ID);
	if (!DmaCfg) {
		return XST_FAILURE;
	}

	Status = XAxiCdma_CfgInitialize(&Dma, DmaCfg, DmaCfg->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	Status = SetupInterruptSystem(&Intc,&Dma);
	if (Status != XST_SUCCESS) return XST_FAILURE;

	XAxiCdma_IntrEnable(&Dma, XAXICDMA_XR_IRQ_ALL_MASK);

	// Disable DCache
	Xil_DCacheDisable();


	/******************************************************************************/
	// First transfer DDR to DDR with AXI CDMA
	/******************************************************************************/
	// initialize writedata array with random numbers
    int i = 0;
    for (i=0;i<DMA_TRANSFER_SIZE;i++)
    {
    	Src[i] = lfsr_rand();
    }

    // clear destination array
    for (i=0;i<DMA_TRANSFER_SIZE;i++)
    {
    	Dst_DDR[i] = 0x00;
    }

	Done = 0;
	Status = XAxiCdma_SimpleTransfer(&Dma, (u32)Src,
		(u32)Dst_DDR, DMA_TRANSFER_SIZE, Example_CallBack,
		(void *)&Dma);
	XTime_GetTime(&tStart);
	if (Status == XST_SUCCESS)
	{
		while (!Done && !Error);
	}
	XTime_GetTime(&tEnd);

	u64 et_dma_ddr = tEnd - tStart;
	float et_us_dma_ddr = ((float)((float)et_dma_ddr) / 333333333.3)*1000000;

    // check if write and read arrays are equal
    for (i=0;i<DMA_TRANSFER_SIZE;i++)
    {
    	if (Src[i] != Dst_DDR[i])
    	return XST_FAILURE;
    }

	/******************************************************************************/
	// Second transfer DDR to OCM with AXI CDMA
	/******************************************************************************/
    for (i=0;i<DMA_TRANSFER_SIZE;i++)
    {
    	Src[i] = lfsr_rand();
    }

    // clear destination array
    for (i=0;i<DMA_TRANSFER_SIZE;i++)
    {
    	Xil_Out8(CDMA_OCM_MEMORY_0+i, 0x00);
    }

	Done = 0; Error = 0;
	Status = XAxiCdma_SimpleTransfer(&Dma, (u32)Src,
		(u32)cdma_memory_destination_ocm, DMA_TRANSFER_SIZE, Example_CallBack,
		(void *)&Dma);
	XTime_GetTime(&tStart);
	if (Status == XST_SUCCESS)
	{
		while (!Done && !Error);
	}
	XTime_GetTime(&tEnd);

	u64 et_dma_ocm = tEnd - tStart;
	float et_us_dma_ocm = ((float)((float)et_dma_ocm) / 333333333.3)*1000000;

    // check if write and read arrays are equal
    for (i=0;i<DMA_TRANSFER_SIZE;i++)
    {
    	u8 dataread = Xil_In8(CDMA_OCM_MEMORY_0+i);
    	if (Src[i] != dataread)
    	return XST_FAILURE;
    }

	/******************************************************************************/
	// Third transfer DDR to BRAM with AXI CDMA
	/******************************************************************************/
    // refill source array with pseudo-random numbers
    for (i=0;i<DMA_TRANSFER_SIZE;i++)
    {
    	Src[i] = lfsr_rand();
    }

    // clear destination array
    for (i=0;i<DMA_TRANSFER_SIZE;i++)
    {
    	Xil_Out8(CDMA_BRAM_MEMORY_1+i, 0x00);
    }

	Done = 0; Error = 0;
	Status = XAxiCdma_SimpleTransfer(&Dma, (u32)Src,
		(u32)cdma_memory_destination_bram, DMA_TRANSFER_SIZE, Example_CallBack,
		(void *)&Dma);
	XTime_GetTime(&tStart);
	if (Status == XST_SUCCESS)
	{
		while (!Done && !Error);
	}
	XTime_GetTime(&tEnd);

	u64 et_dma_bram = tEnd - tStart;
	float et_us_dma_bram = ((float)((float)et_dma_bram) / 333333333.3)*1000000;

    // check if write and read arrays are equal
    for (i=0;i<DMA_TRANSFER_SIZE;i++)
    {
    	u8 dataread = Xil_In8(CDMA_BRAM_MEMORY_1+i);
    	if (Src[i] != dataread)
    	return XST_FAILURE;
    }

	/******************************************************************************/
	// Fourth transfer DDR to DDR with CPU load/store instructions
	/******************************************************************************/
    // refill source array with pseudo-random numbers
    for (i=0;i<DMA_TRANSFER_SIZE;i++)
    {
    	Src[i] = lfsr_rand();
    }

    // clear destination array
    for (i=0;i<DMA_TRANSFER_SIZE;i++)
    {
    	Dst_DDR[i] = 0x00;
    }

    // calculate time
    XTime_GetTime(&tStart);
    for (i=0;i<DMA_TRANSFER_SIZE;i++)
    {
    	Dst_DDR[i] = Src[i];
    }
    XTime_GetTime(&tEnd);

	u64 et_ls = tEnd - tStart;
	float et_us_ls = ((float)((float)et_ls) / 333333333.3)*1000000;

    cleanup_platform();
    return 0;
}

/***************************************************************************************************************************************************/
																// SetupInterruptSystem
/***************************************************************************************************************************************************/
int SetupInterruptSystem(XScuGic *GicInstancePtr,XAxiCdma *DmaPtr)
{
	XScuGic_Config *IntcConfig;

	Xil_ExceptionInit();

	// initialize the interrupt control driver so that it is ready to use
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	Status = XScuGic_CfgInitialize(GicInstancePtr, IntcConfig, IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) return XST_FAILURE;

	XScuGic_SetPriorityTriggerType(GicInstancePtr, DMA_CTRL_IRPT_INTR, 0xA0, 0x3);

	// Connect the interrupt controller interrupt handler to the hardware interrupt handling logic in the processor
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler, GicInstancePtr);

	// Connect the device driver handler that will be called when an interrupt for the device occurs
	// The handler defined above performs the specific interrupt processing for the device

	// Connect the Fault ISR
	Status = XScuGic_Connect(GicInstancePtr,DMA_CTRL_IRPT_INTR,(Xil_InterruptHandler)XAxiCdma_IntrHandler,(void *)DmaPtr);
	if (Status != XST_SUCCESS) return XST_FAILURE;

	// Enable the interrupt for the device
	XScuGic_Enable(GicInstancePtr, DMA_CTRL_IRPT_INTR);

	Xil_ExceptionInit();

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
				(Xil_ExceptionHandler)XScuGic_InterruptHandler,
				GicInstancePtr);

	Xil_ExceptionEnable();

	// enable interrupts in the processor
	//Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);

	return XST_SUCCESS;
}

static void Example_CallBack(void *CallBackRef, u32 IrqMask, int *IgnorePtr)
{

	if (IrqMask & XAXICDMA_XR_IRQ_ERROR_MASK) {
		Error = TRUE;
	}

	if (IrqMask & XAXICDMA_XR_IRQ_IOC_MASK) {
		Done = TRUE;
	}

}

// simple 8-bit lfsr for pseudo-random number generator
unsigned char lfsr_rand()
{
	unsigned lsb = lfsr & 1;   /* Get LSB (i.e., the output bit). */
	lfsr >>= 1;                /* Shift register */
	if (lsb) {                 /* If the output bit is 1, apply toggle mask. */
		lfsr ^= 0x71u;
	}
	return lfsr;
}
