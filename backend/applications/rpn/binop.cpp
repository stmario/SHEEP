#include <vector>
#include <stack>
#include "binop.hpp"
#include "circuit.hpp"
#include "simple-circuits.hpp"
#include "circuit-util.hpp"

using namespace std;

// Constructor
BinOp::BinOp (binoptype binopt){
    btp = binopt;
}

// This implements the core logic of what to do when a binary operation in a calculation has to processed.
// It consumes 2 Circuits from the stack and pushes a new one on the stack by feeding
// them into a new circuit, which implements the computation determined by the operation.
void BinOp::handleOp(vector<int> *ptvec, stack<Circuit> *s){
    if (s->size() < 2)
    {
        throw runtime_error("Stack has not enough input for binary gate."); // TODO: implement meaningful error
    }
    
    Circuit bc; // generate and select the circuit with the binary operation, which determines the computation
    switch (btp)
    {
    case binoptype::Add:
        bc = single_binary_gate_circuit(Gate::Add);
        break;
    case binoptype::Subtract:
        bc = single_binary_gate_circuit(Gate::Subtract);
        break;
    case binoptype::Multiply:
        bc = single_binary_gate_circuit(Gate::Multiply);
        break;
    /*case binoptype::Divide:
        throw GateNotImplementedError*/
    default:
        break;
    }
    Circuit l;
    Circuit r;
    l = s->top();
    s->pop();
    r = s->top();
    s->pop();
    Circuit combc = seq(par(l, r),bc); // feed the previous two circuits into this circuit and push on stack.
    s->push(combc);
}