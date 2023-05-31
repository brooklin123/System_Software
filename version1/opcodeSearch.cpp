#include "opcodeSearch.h"
#include <fstream>
#include <iostream>
#include <map>
#include <string>
using namespace std;

string opcodeSearch(string m) {
    ifstream in;
    in.open("opcode.txt");
    if (in.fail()) {
        cout << "input file opening failed";
        exit(1);
    }
    string mnem, opcode;
    map<string, string> table;
    while (in >> mnem >> opcode) {
        table[mnem] = opcode;
    }
    in.close();

    if (table.count(m))
        return table[m];
    else {
        cout << "unvalid" << endl;
        return "-1";
    }
}