#include <algorithm>  // Required for std::min
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include "bdd.h"           // Contains symbolicReachability (BDD, CUDD)
#include "deadlock_ILP.h"  // Contains ...
#include "optimization.h"  // Contains ...
#include "pnml_parser.h"  // Contains RawData, toRaw, toPetriNet structures/functions
#include "reachability.h"  // Contains explicitReachability
#include "timer.h"

using namespace std;

// Utility function to print a Marking (vector of integers)
void printMarking(const Marking& M) {
    cout << "[";
    for (size_t i = 0; i < M.size(); ++i) {
        cout << M[i] << (i < M.size() - 1 ? ", " : "");
    }
    cout << "]";
}

int main() {
    int x;
    cout << "Enter file number (e.g., 1 for input_file1.pnml): ";

    // Step 1: Check toRaw function (Task 1: Raw Parsing)
    if (!(cin >> x)) {
        cerr << "Invalid input." << endl;
        return 1;
    }

    string fileName = string("input/") + "input_file" + to_string(x) + ".pnml";

    TIME_START();

    // --- TASK 1: RAW PARSING ---
    RawData raw = toRaw(fileName);
    cout << "\n--- Task 1: Raw Data Check ---" << endl;
    raw.print();  // Print raw data content
    cout << "\n-----------------------------" << endl;

    // Check if RawData is empty (file opening error)
    if (raw.Blocks.empty()) {
        cerr << "Failed to parse data or file is empty." << endl;
        return 1;
    }

    // --- TASK 1: EXPLICIT PETRI NET MODEL CONSTRUCTION ---
    // (This calls the toPetriNet function implemented in pnml_parser.cpp)
    PetriNet net = toPetriNet(raw);

    cout << "\n--- Task 1: PetriNet Model Built ---" << endl;
    cout << "Places: " << net.places.size()
         << ", Transitions: " << net.transitions.size() << endl;
    cout << "Initial Marking M0: ";
    printMarking(net.initialMarking);
    cout << endl;

    // Optional: Print the Incidence Matrix (for debugging)
    /*
    cout << "Incidence Matrix (P x T):" << endl;
    for (const auto& row : net.incidenceMatrix) {
        for (int val : row) {
            cout << val << "\t";
        }
        cout << endl;
    }
    */
    cout << "------------------------------------" << endl;

    TIME_END();

    TIME_START();

    // --- TASK 2: EXPLICIT REACHABILITY COMPUTATION (BFS) ---
    vector<Marking> reachableSet = explicitReachability(net);

    cout << "\n--- Task 2 Results (Explicit Reachability) ---" << endl;
    cout << "Total reachable markings found: " << reachableSet.size() << endl;

    // Display some of the reachable Markings (if any were found)
    if (!reachableSet.empty()) {
        cout << "\n--- Task 2: Sample Reachable Markings ---" << endl;

        // Print the first 5 Markings (or fewer)
        for (size_t i = 0; i < min((size_t)5, reachableSet.size()); ++i) {
            cout << "Marking " << i + 1 << ": ";
            printMarking(reachableSet[i]);
            cout << endl;
        }
    }

    TIME_END();

    TIME_START();

    // --- TASK 3: SYMBOLIC REACHABILITY (BDD + CUDD) ---
    // Hàm này đã in số lượng marking reachable bằng BDD ở trong bdd.cpp
    symbolicReachability(net);

    TIME_END();

    TIME_START();

    cout << "\n--- Task 4: Deadlock detection ---" << endl;

    vector<int> deadlock = findDeadlock(net);
    if (deadlock.empty()) {
        cout << "\nNo deadlock is found.\n" << endl;
    } else {
        printMarking(deadlock);
        cout << "\n";
    }

    TIME_END();

    TIME_START();

    cout << "\n--- Task 5: Linear optimization ---" << endl;

    // dummy costs vector to test
    vector<int> costs(net.places.size(), 1);
    OptimizationTask5Result task5_output = runOptimizationTask5(net, costs);

    task5_output.print();

    TIME_END();

    return 0;
}
