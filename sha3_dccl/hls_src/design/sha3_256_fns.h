#include <string.h>
#include <stdio.h>

#include "ap_int.h"



/*****************************
 * My defines
 */
#define CEILING(x,y) (((x) + (y) - 1) / (y))
#define HEADER_ADDRESS  (0x00100000/8)
#define SPONGE_WIDTH    (1600) //in bits
#define DIGEST_SIZE     (256)  //in bits
#define HASH_SIZE       (16) //(DIGEST_SIZE/64)  //in double words
#define CAPACITY        (2*DIGEST_SIZE)
#define RATE            (SPONGE_WIDTH - CAPACITY)
#define BUFFER_WIDTH    (SPONGE_WIDTH/25)  // in bits
#define BUFFER_SIZE     (18) //CEILING(RATE,64)   // ceil(rate/64) in double words
#define NUMBER_BUFFERS  (RATE/BUFFER_WIDTH)  // in buffer width bits
#define LOG2_BUFFER_WIDTH ((sizeof(unsigned long)*8 - 1 - __builtin_clzl((unsigned long)(BUFFER_WIDTH))))
#define NB_ROUNDS        (12 + 2*LOG2_BUFFER_WIDTH)
#define BYTES_IN_BUFFER (BUFFER_WIDTH/8)   // BUFFER_WIDTH/8
#define LOG2_BYTES_IN_BUFFER (sizeof(unsigned long)*8 - 1 - __builtin_clzl((unsigned long)(BYTES_IN_BUFFER)))

typedef ap_uint<64> ddrBus;
typedef ap_uint<BUFFER_WIDTH> ap_uintW;




void pad(ddrBus* messageBuffer, ap_uint<32> messageSizeBytes) {
#pragma HLS inline region
	/*************************************************
	 * Padding functions, pads in bytes
	 */
	ap_uint<32> rate = BUFFER_SIZE * sizeof(ddrBus);
	// find location of first buffer where pad should start

	ap_uint<32> firstBuffer = messageSizeBytes.range(31, LOG2_BYTES_IN_BUFFER); // first un-filled buffer
	ap_uint<32> nextByte = messageSizeBytes.range(LOG2_BYTES_IN_BUFFER - 1, 0);
	messageBuffer[firstBuffer % BUFFER_SIZE].range(nextByte * 8, nextByte * 8) = 1;
	// pad it
	// pad last location of the messageBuffer
	messageBuffer[BUFFER_SIZE - 1].range(BUFFER_WIDTH - 1, BUFFER_WIDTH - 1) = 1;
	// All in  between = 0.
}

