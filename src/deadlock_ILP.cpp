#include "deadlock_ILP.h"

#include <fstream>
#include <iostream>

#include "bdd.h"
#include "heap_counter.h"

using namespace std;

// Hàm kiểm tra một marking cụ thể có phải là dead hay không
// (Logic: Marking chết nếu không có transition nào kích hoạt được)
bool isMarkingDead(const vector<int>& marking, const PetriNet& net) {
    int P = net.places.size();
    int T = net.transitions.size();

    for (int t = 0; t < T; ++t) {
        bool enabled = true;
        // Kiểm tra xem transition t có enabled không
        for (int p = 0; p < P; ++p) {
            // Nếu có cạnh p -> t (incidence < 0)
            if (net.incidenceMatrix[p][t] < 0) {
                int weight = -net.incidenceMatrix[p][t];
                if (marking[p] < weight) {
                    enabled = false;
                    break;  // Transition này bị disable, kiểm tra t tiếp theo
                }
            }
        }
        if (enabled) {
            return false;  // Nếu tồn tại dù chỉ 1 transition enabled -> Không
                           // phải Deadlock
        }
    }
    return true;  // Tất cả đều disabled -> Deadlock
}

// Hàm sinh file mô hình ILP (định dạng CPLEX LP standard)
void generateDeadlockILP(const PetriNet& net, const string& purename,
                         vector<vector<int>> forbidden) {
    string filename = "generated_files/" + purename + ".lp";
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Cannot create ILP file " << filename << endl;
        return;
    }

    int P = net.places.size();
    int T = net.transitions.size();

    // 1. OBJECTIVE FUNCTION
    // Ta chỉ cần tìm 1 nghiệm khả thi (Feasibility), hàm mục tiêu có thể để
    // trống hoặc dummy
    file << "Minimize\n obj: 0\n";

    // 2. CONSTRAINTS
    file << "\nSubject To\n";

    // --- Ràng buộc 1: Phương trình trạng thái (State Equation) ---
    // M = M0 + C * Sigma
    // => x_p - Sum(C[p][t] * sigma_t) = M0(p)
    for (int p = 0; p < P; ++p) {
        file << " c_state_" << p << ": ";

        // Biến x_p (token tại place p)
        file << "x" << p;

        // Trừ đi dòng C[p] * sigma
        for (int t = 0; t < T; ++t) {
            int val = net.incidenceMatrix[p][t];
            if (val != 0) {
                // Đảo dấu vì chuyển vế: - (val * sigma)
                if (val > 0)
                    file << " - " << val << " s" << t;
                else
                    file << " + " << -val << " s" << t;  // val âm -> -val dương
            }
        }

        // Bằng Initial Marking
        file << " = " << net.initialMarking[p] << "\n";
    }

    // --- Ràng buộc 2: Điều kiện Deadlock (Disablement Constraints) ---
    // Với mỗi transition t, nó phải bị disabled.
    // Đối với mạng 1-safe: Disabled <=> Tồn tại p thuộc input(t) sao cho x_p =
    // 0. Công thức ILP: Sum_{p in input(t)} (1 - x_p) >= 1
    // <=> Sum ( -x_p ) >= 1 - Count(input_places)
    // <=> Sum ( x_p ) <= Count(input_places) - 1

    for (int t = 0; t < T; ++t) {
        vector<int> inputPlaces;
        for (int p = 0; p < P; ++p) {
            if (net.incidenceMatrix[p][t] < 0) {
                inputPlaces.push_back(p);
            }
        }

        // Nếu transition không có đầu vào (Source transition) -> Luôn enabled
        // -> Hệ thống không bao giờ Deadlock Ta vẫn in constraint nhưng nó sẽ
        // vô nghiệm (0 <= -1), đúng logic.
        if (inputPlaces.empty()) {
            file << " c_dead_" << t << ": 0 >= 1\n";  // Vô lý -> Vô nghiệm
        } else {
            file << " c_dead_" << t << ": ";
            for (size_t i = 0; i < inputPlaces.size(); ++i) {
                file << "x" << inputPlaces[i];
                if (i < inputPlaces.size() - 1) file << " + ";
            }
            // Tổng token ở các chỗ đầu vào phải bé hơn tổng số chỗ đầu vào (tức
            // là ít nhất 1 chỗ bằng 0)
            file << " <= " << (inputPlaces.size() - 1) << "\n";
        }
    }

    // Extra constraint to make sure the result is not unreachable dead marking
    /*
    if (!forbidden.empty()) {
        file << "\n// Forbidden markings\n";
        for (size_t k = 0; k < forbidden.size(); ++k) {
            // Example: force place 0 to differ
            int p0 = 0;
            int val = forbidden[k][p0];
            file << " c_cut_" << k << ": x" << p0 << " <= " << (val - 1)
                 << "\n";
        }
    }
    */

    // 3. BOUNDS & TYPES
    file << "\nBounds\n";
    // Sigma (số lần bắn) >= 0 (mặc định trong LP format, nhưng khai báo cho rõ)
    for (int t = 0; t < T; ++t) {
        file << " s" << t << " >= 0\n";
    }

    file << "\nBinaries\n";  // Khai báo biến nhị phân (Binary) cho x_p (mạng
                             // 1-safe)
    for (int p = 0; p < P; ++p) {
        file << " x" << p << "\n";
    }

    // (Nếu không phải 1-safe thì dùng Generals và Bounds cho x_p)
    file << "Generals\n";  // Khai báo biến nguyên cho Sigma
    for (int t = 0; t < T; ++t) {
        file << " s" << t << "\n";
    }

    file << "End\n";
    file.close();
    cout << "--> Generated ILP file: " << filename << endl;
}

bool solveILP(const string& purename) {
    string filename = "generated_files/" + purename + ".lp";
    string solfilename = "generated_files/" + purename + ".sol";
    string systemCmd = "cplex -c \"read " + filename + "\" \"opt\" \"write " +
                       solfilename + "\" \"quit\" > NUL 2>&1";
    // > NUL is to mute log
    // 2>&1 is to keep log minimal
    int res = system(systemCmd.c_str());
    if (res == -1) {
        cerr << "ERROR: system() failed to run the command.\n";
        return false;
    } else if (res != 0) {
        cerr << "ERROR: CPLEX failed to execute. Exit code = " << res << "\n";
        return false;
    }
    if (!filesystem::exists(solfilename)) {
        cerr << "ERROR: No solution file generated by CPLEX.\n";
        return false;
    }
    cout << "\nILP solved successfully" << endl;
    return true;

    // incomplete function
}

ILPResult cplex(const PetriNet& net, vector<vector<int>> forbidden) {
    string purename = "auto_named";
    string solFile = "generated_files/" + purename + ".sol";
    if (filesystem::exists(solFile)) {
        if (filesystem::remove(solFile)) {
            cout << "solFile deleted successfully.\n";
        } else {
            cout << "Failed to delete file.\n";
        }
    } else {
        cout << "File does not exist.\n";
    }
    // This ensure old tests cannot affect new tests
    generateDeadlockILP(net, purename, forbidden);
    bool solved = solveILP(purename);
    ILPResult output;
    if (!solved) {
        output.hasSolution = false;
    } else {
        // you might have noticed the little parser below is kinda weak, feel
        // free to improve it then. However, it works for trivial cases
        output.hasSolution = true;
        vector<int> Marking(net.places.size(), 0);
        ifstream file(solFile);
        if (!file.is_open()) {
            cout << "Cannot open file " << solFile << "." << endl;
            output.hasSolution = false;
        }
        cout << "File " << solFile << " is opened." << endl;
        string line;
        while (getline(file, line)) {
            shearSpace(line);
            if (line.substr(0, 14) == "<variable name") {
                string str = inQuote(line);
                if (str[0] == 'x') {
                    int state = stoi(inQuote(line.substr(line.find('u'))));
                    if (state) {
                        Marking.at(stoi(str.substr(1))) = state;
                    }
                }
            }
        }
        file.close();
        output.deadMarking = Marking;
    }
    return output;
}

// Check if a marking M is in R
bool is_marking_in_R(DdManager* mgr, DdNode* R, const std::vector<DdNode*>& x,
                     const std::vector<int>& M) {
    int P = static_cast<int>(M.size());

    // Build BDD for candidate marking
    DdNode* markingBDD =
        make_marking(mgr, const_cast<DdNode**>(x.data()), M, P);

    DdNode* zero = Cudd_ReadLogicZero(mgr);
    Cudd_Ref(zero);
    DdNode* one = Cudd_ReadOne(mgr);
    Cudd_Ref(one);

    DdNode* test = Cudd_bddAnd(mgr, zero, one);
    Cudd_Ref(test);
    Cudd_RecursiveDeref(mgr, test);
    Cudd_RecursiveDeref(mgr, zero);
    Cudd_RecursiveDeref(mgr, one);

    // Intersection with reachable set
    DdNode* inter = Cudd_bddAnd(mgr, R, markingBDD);

    Cudd_Ref(inter);
    bool exists = (inter != Cudd_ReadLogicZero(mgr));

    // Dereference temporary BDDs
    Cudd_RecursiveDeref(mgr, markingBDD);
    Cudd_RecursiveDeref(mgr, inter);

    return exists;
}

// FUNCTION ABOVE WAS IN DEBUGGING SESSION

vector<int> findDeadlock(const PetriNet& net) {
    int P = static_cast<int>(net.places.size());

    // Initialize BDD manager
    DdManager* mgr = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);

    // Create place variables
    vector<DdNode*> x(P), x_next(P);
    for (int i = 0; i < P; ++i) {
        x[i] = Cudd_bddNewVar(mgr);
        Cudd_Ref(x[i]);
        x_next[i] = Cudd_bddNewVar(mgr);
        Cudd_Ref(x_next[i]);
    }

    // Compute reachable set R once
    DdNode* R =
        symbolicReachability_in_mgr(mgr, net, x, x_next);  // updated function

    // Forbidden list for ILP
    vector<vector<int>> forbidden;

    while (true) {
        // Run ILP with current forbidden markings
        ILPResult res = cplex(net, forbidden);

        if (!res.hasSolution) {
            cout << "No deadlock found (ILP infeasible)\n";
            break;
        }

        // Check if candidate marking is reachable
        if (is_marking_in_R(mgr, R, x, res.deadMarking)) {
            cout << "Deadlock found!\n";

            // Cleanup BDDs
            Cudd_RecursiveDeref(mgr, R);
            for (int i = 0; i < P; ++i) {
                Cudd_RecursiveDeref(mgr, x[i]);
                Cudd_RecursiveDeref(mgr, x_next[i]);
            }
            Cudd_Quit(mgr);
            return res.deadMarking;
        } else {
            cout << "Candidate marking not reachable, adding to forbidden "
                    "list\n";
            forbidden.push_back(res.deadMarking);
        }
    }

    // Cleanup if no deadlock found
    Cudd_RecursiveDeref(mgr, R);
    for (int i = 0; i < P; ++i) {
        Cudd_RecursiveDeref(mgr, x[i]);
        Cudd_RecursiveDeref(mgr, x_next[i]);
    }
    Cudd_Quit(mgr);

    return {};  // empty = no deadlock
}

DdNode* symbolicReachability_in_mgr(DdManager* mgr, const PetriNet& net,
                                    vector<DdNode*>& x,
                                    vector<DdNode*>& x_next) {
    int P = static_cast<int>(net.places.size());
    int T = static_cast<int>(net.transitions.size());
    const auto& C = net.incidenceMatrix;

    // Create current and next state variables
    for (int i = 0; i < P; ++i) {
        x[i] = Cudd_bddIthVar(mgr, i);  ////was Cudd_bddNewVar(mgr)
        Cudd_Ref(x[i]);
        x_next[i] = Cudd_bddIthVar(mgr, i + P);  // was Cudd_bddNewVar(mgr)
        Cudd_Ref(x_next[i]);
    }

    // R0 = initial marking
    DdNode* R = make_marking(mgr, x.data(), net.initialMarking, P);
    // make_marking already Ref'd
    bool changed = true;

    while (changed) {
        changed = false;

        DdNode* newStates = Cudd_ReadLogicZero(mgr);
        Cudd_Ref(newStates);

        for (int t = 0; t < T; ++t) {
            DdNode* post = compute_post(
                mgr, R, C, t, x, x_next);  // assumed already Ref'd inside
            DdNode* tmp = Cudd_bddOr(mgr, newStates, post);
            Cudd_Ref(tmp);
            Cudd_RecursiveDeref(mgr, newStates);
            newStates = tmp;
            Cudd_RecursiveDeref(mgr, post);
        }

        DdNode* diff = Cudd_bddAnd(mgr, newStates, Cudd_Not(R));
        Cudd_Ref(diff);

        if (diff != Cudd_ReadLogicZero(mgr)) {
            changed = true;
            DdNode* tmp = Cudd_bddOr(mgr, R, diff);
            Cudd_Ref(tmp);
            Cudd_RecursiveDeref(mgr, R);
            R = tmp;
        }

        Cudd_RecursiveDeref(mgr, newStates);
        Cudd_RecursiveDeref(mgr, diff);
    }

    /*
    double num = Cudd_CountMinterm(mgr, R, P);
    cout << "\n--- Task 3 Results (Symbolic Reachability with BDDs) ---"
         << endl;
    cout << "Number of reachable markings (BDD): " << num << endl;
    */
    // left here to indicate this function is a derivative of its origin

    return R;
}