#include "optimization.h"

static map<DdNode*, double> memoVal;
const double NEG_INF = -1e18;

static double getGapBonus(int current_level, int next_level,
                          const vector<int>& costs) {
    double bonus = 0;
    for (size_t i = current_level + 1; i < (size_t)next_level; ++i) {
        if (i < costs.size() && costs[i] > 0) {
            bonus += costs[i];
        }
    }
    return bonus;
}
// Dynamic function

double calculateMaxValueRec(DdManager* mgr, DdNode* node,
                            const vector<int>& costs) {
    DdNode* deadNode = Cudd_ReadLogicZero(mgr);
    DdNode* leafNode = Cudd_ReadOne(mgr);
    if (node == deadNode) return NEG_INF;
    if (node == leafNode) return 0.0;

    if (memoVal.find(node) != memoVal.end()) return memoVal[node];

    size_t currPart = Cudd_NodeReadIndex(
        node);  // changed int to size_t to remove warnings 29/11.Nhan

    DdNode* highNode = Cudd_T(node);
    DdNode* lowNode = Cudd_E(node);

    if (Cudd_IsComplement(node)) {
        highNode = Cudd_Not(highNode);
        lowNode = Cudd_Not(lowNode);
    }
    double valueHigh = calculateMaxValueRec(mgr, highNode, costs);

    if (valueHigh > NEG_INF) {
        int nextPart = (highNode == deadNode || highNode == leafNode)
                           ? costs.size()
                           : Cudd_NodeReadIndex(highNode);
        if (currPart < costs.size()) {
            valueHigh += costs[currPart];
        }
        valueHigh += getGapBonus(currPart, nextPart, costs);
    }

    double valueLow = calculateMaxValueRec(mgr, lowNode, costs);

    if (valueLow > NEG_INF) {
        int nextPart = (lowNode == deadNode || lowNode == leafNode)
                           ? costs.size()
                           : Cudd_NodeReadIndex(lowNode);
        valueLow += getGapBonus(currPart, nextPart, costs);
    }

    double maxVal = max(valueHigh, valueLow);
    memoVal[node] = maxVal;

    return maxVal;
}

void reconstructPath(DdManager* mgr, DdNode* node, const vector<int>& costs,
                     vector<int>& marking, int parent_level) {
    DdNode* deadNode = Cudd_ReadLogicZero(mgr);
    DdNode* leafNode = Cudd_ReadOne(mgr);
    if (node == leafNode) {
        for (size_t i = parent_level + 1; i < costs.size(); ++i)
            marking[i] = (costs[i] > 0) ? 1 : 0;
        return;
    }
    if (node == deadNode) return;

    int currPart = Cudd_NodeReadIndex(node);
    for (int i = parent_level + 1; i < currPart; ++i)
        marking[i] = (costs[i] > 0) ? 1 : 0;

    DdNode* highNode = Cudd_T(node);
    DdNode* lowNode = Cudd_E(node);
    if (Cudd_IsComplement(node)) {
        highNode = Cudd_Not(highNode);
        lowNode = Cudd_Not(lowNode);
    }
    double valueHigh = NEG_INF;
    if (highNode != deadNode) {
        int nextPart = (highNode == leafNode) ? costs.size()
                                              : Cudd_NodeReadIndex(highNode);
        double hVal =
            (highNode == leafNode)
                ? 0.0
                : ((memoVal.find(highNode) != memoVal.end()) ? memoVal[highNode]
                                                             : NEG_INF);

        if (hVal > NEG_INF)
            valueHigh =
                hVal + costs[currPart] + getGapBonus(currPart, nextPart, costs);
    }

    double valueLow = NEG_INF;
    if (lowNode != deadNode) {
        int nextPart =
            (lowNode == leafNode) ? costs.size() : Cudd_NodeReadIndex(lowNode);
        double lVal =
            (lowNode == leafNode)
                ? 0.0
                : ((memoVal.find(lowNode) != memoVal.end()) ? memoVal[lowNode]
                                                            : NEG_INF);

        if (lVal > NEG_INF)
            valueLow = lVal + getGapBonus(currPart, nextPart, costs);
    }

    if (valueHigh >= valueLow && valueHigh > NEG_INF) {
        marking[currPart] = 1;
        reconstructPath(mgr, highNode, costs, marking, currPart);
    } else {
        marking[currPart] = 0;
        reconstructPath(mgr, lowNode, costs, marking, currPart);
    }
}

OptimizationTask5Result optimizationTask5Function(DdManager* mgr,
                                                  DdNode* reachableSet,
                                                  const vector<int>& costs) {
    DdNode* deadNode = Cudd_ReadLogicZero(mgr);
    DdNode* leafNode = Cudd_ReadOne(mgr);
    OptimizationTask5Result res;
    res.found = false;
    res.maxValue = NEG_INF;
    memoVal.clear();

    if (reachableSet == deadNode) return res;

    double rawMax = calculateMaxValueRec(mgr, reachableSet, costs);

    int rootPart = (reachableSet == leafNode)
                       ? costs.size()
                       : Cudd_NodeReadIndex(reachableSet);
    double initialGap = getGapBonus(-1, rootPart, costs);

    if (rawMax > NEG_INF) {
        res.found = true;
        res.maxValue = rawMax + initialGap;

        res.optimalMarking.resize(costs.size());
        reconstructPath(mgr, reachableSet, costs, res.optimalMarking, -1);
    }

    return res;
}

void printMarking_opt(const Marking& M) {
    cout << "[";
    for (size_t i = 0; i < M.size(); ++i) {
        cout << M[i] << (i < M.size() - 1 ? ", " : "");
    }
    cout << "]";
}

void OptimizationTask5Result::print() {
    if (this->found) {
        cout << "\nthis marking is a maximizer:" << "\n";
        printMarking_opt(this->optimalMarking);
        cout << " Max value: " << this->maxValue << endl;
    }
}

OptimizationTask5Result runOptimizationTask5(const PetriNet& net,
                                             const std::vector<int>& costs) {
    int P = static_cast<int>(net.places.size());

    DdManager* mgr = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
    std::vector<DdNode*> x(P), x_next(P);
    for (int i = 0; i < P; ++i) {
        x[i] = Cudd_bddIthVar(mgr, i);  // was Cudd_bddNewVar(mgr)
        Cudd_Ref(x[i]);
        x_next[i] = Cudd_bddIthVar(mgr, i + P);  // was Cudd_bddNewVar(mgr)
        Cudd_Ref(x_next[i]);
    }

    DdNode* reachableSet = symbolicReachability_in_mgr(mgr, net, x, x_next);

    OptimizationTask5Result result =
        optimizationTask5Function(mgr, reachableSet, costs);

    Cudd_RecursiveDeref(mgr, reachableSet);
    for (auto v : x) Cudd_RecursiveDeref(mgr, v);
    for (auto v : x_next) Cudd_RecursiveDeref(mgr, v);
    Cudd_Quit(mgr);

    return result;
}
