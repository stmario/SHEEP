#include <vector>
#include <stack>
#include "include/unop.hpp"
#include "circuit.hpp"
#include "simple-circuits.hpp"
#include "circuit-util.hpp"

using namespace std;

/// Constructor
/// \param unopt Type of the operation (which gate). Gets simply copied into utp.
UnOp::UnOp(unoptype unopt){
    utp = unopt;
}


/// This implements the core logic of what to do when a unary operation in a calculation has to be processed.
/// A Circuit a from the stack gets consumed, such that when handleOp returns the Circuit on top is that
/// Circuit made sequential with the new operation b: a->b
/// \param ptvec Vector, in which the plaintext inputs of the calculation get stored (in order of appearance).
/// \param s Stack of circuits, which represent the intermediately computed subcomponents of the final circuit.
void UnOp::handleOp(vector<int> &ptvec, stack<Circuit> &s){
    if (s.empty())
    {
        throw runtime_error("Stack has not enough input for unary gate.");
    }
    
    Circuit uc;
    switch (utp)
    {
    case unoptype::Negate: // handle negation operation
    {
        uc = single_unary_gate_circuit(Gate::Negate); // generate new circuit uc with negation gate.
        Circuit l;
        l = s.top(); // get intermediate circuit l from stack
        s.pop(); // remove it
        Circuit sc = seq(l,uc); // make sequential circuit from intermediate circuit and new gat circuit: l->uc
        s.push(sc); // push this as new intermediate circuit on the stack
        break;
    }
    case unoptype::Square: //handle square operation
    {
        // generate "duplicator" circuit, so we don't have a mismatch on int inputs and inputs required. Then multiply to square.
        // Make sequential with previous circuit on stack.
        // This would be nice to integrate int circuit-util.hpp
        // Circuit design:
        //     in1
        //     dup
        //   /     \
        // out1 * out2
        //      |
        Circuit duplicator;
        Wire in1 = duplicator.add_input("in1");
        Wire out1 = duplicator.add_assignment("out1", Gate::Alias, in1);
        Wire out2 = duplicator.add_assignment("ou2", Gate::Alias, in1);
        duplicator.set_output(out1);
        duplicator.set_output(out2);
        uc = single_binary_gate_circuit(Gate::Multiply);
        Circuit in;
        in = s.top();
        s.pop();
        Circuit sc = seq(seq(in, duplicator),uc);
        s.push(sc);
        break;
    }
    /*case unoptype::SquareRoot:
        throw GateNotImplementedError
        break;
    case unoptype::Invert:
        throw GateNotImplementedError
        */
    default:
        break;
    }
}