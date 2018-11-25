

#include "sha3_256_fns.h"

void sha3_256(ddrBus *dataPort) {
#pragma HLS INTERFACE m_axi port=dataPort
#pragma HLS inline region

	/*****************************
	 * Read the header
	 */
	ddrBus header = dataPort[HEADER_ADDRESS];
	ap_uint<32> messageAddress = header.range(63, 32); // offset of double words
	ap_uint<32> messageSizeBytes = header.range(31, 0);  // in bytes
	header = dataPort[HEADER_ADDRESS+1];
	ap_uint<32> hashAddress = header.range(31, 0); // offset of double words
	ap_uint<32> lastBlock   = header.range(63, 32); // last signal (0 or 1)
	/*****************************
	 * Declare a message buffer
	 * and a hash buffer and a
	 * last signal
	 */

	ddrBus hashBuffer[HASH_SIZE];
	ap_uint<1> halt = 0;
	ap_uint<1> last = 0;

	/*****************************
	 * Start reading the message
	 */
	ap_uint<32> blockOffset = 0;
	while (!halt) {
		ddrBus messageBuffer[BUFFER_SIZE];
		// memcopy the block
		memcpy(messageBuffer, dataPort + messageAddress + blockOffset,
		BUFFER_SIZE * sizeof(ddrBus));
		// is it the last block ?
		if ((blockOffset + BUFFER_SIZE) * sizeof(ddrBus) < messageSizeBytes) {
			halt = 0;
			last = 0;
		} else {
			// if not do the padding
			halt = 1;
			last = lastBlock.range(0,0);
			//pad(messageBuffer, messageSizeBytes);
		}
		// pass it to kernel
		keccak(messageBuffer, hashBuffer, last);
		// update block address
		blockOffset = blockOffset + BUFFER_SIZE;
	}
	memcpy(dataPort + hashAddress, hashBuffer, HASH_SIZE * sizeof(ddrBus));

}
