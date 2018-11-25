#include <stdio.h>
#include "platform.h"
#include "xbasic_types.h"
#include "xparameters.h"
#include "xgpio.h"
#include "xstatus.h"


#define DEC_TO_BIN_3BIT_HOLDER "%d%d%d"
#define DEC_TO_BIN_3BIT(byte) \
    ((byte & 0x4)>>2), \
    ((byte & 0x2)>>1), \
    (byte & 0x1)


#define DCACHE_ENABLE 1

typedef enum {fetch , feed} prngRequest;

/*******************************************
 * Declare global variables for keccak
 */
XGpio controlGpio;
Xuint32 gpioStatus;

void keccakInitialize(){
	/*******************************************
	 * Initialize the GPIO driver so that it's
	 * ready to be used
	 */
	gpioStatus = XGpio_Initialize(&controlGpio, XPAR_AXI_GPIO_0_DEVICE_ID);

	if (XST_SUCCESS != gpioStatus) {
		print("GPIO INIT FAILED\n\r");
	}

	/*******************************************
	 * Set the direction of port 1 to be inputs
	 * Set the direction of port 2 to be outputs
	 */
	XGpio_SetDataDirection(&controlGpio, 1, 0xFFFFFFFF);
	XGpio_SetDataDirection(&controlGpio, 2, 0x0);
}

void keccakRun(uint64_t* headerAddress, uint64_t messageAddress, uint64_t messageSizeBytes, uint64_t hashAddress, uint64_t lastBlock ){
	Xil_DCacheFlush();
	headerAddress[0] = (messageAddress << 32) | messageSizeBytes; // message address <<32 | message size in bytes
	headerAddress[1] = (lastBlock << 32)      | hashAddress ; // hash address
	Xil_DCacheFlush();
	xil_printf("At address 0x%x, header number of blocks is %d  \n\r", headerAddress + 0,
			headerAddress[0]);
	xil_printf("At address 0x%x, header message address is 0x%x  \n\r", headerAddress + 0,
			headerAddress[0] >> 32);
	xil_printf("At address 0x%x, header hash address is 0x%x  \n\r", headerAddress + 1,
			headerAddress[1] );
	xil_printf("At address 0x%x, last block signal is 0x%x  \n\r", headerAddress + 1,
			headerAddress[1] >>32 );

	Xuint32 idleSignals;
	/*******************************************
	 * Check the idle signals before triggering
	 * the keccak
	 */
	idleSignals = XGpio_DiscreteRead(&controlGpio, 1);
	xil_printf( "Idle signals before applying the trigger are " DEC_TO_BIN_3BIT_HOLDER "\n\r",
			DEC_TO_BIN_3BIT(idleSignals));
	/*******************************************
	 * Give a start pulse to all the keccak
	 */
	XGpio_DiscreteWrite(&controlGpio, 2, 1);
	XGpio_DiscreteWrite(&controlGpio, 2, 0);

	/*******************************************
	 * Wait till the keccak is idle
	 */

	do {
		idleSignals = XGpio_DiscreteRead(&controlGpio, 1);
	} while ((idleSignals & 1) != 1);
#if DCACHE_ENABLE
	Xil_DCacheFlush();
#endif
}

void sha3Test(){
	/*******************************************
	 * Print a welcome message
	 */
	xil_printf("Welcome to the SHA3 Test...\n\r");

	/*******************************************
	 * Declare variables
	 */
	uint64_t tmp;
	int i, j, pass;


	/*******************************************
	 * Write the header on the DDR
	 */
	uint64_t headerAddress = 0x00100000ull / 8ull; // address of the header
	uint64_t messageOffset = 2ull; // location of the message relative to the header in double words
	uint64_t hashOffset    = 50ull; // location of the hash relative to the header in double words
	uint64_t messageAddress = headerAddress + messageOffset;
	uint64_t hashAddress    = headerAddress + hashOffset;
	uint64_t messageSizeBytes = 1ull*17ull*8ull + 8ull;
	// number of blocks x number of double words in a block x bytes in a double word
	uint64_t* ddr = (uint64_t *) (0x00100000);


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

	uint64_t input2[17] = { 0xBA73A9479EE00C63, 0x0100000000000000,
			0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
			0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
			0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
			0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
			0x0000000000000000, 0x0000000000000000, 0x0000000000000080 };

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
	 * Initialize and run the keccak
	 */
	keccakInitialize();
	keccakRun(ddr, messageAddress, messageSizeBytes, hashAddress, 1ull);

	/*******************************************
	 * Read the resulted Hash
	 * And prepare it
	 */
	uint64_t output[4];
	for (i = 0; i < 4; i++) {
		output[i] = ddr[i + hashOffset];
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
		xil_printf("Two block test passed, Every thing is awesome! \n\r");
	}

	Xil_DCacheFlush();
	xil_printf("SHA3 test done !\n\r");

}

void hmacTest(){

	uint32_t loopIndex;
	// 256-bit Key splitted into 4 double words
	uint64_t key[4] = { 0x4CE7C2B935F21FC3, 0x4C5E56D940A555C5,
			0x93872AEC2F896DE4, 0xE68F2A017060F535 };
	uint32_t keySize   = 4*8; // In bytes
	uint32_t blockSize = 256/8; // B in bytes (must be > 256/8 because used hash is SHA3-256 )
	uint32_t hashSizeBytes = 4*8;
	/*******************************************
	 * Declare the byte addressable memory arrays
	 */
	uint8_t* ddr  = (uint8_t *) (0x00100000);
	uint8_t* key0 = (uint8_t *) (0x00200000);
	uint8_t* keyIpad = (uint8_t *) (0x00201000);
	uint8_t* keyOpad = (uint8_t *) (0x00202000);
	uint8_t* text    = (uint8_t *) (0x00300000);

	/*******************************************
	 * Prepare the key
	 */
	for (loopIndex = 0; loopIndex < keySize; loopIndex ++){
		key0[loopIndex] = key[loopIndex/8] >>(8*(loopIndex%8))   ;
	}
	Xil_DCacheFlush();
	/*******************************************
	 * Prepare the text
	 */
	uint32_t textSizeBytes = 17*8;
	uint64_t input[17] = { 0x157D5B7E4507F66D, 0x9A267476D33831E7,
			0xBB768D4D04CC3438, 0xDA12F9010263EA5F, 0xCAFBDE2579DB2F6B,
			0x58F911D593D5F79F, 0xB05FE3596E3FA80F, 0xF2F761D1B0E57080,
			0x055C118C53E53CDB, 0x63055261D7C9B2B3, 0x9BD90ACC32520CBB,
			0xDBDA2C4FD8856DBC, 0xEE173132A2679198, 0xDAF83007A9B5C515,
			0x11AE49766C792A29, 0x520388444EBEFE28, 0x256FB33D4260439C };
	for(loopIndex = 0; loopIndex < textSizeBytes; loopIndex++){
		text[loopIndex] = input[loopIndex/8] >>(8*(loopIndex%8))   ;
	}
	/*******************************************
	 * Initialize the keccak
	 */
	keccakInitialize();

	if (keySize >blockSize ){
		// TODO
		// Place the key in the DDR
		// Run keccak on it
	}else if (keySize < blockSize){
		// Append zeros to k
		for (loopIndex = keySize; loopIndex<blockSize; loopIndex++){
			key0[loopIndex] = 0x00;
		}
		Xil_DCacheFlush();
	}

	// key0 XOR ipad
	// key0 XOR Opad
	for (loopIndex = 0; loopIndex<blockSize; loopIndex++){
		keyIpad[loopIndex] = key0[loopIndex]  ^ 0x36;
		keyOpad[loopIndex] = key0[loopIndex]  ^ 0x5C;
	}
	Xil_DCacheFlush();



	// move keyIpad ahead of text starting from the last byte
	for (loopIndex = 0; loopIndex < blockSize; loopIndex++){
		text[-1-loopIndex] = keyIpad[blockSize - loopIndex -1];
	}
	Xil_DCacheFlush();

	// Debug prints
	for (loopIndex = 0; loopIndex<blockSize+textSizeBytes; loopIndex++){
		xil_printf("At address 0x%x, keyIpad||text is 0x%x  \n\r", text - blockSize + loopIndex,
				text[- blockSize + loopIndex]);
	}


	// Apply keccak
	uint64_t* headerAddress    = (uint64_t *) ddr;
	uint64_t  messageAddress   = (((uintptr_t)text) -blockSize)/8;
	uint64_t  messageSizeBytes =  blockSize + textSizeBytes;
	uint64_t  hashAddress      = (((uintptr_t)keyOpad ) + blockSize)/8; // Append hashed result after the keyOpad
	Xil_DCacheFlush();
	keccakRun(headerAddress, messageAddress, messageSizeBytes, hashAddress, 1ull);
	// Apply keccak
	messageAddress   = ((uintptr_t)keyOpad )/8;
	messageSizeBytes =  2*blockSize + hashSizeBytes;
	Xil_DCacheFlush();
	keccakRun(headerAddress, messageAddress, messageSizeBytes, hashAddress, 1ull);

	/*******************************************
	 * Check the result
	 */


	Xil_DCacheFlush();
	xil_printf("HMAC done !\n\r");

}



/*
void prng(prngRequest request, ){
	uint32_t mode = 0;

	Xil_DCacheFlush();
	xil_printf("PRNG done !\n\r");
}
*/
