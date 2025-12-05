#pragma once

#include <queue>
#include <set>
#include <vector>

#include "cudd.h"
#include "pnml_parser.h"
#include "reachability.h"

DdNode* compute_post(DdManager* mgr, DdNode* R,
                     const vector<vector<int>>& incidence, int t,
                     const vector<DdNode*>& x, const vector<DdNode*>& x_next);
// just removed static state of this function and add it in this header

DdNode* symbolicReachability(const PetriNet& net);

DdNode* make_marking(DdManager* mgr, DdNode** x, const std::vector<int>& bits,
                     int n);
