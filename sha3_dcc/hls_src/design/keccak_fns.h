

#define DEBUG_ROUND 0

#define index(x, y) (((x)%5)+5*((y)%5))
#define ROLW(a, offset) ((offset != 0) ? ((((ap_uintW)a) << offset) ^ (((ap_uintW)a) >> (BUFFER_WIDTH-offset))) : a)

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

void theta(ap_uintW A[25], ap_uintW B[25]) {
	unsigned int x, y;
	ap_uintW C[5], D[5];

	theta1: for (x = 0; x < 5; x++) {
		C[x] = A[index(x, 0)] ^ A[index(x, 1)] ^ A[index(x, 2)] ^ A[index(x, 3)]
				^ A[index(x, 4)];
	}
	theta2: for (x = 0; x < 5; x++) {
		D[x] = ROLW(C[(x+1)%5], 1) ^ C[(x + 4) % 5];
	}

	theta3: for (x = 0; x < 5; x++) {
		for (y = 0; y < 5; y++) {
			B[index(x, y)] = A[index(x, y)] ^ D[x];
		}
	}
}

void rho(ap_uintW A[25]) {
	static const unsigned int KeccakRhoOffsets[25] = { 0, 1, 62, 28, 27, 36, 44,
			6, 55, 20, 3, 10, 43, 25, 39, 41, 45, 15, 21, 8, 18, 2, 61, 56, 14 };
	unsigned int x, y;

	rho1: for (x = 0; x < 5; x++) {
		for (y = 0; y < 5; y++) {
			A[index(x, y)] = ROLW(A[index(x, y)],
					KeccakRhoOffsets[index(x, y)]);
		}
	}
}

void pi(ap_uintW A[25]) {
	unsigned int x, y;
	ap_uintW tempA[25];

	pi1:for (x = 0; x < 5; x++) {
		for (y = 0; y < 5; y++) {
			tempA[index(x, y)] = A[index(x, y)];
		}
	}
	pi2:for (x = 0; x < 5; x++) {
		for (y = 0; y < 5; y++) {
			A[index(0 * x + 1 * y, 2 * x + 3 * y)] = tempA[index(x, y)];
		}
	}
}

void chi(ap_uintW A[25]) {
	unsigned int x, y;
	ap_uintW C[25];

	chi1:for (y = 0; y < 5; y++) {
		for (x = 0; x < 5; x++) {
			C[index(x, y)] = A[index(x, y)]
					^ ((~A[index(x + 1, y)]) & A[index(x + 2, y)]);
		}
	}

	chi2:for (x = 0; x < 25; x++) {
		A[x] = C[x];
	}
}

void iota(ap_uintW A[25], unsigned int indexRound) {
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
	iota:A[index(0, 0)] ^= KeccakRoundConstants[indexRound];
}

void keccak(ddrBus* data, ddrBus* output, ap_uint<1> last) {
#pragma HLS INLINE
	int i, j;
	ap_uint<8> round;
	static ap_uintW state[25] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	ap_uintW tmp_state[25];

	/* State ^ Data */\
stateXORdata:for (i = 0; i < NUMBER_BUFFERS; i++) {
		state[i] ^= data[i / (64 / BUFFER_WIDTH)].range(
				(((i % (64 / BUFFER_WIDTH)) + 1) * BUFFER_WIDTH) - 1,
				(i % (64 / BUFFER_WIDTH)) * BUFFER_WIDTH);
	}

	/* Main round */
	roundLoop:for (round = 0; round < NB_ROUNDS; round++) {
#if DEBUG_ROUND
		std::cout<<"NB_ROUNDS= "<<NB_ROUNDS<<std::endl;
		std::cout<<"Input to round  "<<round<<":"<<std::endl;
		printState(state);
#endif
		theta(state, tmp_state);
#if DEBUG_ROUND
		std::cout<<"  After theta:"<<std::endl;
		printState(tmp_state);
#endif
		rho(tmp_state);
#if DEBUG_ROUND
		std::cout<<"  After rho:"<<std::endl;
		printState(state);
#endif
		pi(tmp_state);
#if DEBUG_ROUND
		std::cout<<"  After pi:"<<std::endl;
		printState(tmp_state);
#endif
		chi(tmp_state);
#if DEBUG_ROUND
		std::cout<<"  After chi:"<<std::endl;
		printState(tmp_state);
#endif
		iota(tmp_state, round);
#if DEBUG_ROUND
		std::cout<<"  After iota:"<<std::endl;
		printState(tmp_state);
#endif
		if ((round == NB_ROUNDS - 1) && (last == 1)) {
			for (i = 0; i < HASH_SIZE; i++) {
				ap_uint<64> tmpOutput;
				for (j = 0; j < 64 / BUFFER_WIDTH; j++) {
					tmpOutput.range(((j + 1) * BUFFER_WIDTH) - 1,
							j * BUFFER_WIDTH) = tmp_state[(i
							* (64 / BUFFER_WIDTH)) + j];
				}
				//output[i] = tmp_state[i];
				output[i] = tmpOutput;
			}
			for (i = 0; i < 25; i++) {
				state[i] = 0;
			}
		} else
			for (i = 0; i < 25; i++) {
				state[i] = tmp_state[i];
			}
	}
}

