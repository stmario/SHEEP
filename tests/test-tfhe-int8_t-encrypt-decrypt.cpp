#include <cstdint>
#include <cassert>
#include <algorithm>
#include "context-tfhe.hpp"
#include "simple-circuits.hpp"
#include "circuit-test-util.hpp"

using namespace Sheep::TFHE;
typedef ContextTFHE<int8_t>::Plaintext Plaintext;
typedef ContextTFHE<int8_t>::Ciphertext Ciphertext;

// Encrypt a value, decrypt the result, and check that we are
// left with the original value.
void test_single(ContextTFHE<int8_t>& context) {
	for (Plaintext pt_orig = INT8_MIN; pt_orig < INT8_MAX; pt_orig++) {
		Plaintext pt_new = context.decrypt(context.encrypt(pt_orig));
		assert(pt_orig == pt_new);
	}
}

int main(void) {
	ContextTFHE<int8_t> context;
	test_single(context);
}
