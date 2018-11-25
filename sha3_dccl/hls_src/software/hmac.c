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

#include <stdio.h>
#include "platform.h"
#include "xbasic_types.h"
#include"xparameters.h"
#include "xgpio.h"
#include "xstatus.h"

#define DEC_TO_BIN_3BIT_HOLDER "%d%d%d"
#define DEC_TO_BIN_3BIT(byte) \
    ((byte & 0x4)>>2), \
    ((byte & 0x2)>>1), \
    (byte & 0x1)
#define DCACHE_ENABLE 1

int main() {
    init_platform();
    //Xil_DCacheDisable();
    /*******************************************
     * Print a welcome message
     */
    xil_printf("Welcome to the SHA3 Test...\n\r");

    /*******************************************
     * Declare variables
     */
    XGpio controlGPIO;
    Xuint32 idleSignals;
    Xuint32 status;
    uint64_t tmp;
    int i, j, pass;

    /*******************************************
     * Initialize the GPIO driver so that it's
     * ready to be used
     */
    status = XGpio_Initialize(&controlGPIO, XPAR_AXI_GPIO_0_DEVICE_ID);

    if (XST_SUCCESS != status) {
        print("GPIO INIT FAILED\n\r");
    }

    /*******************************************
     * Set the direction of port 1 to be inputs
     * Set the direction of port 2 to be outputs
     */
    XGpio_SetDataDirection(&controlGPIO, 1, 0xFFFFFFFF);
    XGpio_SetDataDirection(&controlGPIO, 2, 0x0);

    /*******************************************
     * Check the idle signals before triggering
     * the IPs
     */
    idleSignals = XGpio_DiscreteRead(&controlGPIO, 1);
    xil_printf(
            "Idle signals before applying the trigger are " DEC_TO_BIN_3BIT_HOLDER "\n\r",
            DEC_TO_BIN_3BIT(idleSignals));

    /*******************************************
     * Write the header on the DDR
     */
    uint64_t headerAddress = 0x00100000ull / 8ull; // address of the header
    uint64_t messageOffset = 1ull; // location of the message relative to the header in double words
    uint64_t messageAddress = headerAddress + messageOffset;
    uint64_t messageSizeBytes = 1ull*17ull*8ull + 8ull;
    // number of blocks x number of double words in a block x bytes in a double word
    uint64_t* ddr = (uint64_t *) (0x00100000);
#if DCACHE_ENABLE
    Xil_DCacheFlush();
#endif

    ddr[0] = (messageAddress << 32) | messageSizeBytes; // message address <<32 | message size in bytes
    Xil_DCacheFlush();
    xil_printf("At address 0x%x, header number of blocks is %d  \n\r", ddr + 0,
            ddr[0]);
    xil_printf("At address 0x%x, header message address is 0x%x  \n\r", ddr + 0,
            ddr[0] >> 32);

    /*******************************************
     * Write the message on the DDR
     */
    // first block
    uint64_t input[17] = { 0x157D5B7E4507F66D, 0x9A267476D33831E7,
            0xBB768D4D04CC3438, 0xDA12F9010263EA5F, 0xCAFBDE2579DB2F6B,
            0x58F911D593D5F79F, 0xB05FE3596E3FA80F, 0xF2F761D1B0E57080,
            0x055C118C53E53CDB, 0x63055261D7C9B2B3, 0x9BD90ACC32520CBB,
            0xDBDA2C4FD8856DBC, 0xEE173132A2679198, 0xDAF83007A9B5C515,
            0x11AE49766C792A29, 0x520388444EBEFE28, 0x256FB33D4260439C };

    uint64_t input2[17] = { 0xBA73A9479EE00C63, 0x0000000000000000,
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000 };

    // expected hash
    uint64_t exp[4] = { 0x4CE7C2B935F21FC3, 0x4C5E56D940A555C5,
            0x93872AEC2F896DE4, 0xE68F2A017060F535 };

    // Prepare the input and store in the DRAM
    for (i = 0; i < 17; i++) {
        tmp = 0;
        for (j = 0; j < 8; j++)
            tmp ^= ((input[i] >> j * 8) & 0xFF) << ((7 - j) * 8);
        input[i] = tmp;

        ddr[i + messageOffset] = input[i];
#if DCACHE_ENABLE
        Xil_DCacheFlush();
#endif
        tmp = 0;
        for (j = 0; j < 8; j++)
            tmp ^= ((input2[i] >> j * 8) & 0xFF) << ((7 - j) * 8);
        input2[i] = tmp;

        ddr[i + messageOffset + 17] = input2[i];
#if DCACHE_ENABLE
        Xil_DCacheFlush();
#endif

    }
#if DCACHE_ENABLE
    Xil_DCacheFlush();
#endif

    /*******************************************
     * Give a start pulse to all the IPs
     */
    XGpio_DiscreteWrite(&controlGPIO, 2, 1);
    XGpio_DiscreteWrite(&controlGPIO, 2, 0);

    /*******************************************
     * Wait till all IPs are idle again, i.e. bus
     * transaction is over
     */

    do {
        idleSignals = XGpio_DiscreteRead(&controlGPIO, 1);
    } while ((idleSignals & 1) != 1);
#if DCACHE_ENABLE
    Xil_DCacheFlush();
#endif

    /*******************************************
     * Read the resulted Hash
     * And prepare it
     */
    uint64_t output[4];
    for (i = 0; i < 4; i++) {
        output[i] = ddr[i + messageOffset];
        tmp = 0;
        for (j = 0; j < 8; j++)
            tmp ^= ((output[i] >> j * 8) & 0xFF) << ((7 - j) * 8);
        output[i] = tmp;
    }

    /*******************************************
     * Check the result
     */
    pass = 1;
    for (i = 0; i < 4; i++) {
        if (exp[i] != output[i]) {
            xil_printf("Test fail at word %d\n\r", i);
            xil_printf("out.lower is 0x%x\n\r",
                    (unsigned int) (output[i] & 0x00000000FFFFFFFF));
            xil_printf("out.upper is 0x%x\n\r",
                    (unsigned int) ((output[i] & 0xFFFFFFFF00000000) >> 32));
            pass = 0;
        }
    }
    if (pass == 1) {
        xil_printf("Hooray! Two block test passed\n\r");
    }

    Xil_DCacheFlush();
    xil_printf("Done !\n\r");

    cleanup_platform();
    return 0;
}
