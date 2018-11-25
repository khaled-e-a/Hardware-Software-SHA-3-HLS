
#include "sha3_256_fns.h"
#define DEBUG_ROUND 0

#define index(x, y) (((x)%5)+5*((y)%5))
#define ROLW(a, offset) ((((ap_uintW)a) << offset) ^ (((ap_uintW)a) >> (BUFFER_WIDTH-offset)))
#define ROL32(a, offset) ((((ap_uint<32>)a) << offset) ^ (((ap_uint<32>)a) >> (32-offset)))
#define ROL16(a, offset) ((((ap_uint<16>)a) << offset) ^ (((ap_uint<16>)a) >> (16-offset)))
#define ROL8(a, offset) ((((ap_uint<8>)a) << offset) ^ (((ap_uint<8>)a) >> (8-offset)))
#define CONCAT(x, y) (x, y)

void printState(ap_uintW state[25]) {
	int x, y;
	for (x = 0; x < 5; x++) {
		for (y = 0; y < 5; y++) {
			std::cout << std::hex << state[x * 5 + y] << " ";
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}
void printState8(ap_uint<8> state[25]) {
	int x, y;
	for (x = 0; x < 5; x++) {
		for (y = 0; y < 5; y++) {
			std::cout << std::hex << state[x * 5 + y] << " ";
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}
ap_uintW rotateLeft(ap_uintW data, ap_uintW offset, ap_uint<2> split) {
	/*********************************************************
	 * Divide data into a number =  (2**(3-split))
	 * Rotate it by offset
	 */
	ap_uintW result = 0;

	if (split == 3) {
		ap_uintW temp = data;
		temp.lrotate(offset);
		result = temp;
	} else if (split == 2) {
		ap_uint<32> temp = data.range(31, 0);
		temp.lrotate(offset.range(4,0));
		result.range(31, 0) = temp;

	} else if (split == 1) {
		ap_uint<16> temp = data.range(15, 0);
		temp.lrotate(offset.range(3,0));
		result.range(15, 0) = temp;
	} else {
		ap_uint<8> temp = data.range(7, 0);
		temp.lrotate(offset.range(2,0));
		result.range(7, 0) = temp;
	}


	return result;
}
void thetaRhoPi(ap_uintW A[25], ap_uint<8> B[25][8], ap_uint<2> split) {
#pragma HLS INLINE
#pragma HLS PIPELINE
//#pragma HLS LOOP_MERGE
	unsigned int x, y;
	ap_uintW C[5], D[5];
	ap_uintW test[25];
	static const unsigned int KeccakRhoOffsets[25] = { 0, 1, 62, 28, 27, 36, 44,
			6, 55, 20, 3, 10, 43, 25, 39, 41, 45, 15, 21, 8, 18, 2, 61, 56, 14 };
	ap_uintW tempA[25];
#pragma HLS ARRAY_PARTITION variable=tempA dim=1
#pragma HLS ARRAY_PARTITION variable=C dim=1
#pragma HLS ARRAY_PARTITION variable=D dim=1
	theta1:
	for (x = 0; x < 5; x++) {
		C[x] = A[index(x, 0)] ^ A[index(x, 1)] ^ A[index(x, 2)] ^ A[index(x, 3)]
				^ A[index(x, 4)];
	}
	theta2:
	for (x = 0; x < 5; x++) {
		D[x] = rotateLeft(C[(x + 1) % 5], 1, split) ^ C[(x + 4) % 5];
	}
	rho:
	for (x = 0; x < 5; x++) {
		for (y = 0; y < 5; y++) {
			ap_uintW temp = A[index(x, y)] ^ D[x];
			tempA[index(x, y)] = rotateLeft(temp, KeccakRhoOffsets[index(x, y)], split);
		}
	}
	pi:
	for (x = 0; x < 5; x++) {
		for (y = 0; y < 5; y++) {
			//test[index(0 * x + 1 * y, 2 * x + 3 * y)] = tempA[index(x, y)];
			//std::cout<<"test is "<<test[index(0 * x + 1 * y, 2 * x + 3 * y)]<<std::endl;
			for (int i = 0 ; i<8;i++ ){
				//std::cout<<"here"<<std::endl;
				B[index(0 * x + 1 * y, 2 * x + 3 * y)][i] = tempA[index(x, y)].range(((i+1)*8)-1, i*8);
				//std::cout<<"tempA at i = "<<i<<" is "<<B[index(0 * x + 1 * y, 2 * x + 3 * y)][i]<<std::endl;
			}
		}
	}
	std::cout<<"In Theta "<<std::endl;
	printState(test);
}

void chi(ap_uint<8> A[25], ap_uint<8> B[25]) {
//#pragma HLS PIPELINE
	unsigned int x, y;
	ap_uint<8> C[25];
	chi1:
	for (y = 0; y < 5; y++) {
		for (x = 0; x < 5; x++) {
			C[index(x, y)] = A[index(x, y)]
					^ ((~A[index(x + 1, y)]) & A[index(x + 2, y)]);
		}
	}
	chi2:
	for (x = 0; x < 25; x++) {
		B[x] = C[x];
	}
}

void iota(ap_uint<8> A[25], ap_uint<8> B[25], unsigned int indexRound, ap_uint<8> splitIndex) {
//#pragma HLS PIPELINE
#if BUFFER_WIDTH == 64
	static const ap_uintW KeccakRoundConstants[32] = {
		0x0000000000000001, 0x0000000000008082, 0x800000000000808A, 0x8000000080008000,
		0x000000000000808B, 0x0000000080000001, 0x8000000080008081, 0x8000000000008009,
		0x000000000000008A, 0x0000000000000088, 0x0000000080008009, 0x000000008000000A,
		0x000000008000808B, 0x800000000000008B, 0x8000000000008089, 0x8000000000008003,
		0x8000000000008002, 0x8000000000000080, 0x000000000000800A, 0x800000008000000A,
		0x8000000080008081, 0x8000000000008080, 0x0000000080000001, 0x8000000080008008,
		0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
		0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000
	};
#elif BUFFER_WIDTH == 32
	static const ap_uintW KeccakRoundConstants[32] = {
		0x00000001, 0x00008082, 0x0000808A, 0x80008000,
		0x0000808B, 0x80000001, 0x80008081, 0x00008009,
		0x0000008A, 0x00000088, 0x80008009, 0x8000000A,
		0x8000808B, 0x0000008B, 0x00008089, 0x00008003,
		0x00008002, 0x00000080, 0x0000800A, 0x8000000A,
		0x80008081, 0x00008080, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000
	};
#elif BUFFER_WIDTH == 16
	static const ap_uintW KeccakRoundConstants[32] = {
		0x0001, 0x8082, 0x808A, 0x8000,
		0x808B, 0x0001, 0x8081, 0x8009,
		0x008A, 0x0088, 0x8009, 0x000A,
		0x808B, 0x008B, 0x8089, 0x8003,
		0x8002, 0x0080, 0x800A, 0x000A,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000,
	};
#elif BUFFER_WIDTH == 8
	static const ap_uintW KeccakRoundConstants[32] = {
		0x01, 0x82, 0x8A, 0x00,
		0x8B, 0x01, 0x81, 0x09,
		0x8A, 0x88, 0x09, 0x0A,
		0x8B, 0x8B, 0x89, 0x03,
		0x02, 0x80, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00,
	};
#elif BUFFER_WIDTH == 4
	static const ap_uintW KeccakRoundConstants[16] = {
		0x1, 0x2, 0xA, 0x0,
		0xB, 0x1, 0x1, 0x9,
		0xA, 0x8, 0x9, 0xA,
		0xB, 0xB, 0x9, 0x3
	};
#elif BUFFER_WIDTH == 2
	static const ap_uintW KeccakRoundConstants[16] = {
		0x1, 0x2, 0x2, 0x0,
		0x3, 0x1, 0x1, 0x1,
		0x2, 0x0, 0x1, 0x2,
		0x3, 0x3, 0x0, 0x0
	};
#elif BUFFER_WIDTH == 1
	static const ap_uintW KeccakRoundConstants[16] = {
		0x1, 0x0, 0x0, 0x0,
		0x1, 0x1, 0x1, 0x1,
		0x0, 0x0, 0x1, 0x0,
		0x0, 0x0, 0x0, 0x0
	};

#endif

#pragma HLS RESOURCE variable=KeccakRoundConstants core=ROM_1P_1S

	for (int x = 0; x < 25; x++) {
		B[x] = A[x];
	}
	B[index(0, 0)] = A[index(0, 0)] ^ KeccakRoundConstants[indexRound].range(((splitIndex+1)*8)-1, splitIndex*8);

}
void keccak(ddrBus* data, ddrBus* output, ap_uint<1> last, ap_uint<2> split, ap_uint<32> nBytes) {

//#pragma HLS INLINE
#pragma HLS PIPELINE
	ap_uint<8> round, splitIndex;
	static ap_uintW state[25] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#pragma HLS ARRAY_PARTITION variable=state dim=1
	ap_uint<8> tmp_state[25][8];
#pragma HLS ARRAY_PARTITION variable=tmp_state dim=0
	ap_uint<8> tmpStateSplit [25] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#pragma HLS ARRAY_PARTITION variable=tmpStateSplit dim=1
	ap_uint<8> tmpStateSplit2[25] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#pragma HLS ARRAY_PARTITION variable=tmpStateSplit2 dim=1
	ap_uint<8> tmpStateSplit3[25] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#pragma HLS ARRAY_PARTITION variable=tmpStateSplit3 dim=1


	/* State ^ Data */
	stateXORdata:
	for (ap_uint<8> i = 0; i < 17; i++) {
#pragma HLS UNROLL
		state[i] ^= data[i];
	}

	/* Main round */
	roundloop:
	for (round = 0; round < 12+(2*(split+3)); round++) {
#pragma HLS PIPELINE
#if DEBUG_ROUND
		std::cout<<"In round "<<round<<std::endl;
		printState(state);
#endif
		thetaRhoPi(state, tmp_state, split);

		splitLoop:
		for (splitIndex = 0; splitIndex < (1<<split); splitIndex++   ){
#pragma HLS PIPELINE
			// Split the tmp state into bytes
			for (int i = 0; i < 25; i++){
#pragma HLS PIPELINE
				ap_uint<3> zero3Bit = 0;
				//tmpStateSplit[i] = tmp_state[i].range(CONCAT(splitIndex+1, zero3Bit) -1, CONCAT(splitIndex, zero3Bit) );
				tmpStateSplit[i] = tmp_state[i][splitIndex];
			}
			std::cout<<"In round "<<round<<" Split index "<<splitIndex<<std::endl;
			//printState(state);
			//printState8(tmpStateSplit);

			chiIota:{
#pragma HLS PIPELINE
			chi(tmpStateSplit, tmpStateSplit2);

			iota(tmpStateSplit2, tmpStateSplit3, round, splitIndex);
			}
			printState8(tmpStateSplit3);
			// Merge the splitted bytes
			mergeLoop:
			for (int i = 0; i < 25; i++){
#pragma HLS PIPELINE
				ap_uint<3> zero3Bit = 0;
				//tmp_state[i].range( CONCAT(splitIndex+1, zero3Bit)-1 , CONCAT(splitIndex, zero3Bit))  = tmpStateSplit3[i];
				tmp_state[i][splitIndex] = tmpStateSplit3[i];
			}
		}
		if ((round == 12+(2*(split+3)) - 1) && (last == 1)) {
			for (int i = 0; i < HASH_SIZE; i++) {
#pragma HLS PIPELINE
				ap_uintW tempOut=0;
				for (int j = 0; j < 8; j++){
					tempOut.range(((j+1)*8)-1, j*8) = tmp_state[i][j];

				}
				output[i] = tempOut;
			}
			for (int i = 0; i < 25; i++) {
#pragma HLS PIPELINE
				state[i] = 0;
			}
		} else
			for (int i = 0; i < 25; i++) {
#pragma HLS PIPELINE
				for (int j = 0; j < 8; j++){
					state[i].range(((j+1)*8)-1, j*8) = tmp_state[i][j];
					std::cout<<"tmp state is "<<tmp_state[i][j]<<std::endl;
				}

			}

	}
}

void keccak_wrapper(ddrBus data[17], ddrBus output[4], ap_uint<1> last, ap_uint<2> split, ap_uint<32> nBytes){
#pragma HLS INTERFACE ap_bus port=data
#pragma HLS INTERFACE ap_bus PORT=output
	ap_uintW   buffer[17];
#pragma HLS ARRAY_PARTITION VARIABLE=buffer dim=1
	ap_uintW   hash[4];
#pragma HLS ARRAY_PARTITION VARIABLE=hash dim=1
	ap_uint<1> stop = 0;
	while (!stop){
#pragma HLS LOOP_TRIPCOUNT MIN=2 MAX=3
#pragma HLS PIPELINE
		stop = last;
		for (int i = 0; i < 17; i++){
			buffer[i] = 0;
		}
		//memcpy(buffer, data, 64*(nBytes>>split));
		memcpy(buffer, data, 8*17);
		keccak(buffer, hash,  stop, split, nBytes);
	}
	memcpy(output, hash, 4*8);
}
