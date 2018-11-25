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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * XILINX CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
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

#include "hash_fns.h"



#define SHA3_TEST 0
#define HMAC_TEST 0
#define PRNG_TEST 1



int main() {
    init_platform();
    //Xil_DCacheDisable();
#if SHA3_TEST
    sha3Test();
#endif

#if HMAC_TEST
    keccakPlatform hmacType = hardware;
    //keccakPlatform hmacType = software;
    xil_printf("HMAC software test !\n\r");
    uint32_t textSize = 4*256; //17*8;
    hmacTest(hmacType, textSize);
#endif

#if PRNG_TEST
    printf ("PRNG start !\n\r");
    double execTime = 0;
    double* execTimePtr = &execTime;
    double temp =0;
    keccakPlatform systemType = software;
	uint64_t seed[2*16] = { 0x4CE7C2B935F21FC3, 0x4C5E56D940A555C5,
			0x93872AEC2F896DE4, 0xE68F2A017060F535 };
	uint32_t seedSize  = 4*256; //4*8
	uint32_t rateBytes = 16*8;
	uint32_t lengthOutBytes = rateBytes + rateBytes/2;
	uint8_t  result[2*16*8] = {0};
	prngRequest request     = feed;
    prng(systemType, request, seed, seedSize, lengthOutBytes, result, execTimePtr);
    temp += execTime;
    // XOR old result with the next seed for re-feed
    int j = 0;
    for (j = 0; j < 2*16*8 ; j++){
    	*(((uint8_t *)seed) + j) = result[j] ^ *(((uint8_t *)seed) + j);
    }
    prng(systemType, request, seed, 2*16*8, lengthOutBytes, result, execTimePtr);
    temp += execTime;
#if DEBUG_PRINT
    int i = 0;
    for (i = 0; i< 17*8; i++){
    	printf("Result[%d] = %d \n\r", i, result[i]);
    }
#endif
    request       = fetch;
    prng(systemType, request, (uint64_t*) result, 16*8, lengthOutBytes, result, execTimePtr);
    temp += execTime;
    printf ("Execution time is %f ms \n\r", temp);
    printf ("PRNG done !\n\r");
#if DEBUG_PRINT
    for (i = 0; i< lengthOutBytes; i++){
    	printf("Result[%d] = %d \n\r", i, result[i]);
    }
#endif // End of debug
#endif // end of PRNG
    cleanup_platform();
    return 0;
}
