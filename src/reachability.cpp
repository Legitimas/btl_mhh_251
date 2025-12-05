#include "reachability.h"

#include <iostream>

#include "heap_counter.h"

using namespace std;

bool is_enabled(const Marking& M, int T_index, const PetriNet& net) {
    int P_size = net.places.size();
    for (int i = 0; i < P_size; ++i) {
        int required_tokens = -net.incidenceMatrix[i][T_index];
        if (required_tokens > 0 && required_tokens > M[i]) {
            return false;
        }
    }
    return true;
}

Marking fire_transition(const Marking& M, int T_index, const PetriNet& net) {
    Marking M_prime = M;
    int P_size = net.places.size();
    for (int i = 0; i < P_size; ++i) {
        M_prime[i] += net.incidenceMatrix[i][T_index];
    }
    return M_prime;
}

// --- Hàm chính Task 2: Explicit Reachability bằng BFS ---

vector<Marking> explicitReachability(const PetriNet& net) {
    HEAP_START();
    queue<Marking> queue;
    set<Marking> reachSet;

    Marking M0 = net.initialMarking;
    queue.push(M0);
    reachSet.insert(M0);

    vector<Marking> reachableMarkings;
    int T_size = net.transitions.size();

    while (!queue.empty()) {
        Marking M = queue.front();
        queue.pop();
        reachableMarkings.push_back(M);

        for (int j = 0; j < T_size; ++j) {
            if (is_enabled(M, j, net)) {
                Marking M_prime = fire_transition(M, j, net);

                if (reachSet.find(M_prime) == reachSet.end()) {
                    reachSet.insert(M_prime);
                    queue.push(M_prime);
                }
            }
        }
    }

    cout << "--- Task 2 Results (Explicit Reachability) ---" << endl;
    cout << "Total reachable markings found: " << reachableMarkings.size()
         << endl;

    HEAP_END();

    return reachableMarkings;
}