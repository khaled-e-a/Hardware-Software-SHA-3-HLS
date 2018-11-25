
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
#pragma HLS INTERFACE m_axi port=dataPort
//#pragma HLS inline region

	/*****************************
	 * Read the header
	 */
	ddrBus header = dataPort[HEADER_ADDRESS];
	ap_uint<32> hashAddress = header.range(31, 0); // offset of double words
	ap_uint<32> lastBlock   = header.range(63, 32); // last signal (0 or 1)
	header = dataPort[HEADER_ADDRESS + 1];
	ap_uint<2> split = header.range(31, 0); // log2(number of chunks)
	ap_uint<8> nBytes = header.range(63,32);
	header = dataPort[HEADER_ADDRESS + 2];
	ap_uint<32> messageAddress0   = header.range(63, 32); // offset of double words
	ap_uint<32> messageSizeBytes0 = header.range(31, 0);  // in bytes
	ap_uint<32> messageAddress1, messageAddress2, messageAddress3,
				messageAddress4, messageAddress5, messageAddress6,
				messageAddress7;
	ap_uint<32> messageSizeBytes1, messageSizeBytes2, messageSizeBytes3,
				messageSizeBytes4, messageSizeBytes5, messageSizeBytes6,
				messageSizeBytes7;
	/*********************************************************
	 * If split is < 3, then read the following message info
	 */
	if (split < 3){
		header = dataPort[HEADER_ADDRESS + 3];
		messageAddress1   = header.range(63, 32);
		messageSizeBytes1 = header.range(31, 0);
	}
	if (split < 2){
		header = dataPort[HEADER_ADDRESS + 4];
		messageAddress2   = header.range(63, 32);
		messageSizeBytes2 = header.range(31, 0);
		header = dataPort[HEADER_ADDRESS + 5];
		messageAddress3   = header.range(63, 32);
		messageSizeBytes3 = header.range(31, 0);
	}
	if (split < 1){
		header = dataPort[HEADER_ADDRESS + 6];
		messageAddress4   = header.range(63, 32);
		messageSizeBytes4 = header.range(31, 0);
		header = dataPort[HEADER_ADDRESS + 7];
		messageAddress5   = header.range(63, 32);
		messageSizeBytes5 = header.range(31, 0);
		header = dataPort[HEADER_ADDRESS + 8];
		messageAddress6   = header.range(63, 32);
		messageSizeBytes6 = header.range(31, 0);
		header = dataPort[HEADER_ADDRESS + 9];
		messageAddress7   = header.range(63, 32);
		messageSizeBytes7 = header.range(31, 0);
	}
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
