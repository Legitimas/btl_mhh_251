#pragma once

#include <queue>
#include <set>
#include <vector>

#include "pnml_parser.h"

using namespace std;

vector<Marking> explicitReachability(const PetriNet& net);

bool is_enabled(const Marking& M, int T_index, const PetriNet& net);

Marking fire_transition(const Marking& M, int T_index, const PetriNet& net);