#include <cstdint>
#include <cassert>
#include <algorithm>
#include "context-helib.hpp"
#include "simple-circuits.hpp"
#include "circuit-test-util.hpp"

using namespace Sheep::HElib;
typedef ContextHElib_Fp<bool>::Plaintext Plaintext;
typedef ContextHElib_Fp<bool>::Ciphertext Ciphertext;

// Encrypt a value, decrypt the result, and check that we are
// left with the original value.
void test_single(ContextHElib_Fp<bool>& context) {
  Plaintext pt_orig = 1;
  Plaintext pt_new = context.decrypt(context.encrypt(pt_orig));
  assert(pt_orig == pt_new);
}

int main(void) {
  ContextHElib_Fp<bool> context;
  test_single(context);
}
