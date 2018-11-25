
#include "keccak_fns.h"


void keccakWrapper(ddrBus* hashBuffer, ddrBus* address, ap_uint<8> nBytes, ap_uint<1> last){
#pragma HLS DATAFLOW
	ddrBus messageBuffer[BUFFER_SIZE] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0 };
#pragma HLS ARRAY_RESHAPE variable=messageBuffer complete dim=1
	memcpy(messageBuffer, address, nBytes);
	keccak(messageBuffer, hashBuffer, last);
}


void sha3_256(ddrBus *dataPort) {
#pragma HLS INTERFACE ap_bus port=dataPort
//#pragma HLS inline region

	/*****************************
	 * Read the header
	 */
	ddrBus header = dataPort[HEADER_ADDRESS];
	ap_uint<32> messageAddress = header.range(63, 32); // offset of double words
	ap_uint<32> messageSizeBytes = header.range(31, 0);  // in bytes
	header = dataPort[HEADER_ADDRESS + 1];
	ap_uint<32> hashAddress = header.range(31, 0); // offset of double words
	ap_uint<32> lastBlock = header.range(63, 32); // last signal (0 or 1)
	header = dataPort[HEADER_ADDRESS + 2];
	ap_uint<2> split = header.range(31, 0); // log2(number of chunks)

	// number of bytes (17*8 for b=1600, split =3)
	ap_uint<8> nBytes = header.range(63,32);
	/*****************************
	 * Declare a message buffer
	 * and a hash buffer and a
	 * last signal
	 */



	ddrBus hashBuffer[HASH_SIZE] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0};
#pragma HLS ARRAY_RESHAPE variable=hashBuffer complete dim=1
	ap_uint<1> halt = 0;
	ap_uint<1> last = 0;

	/*****************************
	 * Start reading the message
	 */
	ap_uint<32> blockOffset = 0;

	for (blockOffset = 0 ; (blockOffset + (nBytes/BYTES_IN_BUFFER)) * BYTES_IN_BUFFER <= messageSizeBytes;  blockOffset += (nBytes/BYTES_IN_BUFFER) ){
#pragma HLS LOOP_TRIPCOUNT MIN = 1 MAX = 10
		keccakWrapper(hashBuffer, dataPort + messageAddress + blockOffset, nBytes, 0);
	}
	keccakWrapper(hashBuffer, dataPort + messageAddress + blockOffset, nBytes, lastBlock.range(0, 0));
	memcpy(dataPort + hashAddress, hashBuffer, HASH_SIZE * sizeof(ddrBus));


}


void top(ddrBus *message, ddrBus* hash, ap_uint<1> last, ap_uint<2> split) {
#pragma HLS INTERFACE ap_fifo PORT=message
#pragma HLS INTERFACE ap_fifo PORT=hash
	keccak(message, hash, last);
}
