#include <stdio.h>
#include "platform.h"
#include "xbasic_types.h"
#include "xparameters.h"
#include "xgpio.h"
#include "xstatus.h"
#include "xscutimer.h"
#include "keccak_software.h"


#define TEST_CASE_NUMBER  (1)

#define DEC_TO_BIN_3BIT_HOLDER "%d%d%d"
#define DEC_TO_BIN_3BIT(byte) \
    ((byte & 0x4)>>2), \
    ((byte & 0x2)>>1), \
    (byte & 0x1)


#define DCACHE_ENABLE 1
#define DBUG_PRINT (0)

#define NSECS_PER_SEC 		XPAR_CPU_CORTEXA9_0_CPU_CLK_FREQ_HZ/2
#define TIMER_RES_DIVIDER 	40
#define EE_TICKS_PER_SEC 	(NSECS_PER_SEC / TIMER_RES_DIVIDER)
#define TIMER_LOAD_VALUE	0xFFFFFFE

typedef enum {fetch , feed} prngRequest;
typedef enum {software , hardware} keccakPlatform;

/*******************************************
 * Declare global variables for keccak
 */
XGpio controlGpio;
XGpio counterGpio;

void keccakInitialize(){
	/*******************************************
	 * Initialize the GPIO driver so that it's
	 * ready to be used
	 */
	Xuint32 controlGpioStatus;
	controlGpioStatus = XGpio_Initialize(&controlGpio, XPAR_AXI_GPIO_0_DEVICE_ID);

	if (XST_SUCCESS != controlGpioStatus) {
		print("Control GPIO initialization failed\n\r");
	}

	/*******************************************
	 * Set the direction of port 1 to be inputs
	 * Set the direction of port 2 to be outputs
	 */
	XGpio_SetDataDirection(&controlGpio, 1, 0xFFFFFFFF);
	XGpio_SetDataDirection(&controlGpio, 2, 0x0);
	/*******************************************
	 * Initialize the Counter GPIO
	 */
	Xuint32 counterGpioStatus;
	counterGpioStatus = XGpio_Initialize(&counterGpio, XPAR_AXI_GPIO_1_DEVICE_ID);

	if (XST_SUCCESS != counterGpioStatus) {
		print("Counter GPIO initialization failed\n\r");
	}

	/*******************************************
	 * Set the direction of port 1 to be inputs
	 */
	XGpio_SetDataDirection(&controlGpio, 1, 0xFFFFFFFF);
}


void keccakSoftware(uint64_t messageAddress, uint64_t messageSizeBytes, uint64_t hashAddress, uint64_t lastBlock, uint64_t split, uint64_t nBytes ){
	Xil_DCacheFlush();
	uint64_t hashBuffer[16];
	uint8_t bytesInBuffer   = 8;
	uint8_t offsetIncrement = nBytes/bytesInBuffer;
	uint64_t blockOffset = 0;
	uint8_t loopIndex;


	for (blockOffset = 0 ; (blockOffset + offsetIncrement) * bytesInBuffer <= messageSizeBytes;  blockOffset += offsetIncrement ){
		uint64_t messageBuffer[17] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0 };
		memcpy(messageBuffer, (uint64_t *) (messageAddress + blockOffset*8), nBytes);
#if DEBUG_PRINT
		printf("messageAddress =0x%x \n\r", (unsigned int)(messageAddress+ blockOffset));
		for (loopIndex = 0; loopIndex<16; loopIndex++){
			printf("message[%d] is 0x%x  \n\r", loopIndex,
					(unsigned int) messageBuffer[loopIndex]);
			printf("message[%d] is 0x%x  \n\r", loopIndex,
					(unsigned int) (messageBuffer[loopIndex]>>32));
		}
#endif
		keccak(messageBuffer, hashBuffer, 0);
	}

	uint64_t messageBuffer[17] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0 };
	memcpy(messageBuffer, (uint64_t *) (messageAddress + blockOffset*8), nBytes);
#if DEBUG_PRINT
	printf("messageAddress =0x%x \n\r", (unsigned int)(messageAddress+ blockOffset));
	for (loopIndex = 0; loopIndex<16; loopIndex++){
		printf("message[%d] is 0x%x  \n\r", loopIndex,
				(unsigned int) messageBuffer[loopIndex]);
		printf("message[%d] is 0x%x  \n\r", loopIndex,
				(unsigned int) (messageBuffer[loopIndex]>>32));
	}
#endif
	keccak(messageBuffer, hashBuffer, lastBlock);

	memcpy((uint64_t *)hashAddress,  hashBuffer, 4 * sizeof(uint64_t));
	Xil_DCacheFlush();


}

void keccakRun(uint64_t* headerAddress, uint64_t messageAddress, uint64_t messageSizeBytes, uint64_t hashAddress, uint64_t lastBlock, uint64_t split, uint64_t nBytes ){
	Xil_DCacheFlush();
	headerAddress[0] = (messageAddress << 32) | messageSizeBytes; // message address <<32 | message size in bytes
	headerAddress[1] = (lastBlock << 32)      | hashAddress ; // lastBloc | hash address
	headerAddress[2] = (nBytes << 32)         | split; // nBytes | split
	Xil_DCacheFlush();
#if DEBUG_PRINT
	xil_printf("At address 0x%x, header number of blocks is %d  \n\r", headerAddress + 0,
			headerAddress[0]);
	xil_printf("At address 0x%x, header message address is 0x%x  \n\r", headerAddress + 0,
			headerAddress[0] >> 32);
	xil_printf("At address 0x%x, header hash address is 0x%x  \n\r", headerAddress + 1,
			headerAddress[1] );
	xil_printf("At address 0x%x, last block signal is 0x%x  \n\r", headerAddress + 1,
			headerAddress[1] >>32 );
	xil_printf("At address 0x%x, split is 0x%x  \n\r", headerAddress + 2,
			headerAddress[2]);
	xil_printf("At address 0x%x, number of bytes is %d  \n\r", headerAddress + 2,
			headerAddress[2]>>32);
#endif
	Xuint32 idleSignals;
	/*******************************************
	 * Check the idle signals before triggering
	 * the keccak
	 */
	idleSignals = XGpio_DiscreteRead(&controlGpio, 1);
#if DEBUG_PRINT
	xil_printf( "Idle signals before applying the trigger are " DEC_TO_BIN_3BIT_HOLDER "\n\r",
			DEC_TO_BIN_3BIT(idleSignals));
#endif
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
	/*******************************************
	 * Display the number of cycles
	 */
	uint32_t cycles = XGpio_DiscreteRead(&counterGpio, 1);
#if DEBUG_PRINT
	xil_printf("Keccak ran, cycles taken = %d  \n\r", cycles);
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
	uint64_t messageOffset = 3ull; // location of the message relative to the header in double words
	uint64_t hashOffset    = 500ull; // location of the hash relative to the header in double words
	uint64_t messageAddress = headerAddress + messageOffset;
	uint64_t hashAddress    = headerAddress + hashOffset;
	uint64_t messageSizeBytes = 1ull*17ull*8ull + 8ull;
	// number of blocks x number of double words in a block x bytes in a double word
	uint64_t* ddr = (uint64_t *) (0x00100000);

#if TEST_CASE_NUMBER == 0
	// expected hash
	uint64_t exp[4] = { 0x4CE7C2B935F21FC3, 0x4C5E56D940A555C5,
			0x93872AEC2F896DE4, 0xE68F2A017060F535 };
	uint64_t capacity = 512;
	uint64_t rate     = 1600;
	uint32_t split    = 3;
#elif TEST_CASE_NUMBER == 1
	uint64_t exp[4] = { 0x6D735FB7579135F6, 0x1B771B2BB0D81514,
	 	 	 	 	 	0xCDE9C977ACCF6FEA, 0xF6EDEBF0C2A7E6AF};
	uint64_t capacity = 448;
	uint64_t rate     = 1600;
	uint32_t split    = 3;
#elif TEST_CASE_NUMBER == 2
	uint64_t exp[6] = { 0x511B13E53FD353FA, 0x4D38EF0CF8F1AF30,
	 	 	 	 	 	0xDA554828A5FD1C53, 0xEC41F73D9ACA6C54,
	 	 	 	 	 	0xAC7972C933AF4A2F, 0xC7AB852CA63A1BA6};
	uint64_t capacity = 768;
	uint64_t rate     = 1600;
	uint32_t split    = 3;
#elif TEST_CASE_NUMBER == 3
	uint64_t exp[8] = { 0x72DE9184BEB5C6A3, 0x7EA2C395734D0D54,
						0x12991A57CFFCC13F, 0xF9B5FA0F2046EE87,
						0xC61811FE8EF24702, 0x39D5066C220173DE,
						0x5EBE41885ED8ACAE, 0x397FB395E6CA9AEE};
	uint64_t capacity = 1024;
	uint64_t rate     = 1600;
	uint32_t split    = 3;
#elif TEST_CASE_NUMBER == 4
	uint64_t exp[8] = { 0x3B034EDC00000000, 0x8D4A7B8A00000000,
						0xEF14A78E00000000, 0x15B05F1E00000000,
						0x70FAB1D500000000, 0x0BC2BAD500000000,
						0x4FDC2FC800000000, 0x1DF1C15200000000};
	uint64_t capacity = 512;
	uint64_t rate     = 800;
	uint32_t split    = 2;
	messageSizeBytes *=2;
#endif

	uint64_t nBytes  = (rate-capacity)/(1<<split);



	/*******************************************
	 * Write the message on the DDR
	 */

	uint64_t message[36] = {
			0x157D5B7E4507F66D, 0x9A267476D33831E7, 0xBB768D4D04CC3438,
			0xDA12F9010263EA5F, 0xCAFBDE2579DB2F6B, 0x58F911D593D5F79F,
			0xB05FE3596E3FA80F, 0xF2F761D1B0E57080, 0x055C118C53E53CDB,
			0x63055261D7C9B2B3, 0x9BD90ACC32520CBB, 0xDBDA2C4FD8856DBC,
			0xEE173132A2679198, 0xDAF83007A9B5C515, 0x11AE49766C792A29,
			0x520388444EBEFE28, 0x256FB33D4260439C, 0xBA73A9479EE00C63,
			0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
			0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
			0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
			0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
			0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
			0x0000000000000000, 0x0000000000000000, 0x0000000000000000};

	/*******************************************
	 * Pad the message
	 */
#if TEST_CASE_NUMBER == 0
	message[18] = 0x0100000000000000ull;
	message[33] = 0x0000000000000080ull;
#elif TEST_CASE_NUMBER == 1
	message[18] = 0x0100000000000000ull;
	message[35] = 0x0000000000000080ull;
#elif TEST_CASE_NUMBER == 2
	message[18] = 0x0100000000000000ull;
	message[25] = 0x0000000000000080ull;
#elif TEST_CASE_NUMBER == 3
	message[18] = 0x0100000000000000ull;
	message[26] = 0x0000000000000080ull;
#elif TEST_CASE_NUMBER == 4
	message[18] = 0x0100000000000000ull;
	message[22] = 0x0000008000000000ull;
#endif

	// Prepare the input and store in the DRAM

	uint64_t input[36*8];
	int idx = 0;
	for (i = 0; i < 36; i++) {

		tmp = 0;
		for (j = 0; j < 8; j++){
			tmp ^= ((message[i] >> j * 8) & 0xFF) << ((7 - j) * 8);
		}
		int k;
		for (k = 0; k < 8/(1<<split); k++){
			uint32_t shift = (((8/(1<<split)) -1 - k) * 8 * (1<<split));
			uint32_t invShift = 64 - (8 * (1<<split));
			uint64_t store;
			store = tmp << shift;
			input[idx + k] =  store >> invShift ;
		}
		idx += 8/(1<<split);
	}

	for (i = 0; i < 36*8/(1<<split); i++){
		ddr[i + messageOffset] = input[i];
#if DCACHE_ENABLE
		Xil_DCacheFlush();
#endif
		//xil_printf("ddr[%d] = 0x%x  \n\r", i, ddr[i + messageOffset]);
	}

#if DCACHE_ENABLE
	Xil_DCacheFlush();
#endif

	/*******************************************
	 * Initialize and run the keccak
	 */
	keccakInitialize();
	keccakRun(ddr, messageAddress, messageSizeBytes, hashAddress, 1ull, split, nBytes);

	/*******************************************
	 * Read the resulted Hash
	 * And prepare it
	 */
	uint64_t hash_buffers = capacity/(2*64);
	uint64_t output[hash_buffers];
	uint64_t result;
	for (i = 0; i < hash_buffers; i++) {
		result = ddr[i + hashOffset];
		tmp = 0;
		for (j = 0; j < 8; j++)
			tmp ^= ((result >> j * 8) & 0xFF) << ((7 - j) * 8);
		output[i] = tmp;
	}


	/*******************************************
	 * Check the result
	 */
	pass = 1;
	for (i = 0; i < hash_buffers; i++) {
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

void hmacTest(keccakPlatform hmacType, uint64_t size){

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
	uint8_t* ddr = (uint8_t *) (0x00100000);
	uint64_t key0Offset = 0x00100000ull;
	uint64_t keyIpadOffset = 0x00101000ull;
	uint64_t keyOpadOffset = 0x00102000ull;
	uint64_t textOffset    = 0x00100000ull;

	for (loopIndex = 0; loopIndex < 0xFFFull; loopIndex++ ){
		ddr[keyOpadOffset + loopIndex] = 0;
		ddr[keyIpadOffset + loopIndex] = 0;
	}
	Xil_DCacheFlush();
	/*******************************************
	 * Prepare the key
	 */
	for (loopIndex = 0; loopIndex < keySize; loopIndex ++){
		ddr[key0Offset+loopIndex] = key[loopIndex/8] >>(8*(loopIndex%8))   ;
	}
	Xil_DCacheFlush();
	/*******************************************
	 * Prepare the text
	 */
	uint32_t textSizeBytes = size;//17*8;
	uint64_t input[17] = { 0x157D5B7E4507F66D, 0x9A267476D33831E7,
			0xBB768D4D04CC3438, 0xDA12F9010263EA5F, 0xCAFBDE2579DB2F6B,
			0x58F911D593D5F79F, 0xB05FE3596E3FA80F, 0xF2F761D1B0E57080,
			0x055C118C53E53CDB, 0x63055261D7C9B2B3, 0x9BD90ACC32520CBB,
			0xDBDA2C4FD8856DBC, 0xEE173132A2679198, 0xDAF83007A9B5C515,
			0x11AE49766C792A29, 0x520388444EBEFE28, 0x256FB33D4260439C };

	for(loopIndex = 0; loopIndex < textSizeBytes; loopIndex++){
		ddr[textOffset+loopIndex] = input[loopIndex/8] >>(8*(loopIndex%8))   ;
	}

	/*******************************************
	* Initialize the software timer
	*/
	uint32_t timer = 0;
	uint32_t timerIntrId = 0;
	XScuTimer_Config    *configPtr;
	XScuTimer           *timerInstancePtr = &timer;

	configPtr = XScuTimer_LookupConfig(timerIntrId);
	XScuTimer_CfgInitialize(timerInstancePtr, configPtr, configPtr->BaseAddr);
	XScuTimer_SetPrescaler(timerInstancePtr, TIMER_RES_DIVIDER);
	XScuTimer_LoadTimer(timerInstancePtr, TIMER_LOAD_VALUE);
	XScuTimer_Start(timerInstancePtr);
	uint32_t timerStartValue = XScuTimer_GetCounterValue(timerInstancePtr);

	/*******************************************
	 * Initialize the keccak
	 */
	if (hmacType == hardware){
		keccakInitialize();
	}


	if (keySize >blockSize ){
		// TODO
		// Place the key in the DDR
		// Run keccak on it
	}else if (keySize < blockSize){
		// Append zeros to k
		for (loopIndex = keySize; loopIndex<blockSize; loopIndex++){
			ddr[key0Offset+loopIndex] = 0x00;
		}
		Xil_DCacheFlush();
	}

	// key0 XOR ipad
	// key0 XOR Opad
	for (loopIndex = 0; loopIndex<blockSize; loopIndex++){
		ddr[keyIpadOffset+loopIndex] = ddr[key0Offset+loopIndex]  ^ 0x36;
		ddr[keyOpadOffset+loopIndex] = ddr[key0Offset+loopIndex]  ^ 0x5C;
	}
	Xil_DCacheFlush();



	// move keyIpad ahead of text starting from the last byte
	for (loopIndex = 0; loopIndex < blockSize; loopIndex++){
		ddr[textOffset-1-loopIndex] = ddr[keyIpadOffset+blockSize - loopIndex -1];
	}
	Xil_DCacheFlush();

#if DEBUG_PRINT == 1
	for (loopIndex = 0; loopIndex<blockSize+textSizeBytes; loopIndex++){
		xil_printf("At address 0x%x, keyIpad||text is 0x%x  \n\r", text - blockSize + loopIndex,
				text[- blockSize + loopIndex]);
	}*/
#endif


	// Apply keccak
	uint64_t* headerAddress    = (uint64_t *) ddr;
	uint64_t  messageAddress   = (0x00100000ull+textOffset -blockSize)/8;
	uint64_t  messageSizeBytes =  blockSize + textSizeBytes;
	uint64_t  hashAddress      = (0x00100000ull+keyOpadOffset + blockSize)/8; // Append hashed result after the keyOpad

	Xil_DCacheFlush();

	if (hmacType == software){
		keccakSoftware( messageAddress*8, messageSizeBytes, hashAddress*8, 1, 3, blockSize);
	}else {
		keccakRun(headerAddress, messageAddress, messageSizeBytes, hashAddress, 1ull, 3, blockSize);
	}


	// Apply keccak
	messageAddress   = (0x00100000ull+keyOpadOffset )/8;
	messageSizeBytes =  blockSize + hashSizeBytes;
	Xil_DCacheFlush();

	// Clear garbage data from the accelerator
	if (hmacType == hardware){
		for (loopIndex = messageSizeBytes/8; loopIndex<messageSizeBytes/4; loopIndex++){
				headerAddress[keyOpadOffset/8 + loopIndex] = 0;
		}
	}

#if DEBUG_PRINT == 1
	printf("messageAddress =0x%x \n\r",(unsigned int)messageAddress);
	for (loopIndex = 0; loopIndex<(2*blockSize + hashSizeBytes)/4; loopIndex++){
		printf("message passed[%d] is 0x%x  \n\r", (int)loopIndex,
				(unsigned int )headerAddress[keyOpadOffset/8 + loopIndex]);
	}
#endif

	if (hmacType == software){
		keccakSoftware( messageAddress*8, messageSizeBytes, hashAddress*8, 1, 3, blockSize);
	}else {
		keccakRun(headerAddress, messageAddress, messageSizeBytes, hashAddress, 1ull, 3, blockSize);
	}
	/*******************************************
	 * Check the result
	 */
#if DEBUG_PRINT == 1
	printf("At address 0x%x = 0x%x, HMAC is 0x%x  \n\r"
			, (unsigned int) hashAddress ,(unsigned int) ( ddr+hashAddress*8 - 0x00100000ull)/8, (unsigned int) ddr[hashAddress*8 - 0x00100000ull]);
#endif

	Xil_DCacheFlush();
	uint32_t timerStopValue = XScuTimer_GetCounterValue(timerInstancePtr);
	double executionTime  = (timerStartValue- timerStopValue)*1000.0 /EE_TICKS_PER_SEC;
	printf ("Execution time is %f ms \n\r", executionTime);
	xil_printf("HMAC done !\n\r");
	printf("At address 0x%x = 0x%x, HMAC is 0x%x  \n\r"
				, (unsigned int) hashAddress ,(unsigned int) ( ddr+hashAddress*8 - 0x00100000ull)/8, (unsigned int) ddr[hashAddress*8 - 0x00100000ull]);
}




void prng(keccakPlatform systemType, prngRequest request, uint64_t* seed, uint32_t seedSize, uint32_t lengthOutBytes, uint8_t* result, double* execTime){


	/*******************************************
	* Initialize the software timer
	*/
	uint32_t timer = 0;
	uint32_t timerIntrId = 0;
	XScuTimer_Config    *configPtr;
	XScuTimer           *timerInstancePtr = &timer;

	configPtr = XScuTimer_LookupConfig(timerIntrId);
	XScuTimer_CfgInitialize(timerInstancePtr, configPtr, configPtr->BaseAddr);
	XScuTimer_SetPrescaler(timerInstancePtr, TIMER_RES_DIVIDER);
	XScuTimer_LoadTimer(timerInstancePtr, TIMER_LOAD_VALUE);
	XScuTimer_Start(timerInstancePtr);
	uint32_t timerStartValue = XScuTimer_GetCounterValue(timerInstancePtr);

	/*******************************************
	 * Initialize the keccak
	 */
	if (systemType == hardware){
		keccakInitialize();
	}




	uint32_t m = 0;
	uint64_t rateBytes = 16*8;


	uint8_t* ddr   = (uint8_t *) (0x00100000);
	uint8_t* hashS = (uint8_t *) (0x00200000);
	uint64_t* headerAddress    = (uint64_t *) ddr;
	uint64_t  messageAddress   = (uint32_t ) seed;
	uint64_t  messageSizeBytes = seedSize;
	uint64_t  hashAddress      = (uint32_t) hashS;
	uint64_t  blockSize        = rateBytes;
	Xil_DCacheFlush();

	if (request == feed) { // feed
		// Apply keccak
		if (systemType == software){
			keccakSoftware( messageAddress*8, messageSizeBytes, hashAddress*8, 1, 3, blockSize);
		}else {
			keccakRun(headerAddress, messageAddress, messageSizeBytes, hashAddress, 1, 3, blockSize);
		}
		m = 0;
		uint8_t* hashPtrBytes = (uint8_t*) hashS;
		memcpy(result, hashPtrBytes, rateBytes);

	}else{  // fetch
		uint32_t a;
		if (m ==0){
			a = rateBytes;
		} else {
			a = (-m) % rateBytes;
		}

		while (lengthOutBytes > 0 ){
			if (a == 0 ){
				messageAddress   = (uint32_t ) hashS;
				if (systemType == software){
					keccakSoftware( messageAddress*8, messageSizeBytes, hashAddress*8, 1, 3, blockSize);
				}else {
					keccakRun(headerAddress, messageAddress, messageSizeBytes, hashAddress, 1, 3, blockSize);
				}
				a = rateBytes;
			}
			uint32_t lengthOutHat = (a < lengthOutBytes)? a: lengthOutBytes;
			uint8_t* hashPtrBytes = (uint8_t*) hashS;
			memcpy(result+m+rateBytes-a, hashPtrBytes+m+rateBytes-a, lengthOutHat);
			//printf("Start = %d, end = %d \n\r",(int)(rateBytes-a), (int)(rateBytes-a+lengthOutHat-1));
			a              -= lengthOutHat;
			lengthOutBytes -= lengthOutHat;
			m              += lengthOutHat;
		}

	}
	Xil_DCacheFlush();
	uint32_t timerStopValue = XScuTimer_GetCounterValue(timerInstancePtr);
	double executionTime  = (timerStartValue- timerStopValue)*1000.0 /EE_TICKS_PER_SEC;
	*execTime = executionTime;

}

