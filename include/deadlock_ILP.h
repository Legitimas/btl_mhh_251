#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "bdd.h"
#include "pnml_parser.h"

// Kết quả trả về (nếu sau này tích hợp solver thì điền vào đây)
struct ILPResult {
    bool hasSolution;
    std::vector<int> deadMarking;
};

// Hàm tạo file .lp (LP Format) để giải bài toán Deadlock
// Input: Mạng Petri, Tên file output
void generateDeadlockILP(
    const PetriNet& net, const std::string& purename,
    vector<vector<int>> forbidden);  // changed to purename, add forbidden (list
                                     // of unreachable dead markings)

// Hàm kiểm tra nhanh xem một Marking có phải là Deadlock không (Dùng để verify
// nghiệm)
bool isMarkingDead(const std::vector<int>& marking, const PetriNet& net);

bool solveILP(const string& purename);  // Call system->cplex.exe to solve
ILPResult cplex(
    const PetriNet& net,
    vector<vector<int>> forbidden);  // call solveILP() and parse sol file
bool is_marking_in_R(
    DdManager* mgr, DdNode* R, const std::vector<DdNode*>& x,
    const std::vector<int>& M);  // check if a marking is reachable
vector<int> findDeadlock(const PetriNet& net);

// integrated copy of original symbolicReachability
DdNode* symbolicReachability_in_mgr(DdManager* mgr, const PetriNet& net,
                                    vector<DdNode*>& x,
                                    vector<DdNode*>& x_next);
