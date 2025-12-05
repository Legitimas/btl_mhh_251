#include "bdd.h"

#include <iostream>
#include <vector>

#include "heap_counter.h"

using std::cout;
using std::endl;
using std::vector;

DdNode* compute_post(DdManager* mgr, DdNode* R,
                     const vector<vector<int>>& incidence, int t,
                     const vector<DdNode*>& x, const vector<DdNode*>& x_next) {
    int P = static_cast<int>(incidence.size());

    // 1) enabled(x)  =  ∧_{p | C[p][t] < 0} x[p]
    DdNode* enabled = Cudd_ReadOne(mgr);
    Cudd_Ref(enabled);
    for (int p = 0; p < P; ++p) {
        if (incidence[p][t] < 0) {  // cần token ở place p
            DdNode* tmp = Cudd_bddAnd(mgr, enabled, x[p]);
            Cudd_Ref(tmp);
            Cudd_RecursiveDeref(mgr, enabled);
            enabled = tmp;
        }
    }

    // 2) relation(x, x') mô tả hiệu ứng bắn t trên mọi place:
    //    - C[p][t] == 0  ->  x'_p <-> x_p  (giữ nguyên)
    //    - C[p][t] > 0   ->  x'_p = 1      (place nhận token)
    //    - C[p][t] < 0   ->  x'_p = 0      (place mất token)
    DdNode* relation = Cudd_ReadOne(mgr);
    Cudd_Ref(relation);
    for (int p = 0; p < P; ++p) {
        int c = incidence[p][t];
        DdNode* clause = nullptr;

        if (c == 0) {
            // x_next[p] <-> x[p]
            clause = Cudd_bddXnor(mgr, x_next[p], x[p]);
            Cudd_Ref(clause);
        } else if (c > 0) {
            // place nhận token: x_next[p] = 1
            clause = x_next[p];
            Cudd_Ref(clause);
        } else {
            // place mất token: x_next[p] = 0
            clause = Cudd_Not(x_next[p]);
            Cudd_Ref(clause);
        }

        DdNode* tmp = Cudd_bddAnd(mgr, relation, clause);
        Cudd_Ref(tmp);

        Cudd_RecursiveDeref(mgr, relation);
        Cudd_RecursiveDeref(mgr, clause);
        relation = tmp;
    }

    // 3) chỉ xét các marking trong R mà t đang enabled
    DdNode* cond = Cudd_bddAnd(mgr, R, enabled);
    Cudd_Ref(cond);
    Cudd_RecursiveDeref(mgr, enabled);

    DdNode* trans = Cudd_bddAnd(mgr, cond, relation);
    Cudd_Ref(trans);
    Cudd_RecursiveDeref(mgr, cond);
    Cudd_RecursiveDeref(mgr, relation);

    // 4) Tồn tại hóa các biến trạng thái hiện tại x:
    //    post_xprime(x') = ∃x. trans(x, x')
    DdNode* cube = Cudd_ReadOne(mgr);
    Cudd_Ref(cube);
    for (int p = 0; p < P; ++p) {
        DdNode* tmp = Cudd_bddAnd(
            mgr, cube, x[p]);  // phase không quan trọng, chỉ cần chứa biến
        Cudd_Ref(tmp);
        Cudd_RecursiveDeref(mgr, cube);
        cube = tmp;
    }

    DdNode* post_xprime = Cudd_bddExistAbstract(mgr, trans, cube);
    Cudd_Ref(post_xprime);
    Cudd_RecursiveDeref(mgr, trans);
    Cudd_RecursiveDeref(mgr, cube);

    // 5) Đổi tên x' -> x để kết quả quay về không gian biến x
    vector<DdNode*> from(P), to(P);
    for (int i = 0; i < P; ++i) {
        from[i] = x_next[i];
        to[i] = x[i];
    }

    DdNode* post =
        Cudd_bddSwapVariables(mgr, post_xprime, from.data(), to.data(), P);
    Cudd_Ref(post);

    Cudd_RecursiveDeref(mgr, post_xprime);

    return post;
}

DdNode* symbolicReachability(const PetriNet& net) {
    int P = static_cast<int>(net.places.size());
    int T = static_cast<int>(net.transitions.size());

    const vector<vector<int>>& C = net.incidenceMatrix;

    DdManager* mgr = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);

    // Tạo biến trạng thái hiện tại x[0..P-1] và biến trạng thái tiếp theo
    // x_next[0..P-1]
    vector<DdNode*> x(P), x_next(P);
    for (int i = 0; i < P; ++i) {
        DdNode* v = Cudd_bddNewVar(mgr);
        Cudd_Ref(v);
        x[i] = v;
    }
    for (int i = 0; i < P; ++i) {
        DdNode* v = Cudd_bddNewVar(mgr);
        Cudd_Ref(v);
        x_next[i] = v;
    }

    // R ban đầu = {M0}
    DdNode* R = make_marking(mgr, x.data(), net.initialMarking, P);
    // make_marking đã Cudd_Ref nên ở đây KHÔNG cần Cudd_Ref(R) nữa

    bool changed = true;

    while (changed) {
        changed = false;

        // newStates = 0
        DdNode* newStates = Cudd_ReadLogicZero(mgr);
        Cudd_Ref(newStates);

        // Hợp post_t(R) cho mọi transition
        for (int t = 0; t < T; ++t) {
            DdNode* post =
                compute_post(mgr, R, C, t, x, x_next);  // đã Ref bên trong
            DdNode* tmp = Cudd_bddOr(mgr, newStates, post);
            Cudd_Ref(tmp);
            Cudd_RecursiveDeref(mgr, newStates);
            newStates = tmp;
            Cudd_RecursiveDeref(mgr, post);
        }

        // diff = newStates & !R  (các marking thực sự mới)
        DdNode* notR = Cudd_Not(R);  // không cần Ref
        DdNode* diff = Cudd_bddAnd(mgr, newStates, notR);
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

    cout << "\n--- Task 3 Results (Symbolic Reachability with BDDs) ---"
         << endl;
    double num = Cudd_CountMinterm(mgr, R, P);  // R chỉ phụ thuộc vào P biến x
    cout << "Number of reachable markings (BDD): " << num << endl;

    return R;
}

DdNode* make_marking(DdManager* mgr, DdNode** x, const std::vector<int>& bits,
                     int n) {
    DdNode* res = Cudd_ReadOne(mgr);
    Cudd_Ref(res);
    for (int i = 0; i < n; ++i) {
        DdNode* lit = bits[i] ? x[i] : Cudd_Not(x[i]);
        DdNode* tmp = Cudd_bddAnd(mgr, res, lit);
        Cudd_Ref(tmp);
        Cudd_RecursiveDeref(mgr, res);
        res = tmp;
    }

    return res;
}
