#pragma once

#include <map>
#include <queue>
#include <set>
#include <vector>

#include "bdd.h"
#include "cudd.h"
#include "deadlock_ILP.h"  //for symbolicReachability_in_mgr
#include "pnml_parser.h"
#include "reachability.h"

// derivative of original printMarking in main.cpp
void printMarking_opt(const Marking& M);

struct OptimizationTask5Result {
    double maxValue;             // Max Value of target function
    vector<int> optimalMarking;  // Marking set which have max Value
    bool found;                  // Check if have marking set to get max value
    void print();                // printer
};

OptimizationTask5Result optimizationTask5Function(DdManager* mgr,
                                                  DdNode* reachableSet,
                                                  const vector<int>& costs);

OptimizationTask5Result runOptimizationTask5(const PetriNet& net,
                                             const vector<int>& costs);
