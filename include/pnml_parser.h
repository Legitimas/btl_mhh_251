#pragma once

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;

struct RawBlock  // contains both places and transitions, transitions have
                 // tokenAmount=-1
{
    string id = "";
    int tokenAmount = 0;
    string toString();
};

struct RawArc {
    string id = "";
    string start = "";
    string end = "";
    string toString();
};

struct RawData {
    vector<RawBlock> Blocks;
    vector<RawArc> Arcs;
    void print();
};

string inQuote(const string& str);

/*
The function above does not handle any exceptions,
it works only when the input has expected form.
*/
void shearSpace(string& str);  // get rid of spaces of indentation
string extract(
    const string& fileName);  // extract useful part of the file as a string
RawData cascade(string extracted);      // turn the extracted string into usable
                                        // data
RawData toRaw(const string& fileName);  // composition of extract and cascade

// A Marking is a vector of integers, representing the number of tokens in each
// Place.
using Marking = vector<int>;

// Structure for a Place:
struct Place {
    string id;
    int initialTokens;
    int index;  // Position in the Marking vector and Incidence Matrix.
};

// Structure for a Transition:
struct Transition {
    string id;
    int index;  // Position in the Incidence Matrix.
};

// Structure to store the explicit representation of the Petri Net:
struct PetriNet {
    vector<Place> places;
    vector<Transition> transitions;
    // The Incidence Matrix (Rows = Places, Columns = Transitions).
    // C[i][j] represents the change in tokens at Place i when Transition j
    // fires.
    vector<vector<int>> incidenceMatrix;
    Marking initialMarking;  // The initial state (M0).
};

// Conversion function (Task 1: PNML Parsing)
PetriNet toPetriNet(const RawData& raw);
