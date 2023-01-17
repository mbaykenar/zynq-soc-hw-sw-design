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
#include "xdmaps.h"
#include "xscugic.h"
#include "xstatus.h"
#include "xparameters.h"
#include "xtime_l.h"
#include "xil_printf.h"

#define DMA_TRANSFER_SIZE 32768
//#define DMA_TRANSFER_SIZE 512
#define SRC_DDR_MEMORY XPAR_PS7_DDR_0_S_AXI_BASEADDR+0x00200000 // pass all code and data sections	(2 MB)
#define DST_DDR_MEMORY SRC_DDR_MEMORY+0x00200000 // (2 MB)
#define DST_OCM_MEMORY XPAR_PS7_RAM_1_S_AXI_BASEADDR

// INTC
#define INTC_DEVICE_ID XPAR_SCUGIC_0_DEVICE_ID
#define INTC_BASEADDR XPAR_PS7_SCUGIC_0_BASEADDR

// DMA
#define DMA0_ID XPAR_XDMAPS_1_DEVICE_ID

// driver instances
static XScuGic Intc; // The instance of the generic interrupt controller (GIC)
XDmaPs Dma;				/* PS DMA */

// Source and Destination memory segments
unsigned char Src[DMA_TRANSFER_SIZE];
unsigned char Dst_DDR[DMA_TRANSFER_SIZE];
unsigned char Dst_OCM[DMA_TRANSFER_SIZE]__attribute__((section(".mba_ocm_section")));
unsigned char Dst_BRAM[DMA_TRANSFER_SIZE]__attribute__((section(".mba_bram_section")));

// PS DMA related definitions
XDmaPs_Config *DmaCfg;
XDmaPs_Cmd DmaCmd = {
	.ChanCtrl = {
		.SrcBurstSize = 4,
		.SrcBurstLen = 4,
		.SrcInc = 1,		// increment source address
		.DstBurstSize = 4,
		.DstBurstLen = 4,
		.DstInc = 1,		// increment destination address
	},
};

unsigned int Channel = 0;
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
int SetupInterruptSystem(XScuGic *GicInstancePtr,XDmaPs *DmaPtr);
// DMA done interrupt handler
void DmaDoneHandler(unsigned int Channel,XDmaPs_Cmd *DmaCmd,void *CallbackRef);
// DMA fault interrupt handler
void DmaFaultHandler(unsigned int Channel,XDmaPs_Cmd *DmaCmd,void *CallbackRef);
// function decleration for simple 8-bit lfsr for pseudo-random number generator
unsigned char lfsr_rand();

int main()
{
    init_platform();

	Status = SetupInterruptSystem(&Intc,&Dma);
	if (Status != XST_SUCCESS) return XST_FAILURE;

	// Disable cache if needed
	Xil_SetTlbAttributes(0xFFFF0000,0x14de2);
	Xil_DCacheDisable();

	XDmaPs_ResetManager(&Dma);
	XDmaPs_ResetChannel(&Dma,Channel);

	// Setup DMA Controller
	DmaCfg = XDmaPs_LookupConfig(DMA0_ID);
	if (!DmaCfg) {
		xil_printf("Lookup DMAC %d failed\r\n", DMA0_ID);
		return XST_FAILURE;
	}

	Status = XDmaPs_CfgInitialize(&Dma,DmaCfg,DmaCfg->BaseAddress);

	if (Status) {
		xil_printf("XDmaPs_CfgInitialize failed\r\n");
		return XST_FAILURE;
	}

	XDmaPs_SetDoneHandler(&Dma,0,DmaDoneHandler,(void*)0);
	XDmaPs_SetFaultHandler(&Dma,DmaFaultHandler,(void*)0);

	/******************************************************************************/
	// First transfer DDR to DDR with PS DMA
	/******************************************************************************/
	DmaCmd.BD.SrcAddr = (u32)Src;
	DmaCmd.BD.DstAddr = (u32)Dst_DDR;
	DmaCmd.BD.Length =  DMA_TRANSFER_SIZE;

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
	Status = XDmaPs_Start(&Dma, Channel, &DmaCmd, 0);
	XTime_GetTime(&tStart);
	if (Status == XST_SUCCESS)
	{
		while (Done == 0);
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
	// Second transfer DDR to OCM with PS DMA
	/******************************************************************************/
	DmaCmd.BD.SrcAddr = (u32)Src;
	DmaCmd.BD.DstAddr = (u32)Dst_OCM;

    // refill source array with pseudo-random numbers
    for (i=0;i<DMA_TRANSFER_SIZE;i++)
    {
    	Src[i] = lfsr_rand();
    }

    // clear destination array
    for (i=0;i<DMA_TRANSFER_SIZE;i++)
    {
    	Dst_OCM[i] = 0x00;
    }

	Done = 0;
	Status = XDmaPs_Start(&Dma, Channel, &DmaCmd, 0);
	XTime_GetTime(&tStart);
	if (Status == XST_SUCCESS)
	{
		while (Done == 0);
	}
	XTime_GetTime(&tEnd);

	u64 et_dma_ocm = tEnd - tStart;
	float et_us_dma_ocm = ((float)((float)et_dma_ocm) / 333333333.3)*1000000;

    // check if write and read arrays are equal
    for (i=0;i<DMA_TRANSFER_SIZE;i++)
    {
    	if (Src[i] != Dst_OCM[i])
    	return XST_FAILURE;
    }

	/******************************************************************************/
	// Third transfer DDR to BRAM with PS DMA
	/******************************************************************************/
	DmaCmd.BD.SrcAddr = (u32)Src;
	DmaCmd.BD.DstAddr = (u32)Dst_BRAM;

    // refill source array with pseudo-random numbers
    for (i=0;i<DMA_TRANSFER_SIZE;i++)
    {
    	Src[i] = lfsr_rand();
    }

    // clear destination array
    for (i=0;i<DMA_TRANSFER_SIZE;i++)
    {
    	Dst_BRAM[i] = 0x00;
    }

	Done = 0;
	Status = XDmaPs_Start(&Dma, Channel, &DmaCmd, 0);
	XTime_GetTime(&tStart);
	if (Status == XST_SUCCESS)
	{
		while (Done == 0);
	}
	XTime_GetTime(&tEnd);

	u64 et_dma_bram = tEnd - tStart;
	float et_us_dma_bram = ((float)((float)et_dma_bram) / 333333333.3)*1000000;

    // check if write and read arrays are equal
    for (i=0;i<DMA_TRANSFER_SIZE;i++)
    {
    	if (Src[i] != Dst_BRAM[i])
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

/***************************************************************************************************************************************************/
																// SetupInterruptSystem
/***************************************************************************************************************************************************/
int SetupInterruptSystem(XScuGic *GicInstancePtr,XDmaPs *DmaPtr)
{
	XScuGic_Config *IntcConfig;

	Xil_ExceptionInit();

	// initialize the interrupt control driver so that it is ready to use
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	Status = XScuGic_CfgInitialize(GicInstancePtr, IntcConfig, IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) return XST_FAILURE;

	// Connect the interrupt controller interrupt handler to the hardware interrupt handling logic in the processor
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler, GicInstancePtr);

	// Connect the device driver handler that will be called when an interrupt for the device occurs
	// The handler defined above performs the specific interrupt processing for the device

	// Connect the Fault ISR
	Status = XScuGic_Connect(GicInstancePtr,XPAR_XDMAPS_0_FAULT_INTR,(Xil_InterruptHandler)XDmaPs_FaultISR,(void *)DmaPtr);
	if (Status != XST_SUCCESS) return XST_FAILURE;

	// Connect the Done ISR for channel 0 of DMA 0
	Status = XScuGic_Connect(GicInstancePtr,XPAR_XDMAPS_0_DONE_INTR_0,(Xil_InterruptHandler)XDmaPs_DoneISR_0,(void *)DmaPtr);
	if (Status != XST_SUCCESS) return XST_FAILURE;

	// Enable the interrupt for the device
	XScuGic_Enable(GicInstancePtr, XPAR_XDMAPS_0_DONE_INTR_0);

	Xil_ExceptionEnable();

	// enable interrupts in the processor
	Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);

	return XST_SUCCESS;
}

void DmaDoneHandler(unsigned int Channel,
		    XDmaPs_Cmd *DmaCmd,
		    void *CallbackRef)
{
	/* done handler */
  	Done = 1;
}

void DmaFaultHandler(unsigned int Channel,
		     XDmaPs_Cmd *DmaCmd,
		     void *CallbackRef)
{
	/* fault handler */

	Error = 1;
}