#pragma once

#include <vector>
#include <stack>
#include "circuit.hpp"

/// Abstract Base class of an operation. Cannot be instantiated, but in the Token class the subclass objects can be accessed via pointer.
/// With handleOp, we can use dynamic binding to iterate through a heterogeneous vector.
class Op {
private:
public:
    /// This implements the core logic of what to do when an operation in a calculation has to processed  by manipulating s and ptvec.
    /// \param ptvec Vector, in which the plaintext inputs of the calculation get stored (in order of appearance).
    /// \param s Stack of circuits, which represent the intermediately computed subcomponents of the final circuit.
    virtual void handleOp(std::vector<int> &ptvec, std::stack<Circuit> &s) = 0;

    //virtual ~Op(){};
};
