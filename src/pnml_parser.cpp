#include "pnml_parser.h"

#include <cctype>  // for std::isspace

string RawBlock::toString() {
    return "(" + id + "," + to_string(tokenAmount) + ")";
}

string RawArc::toString() { return "(" + id + "," + start + "," + end + ")"; }

void RawData::print() {
    for (auto& it : Blocks) {
        cout << it.toString() << " ";
    }
    for (auto& it : Arcs) {
        cout << it.toString() << " ";
    }
}

string inQuote(const string& str) {
    size_t index1 = str.find('\"');
    if (index1 == string::npos) return "";
    size_t index2 = str.find('\"', index1 + 1);
    if (index2 == string::npos || index2 <= index1 + 1) return "";
    return str.substr(index1 + 1, index2 - index1 - 1);
}

/*
 * Legacy helper: remove leading spaces (kept for backwards compatibility).
 * NOTE: This version is NOT used by the new toRaw implementation.
 */
void shearSpace(string& str) {
    size_t index = 0;
    while (index < str.length() && str[index] == ' ') {
        index++;
    }
    str = str.substr(index);
}

/*
 * Legacy helpers kept for reference; the new implementation of toRaw
 * directly parses the PNML line by line and does NOT rely on these.
 */
string extract(const string& fileName) {
    ifstream file(fileName);
    string output = "";
    bool allowNext = false;
    if (file.is_open()) {
        cout << "File " << fileName << " is opened." << endl;
        string line;
        while (getline(file, line)) {
            shearSpace(line);
            if (line.substr(0, 9) == "<place id" ||
                line.substr(0, 14) == "<transition id" ||
                line.substr(0, 7) == "<arc id" || allowNext) {
                output += (line + "$");
                allowNext = false;
                continue;
            }
            if (line == "<initialMarking>") {
                allowNext = true;
                continue;
            }
        }
        file.close();
    } else {
        cout << "Cannot open file " << fileName << "." << endl;
    }
    return output;
}

RawData cascade(string extracted) {
    RawData raw;
    while (extracted.length() > 0) {
        auto findS =
            extracted.find('$');  // changed int to auto to remove warnings
        if (findS == string::npos) break;
        string cut = extracted.substr(0, findS + 1);
        string mode = cut.substr(1, 3);

        if (mode == "pla") {
            RawBlock newBlock{inQuote(cut)};
            raw.Blocks.push_back(newBlock);
            extracted = extracted.substr(findS + 1);
            continue;
        }

        if (mode == "tex") {
            // Old fragile parsing of <text>k</text>$ kept for compatibility.
            // Not used by the new toRaw, but we keep it in case someone
            // still calls extract/cascade directly.
            try {
                raw.Blocks.back().tokenAmount =
                    stoi(cut.substr(6, cut.length() - 14));
            } catch (...) {
                // ignore malformed <text> entries
            }
            extracted = extracted.substr(findS + 1);
            continue;
        }

        if (mode == "tra") {
            RawBlock newBlock{inQuote(cut), -1};
            raw.Blocks.push_back(newBlock);
            extracted = extracted.substr(findS + 1);
            continue;
        }

        if (mode == "arc") {
            RawArc newArc{inQuote(cut), inQuote(cut.substr(cut.find('u'))),
                          inQuote(cut.substr(cut.find('g')))};
            raw.Arcs.push_back(newArc);
            extracted = extracted.substr(findS + 1);
            continue;
        }
    }
    return raw;
}

/*
 * NEW IMPLEMENTATION of toRaw for Task 1
 * --------------------------------------
 * This version parses the PNML file directly and robustly:
 *  - Reads all <place id="..."> and sets tokenAmount to the value
 *    inside its <initialMarking><text>k</text></initialMarking>, if any.
 *  - Reads all <transition id="..."> as RawBlock with tokenAmount = -1.
 *  - Reads all <arc id="..." source="X" target="Y"> as RawArc.
 */
RawData toRaw(const string& fileName) {
    RawData raw;

    ifstream file(fileName);
    if (!file.is_open()) {
        cout << "Cannot open file " << fileName << "." << endl;
        return raw;
    }
    cout << "File " << fileName << " is opened." << endl;

    string line;
    string currentPlaceId;
    bool inPlace = false;
    bool inInitialMarking = false;

    auto trim = [](string& s) {
        size_t start = 0;
        while (start < s.size() &&
               isspace(static_cast<unsigned char>(s[start])))
            ++start;
        size_t end = s.size();
        while (end > start && isspace(static_cast<unsigned char>(s[end - 1])))
            --end;
        s = s.substr(start, end - start);
    };

    while (getline(file, line)) {
        trim(line);

        // Parse <place id="...">
        if (line.rfind("<place id", 0) == 0) {
            string id = inQuote(line);
            RawBlock blk;
            blk.id = id;
            blk.tokenAmount = 0;  // default if no initialMarking
            raw.Blocks.push_back(blk);
            currentPlaceId = id;
            inPlace = true;
            inInitialMarking = false;
            continue;
        }

        // End of current place
        if (inPlace && line.rfind("</place", 0) == 0) {
            inPlace = false;
            inInitialMarking = false;
            currentPlaceId.clear();
            continue;
        }

        // Inside a place: detect initialMarking section
        if (inPlace && line.find("<initialMarking") != string::npos) {
            inInitialMarking = true;
            continue;
        }

        // Inside an <initialMarking> block: look for <text>k</text>
        if (inPlace && inInitialMarking && line.find("<text") != string::npos) {
            size_t open = line.find('>');
            size_t close = line.rfind('<');
            if (open != string::npos && close != string::npos &&
                close > open + 1) {
                string numStr = line.substr(open + 1, close - open - 1);
                try {
                    int tokens = stoi(numStr);
                    if (!raw.Blocks.empty()) {
                        raw.Blocks.back().tokenAmount = tokens;
                    }
                } catch (...) {
                    // ignore malformed numbers
                }
            }
            inInitialMarking = false;
            continue;
        }

        // Parse transitions
        if (line.rfind("<transition id", 0) == 0) {
            string id = inQuote(line);
            RawBlock blk;
            blk.id = id;
            blk.tokenAmount = -1;  // mark as transition
            raw.Blocks.push_back(blk);
            continue;
        }

        // Parse arcs
        if (line.rfind("<arc id", 0) == 0) {
            string id = inQuote(line);

            string source = "";
            string target = "";

            size_t srcPos = line.find("source=");
            if (srcPos != string::npos) {
                string sub = line.substr(srcPos);
                source = inQuote(sub);
            }

            size_t tgtPos = line.find("target=");
            if (tgtPos != string::npos) {
                string sub = line.substr(tgtPos);
                target = inQuote(sub);
            }

            RawArc arc{id, source, target};
            raw.Arcs.push_back(arc);
            continue;
        }
    }

    file.close();
    return raw;
}

/*
 * Convert RawData to explicit PetriNet representation.
 */
PetriNet toPetriNet(const RawData& raw) {
    PetriNet net;
    map<string, int> id_to_index;

    int p_index = 0;
    int t_index = 0;
    for (const auto& block : raw.Blocks) {
        if (block.tokenAmount != -1) {
            // It's a place
            Place p{block.id, block.tokenAmount, p_index};
            net.places.push_back(p);
            id_to_index[block.id] = p_index;
            net.initialMarking.push_back(block.tokenAmount);
            p_index++;
        } else {
            // It's a transition
            Transition t{block.id, t_index};
            net.transitions.push_back(t);
            id_to_index[block.id] = t_index;
            t_index++;
        }
    }

    int P = static_cast<int>(net.places.size());
    int T = static_cast<int>(net.transitions.size());

    net.incidenceMatrix.assign(P, vector<int>(T, 0));

    for (const auto& arc : raw.Arcs) {
        if (id_to_index.find(arc.start) == id_to_index.end() ||
            id_to_index.find(arc.end) == id_to_index.end()) {
            // Inconsistent PNML: arc refers to unknown node.
            // For Task 1 consistency check, có thể log warning thêm.
            continue;
        }

        char start_type = arc.start[0];  // 'p' or 't'
        char end_type = arc.end[0];

        int start_idx = id_to_index[arc.start];
        int end_idx = id_to_index[arc.end];

        if (start_type == 'p' && end_type == 't') {
            // Place -> Transition : consuming arc
            net.incidenceMatrix[start_idx][end_idx] = -1;
        } else if (start_type == 't' && end_type == 'p') {
            // Transition -> Place : producing arc
            net.incidenceMatrix[end_idx][start_idx] = 1;
        }
    }

    return net;
}
