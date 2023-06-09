#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;
struct midStructure {
    // 0 mean no error
    vector<bool> codeType; // 用bitwise!
    string opcode;         // hex
    string operand;
    int opcodeformat;
    int loc, line, len; // 用10進位處理
    midStructure() { this->codeType = vector<bool>(20, false); }
};

struct errorStructe {
    int line, errorType;
    errorStructe(int l, int e) {
        this->line = l;
        this->errorType = e;
    }
};
class Scanner {
  public:
    Scanner();
    void func(string);
    // void secondpass(string);
  private:
    void initOpcode();
    void initRegesterCode();
    void firstpass();
    bool check_insertSymbolTable(string);
    int countLen_handleNextNewAddress(int, string);
    void identifyCodeType_ErrorType(vector<string>&);
    // char intToHexChar(int);
    // void printSymbolTable();
    // void nixbpe(string);
    bool checkDecimal(string);
    string decimalToHex(int);
    pair<int, string> searchOpcode(string);
    string getSubstringUntilFirstDot(string);
    string removeFirstChar(string);
    vector<string> removeAnnotation_Blank(string str);
    bool start, end; // 未出現 //出現兩次要報錯 (?)
    int newAddressing, line, endAddress;
    string program_name, filepath, baseRegister; // operand暫存在class中!
    vector<midStructure*> intermediate_file;
    unordered_map<string, int> SymbolTable;
    unordered_map<string, pair<int, string>> opcodetable;
    unordered_map<string, string> registerTable;
    // unordered_map<int, string> errorTable;
    queue<errorStructe> errorQue;
    void printErrorQue();
};
Scanner::Scanner() {
    initOpcode();
    initRegesterCode();
    start = end = false;
    line = 0;
}
void Scanner::initOpcode() {
    ifstream in;
    in.open("opcode.txt");
    if (in.fail()) {
        cout << "input file opening failed";
        exit(1);
    }
    int format;
    string mnem, opcode;
    while (in >> format >> mnem >> opcode) {
        opcodetable[mnem] = make_pair(format, opcode);
    }
    in.close();
}
void Scanner::initRegesterCode() {
    string name[] = {"A", "X", "L", "B", "S", "T", "F", "PC", "SW"};
    string val[] = {"0", "1", "2", "3", "4", "5", "6", "8", "9"};
    for (int i = 0; i < 9; i++) {
        registerTable[name[i]] = val[i];
    }
}
bool Scanner::checkDecimal(string str) {
    for (int j = 0; j < str.size(); j++) {
        if ((str[j] < '0' || str[j] > '9')) {
            return false;
        }
    }
    return true;
}
pair<int, string> Scanner::searchOpcode(string m) {
    if (opcodetable.count(m))
        return opcodetable[m];
    else {
        return make_pair(0, "0");
    }
}
void Scanner::printErrorQue() {
    // 差: 查errorTable
    cout << "\nprintErrorQue: ";
    int s = errorQue.size();
    for (int i = 0; i < s; i++) {
        cout << errorQue.front().line << " " << errorQue.front().errorType;
        errorQue.pop();
    }
    cout << " ";
}
string Scanner::getSubstringUntilFirstDot(string str) {
    size_t dotIndex = str.find('.');
    if (dotIndex != string::npos) {
        return str.substr(0, dotIndex);
    }
    return str;
}
vector<string> Scanner::removeAnnotation_Blank(string str) {
    // 將字串根據空白拆開，塞進vector裡
    istringstream iss(str); // C++風格的串流的输入操作
    vector<string> vec(istream_iterator<string>{iss},
                       istream_iterator<string>());
    return vec;
}

bool Scanner::check_insertSymbolTable(string label) {
    if (!SymbolTable.count(label)) {
        SymbolTable[label] = newAddressing;
        return true;
    } else {
        return false;
    }
}
int Scanner::countLen_handleNextNewAddress(int kind, string val) { // not finish
    if (kind == 0 || kind == 3) {
        newAddressing += 3;
        return 3;
    }
    if (kind == 1) { //"RESB"
        newAddressing += stoi(val);
        return stoi(val);
    }
    if (kind == 2) { // "RESW"
        newAddressing += stoi(val) * 3;
        return stoi(val) * 3;
    }
    if (kind == 4) {        // "BYTE" X
        newAddressing += 3; // hex -> +3
        return 3;
    }
    if (kind == 5) { // "BYTE" C
        newAddressing += val.size();
        return val.size();
    }
}
void Scanner::identifyCodeType_ErrorType(vector<string>& vec) {
    line++;
    midStructure* tmp = new midStructure();
    string symbol = "", operand = "";
    // cout << line << vec[0] << "!!!!!!!!!!";
    cout << "line: " << line << " ";

    // Pesudo
    if (vec.size() == 3 && (vec[1] == "RESB" || vec[1] == "RESW" ||
                            vec[1] == "BYTE" || vec[1] == "WORD")) {
        // label
        // label != opcodeName
        if (!(searchOpcode(vec[0]).first == 0 &&
              searchOpcode(vec[0]).second == "0")) {
            errorQue.push(errorStructe(line, 1));
            cout << line << " error opcode: " << vec[0] << "\n";
            return;
        } else {
            symbol = vec[0];
            cout << "check get symbol: " << symbol << " ";
        }
        // operand
        // RESB
        if (vec[1] == "RESB") {
            if (checkDecimal(vec[2])) {
                countLen_handleNextNewAddress(1, vec[2]);
                tmp->operand = vec[2]; //?
                cout << "here1";
                tmp->codeType[8] = true;
                cout << "here? ";
                cout << "check RESB ";
            } else {
                errorQue.push(errorStructe(line, 2));
                cout << "error2: RESB format (decimal)\n";
                return;
            }
        }
        // RESW
        else if (vec[1] == "RESW") {
            if (checkDecimal(vec[2])) {
                countLen_handleNextNewAddress(2, vec[2]);
                tmp->operand = vec[2];
                tmp->codeType[8] = true;
                cout << "check RESW ";
            } else {
                errorQue.push(errorStructe(line, 3));
                cout << "RESW format error (decimal)\n";
                return;
            }
        }
        // BYTE
        else if (vec[1] == "BYTE") {
            // e.g.:  C'EOF'
            if (vec[2][0] == 'C' && vec[2][1] == '\'' &&
                vec[2][vec[2].size() - 1] == '\'') {
                // check isAscii
                for (int j = 2; j < vec[2].size() - 1; j++) {
                    if (!isascii(vec[2][j])) {
                        errorQue.push(errorStructe(line, 4));
                        cout << "pesudo BYTE operand error!(ASCII)\n";
                        return;
                    } else {
                        // 強制類型轉換為整數
                        int AsciiCode = static_cast<int>(vec[2][j]);
                        string str_asciiCode = to_string(AsciiCode);
                        operand += str_asciiCode;
                        tmp->codeType[6] = true;
                    }
                }
                cout << "check BYTE ";
                tmp->operand = operand;
                countLen_handleNextNewAddress(5, operand);
            }
            // e.g.:  X'F1'
            else if (vec[2][0] == 'X' && vec[2][1] == '\'' &&
                     vec[2][vec[2].size() - 1] == '\'') {
                // check hex
                for (int j = 2; j < vec[2].size() - 1; j++) {
                    if (!((vec[2][j] >= '0' and vec[2][j] <= '9') ||
                          (vec[2][j] >= 'A' and vec[2][j] <= 'F'))) {
                        errorQue.push(errorStructe(line, 5));
                        cout << "pesudo BYTE operand error! (HEX)\n";
                        return;
                    } else {
                        operand += vec[2][j];
                        tmp->codeType[8] = true;
                    }
                }
                tmp->operand = operand;
                countLen_handleNextNewAddress(4, operand);
            } else {
                errorQue.push(errorStructe(line, 6));
                cout << "BYTE format error\n";
                return;
            }
            cout << "all vec: ";
            for (auto v : vec) {
                cout << v << " * ";
            }
            cout << endl;

        }
        // WORD
        else {
            // check decimal
            if (checkDecimal(vec[2])) {
                tmp->operand = vec[2];
                countLen_handleNextNewAddress(3, vec[2]);
            } else {
                errorQue.push(errorStructe(line, 7));
                cout << "WORD format error (decimal)\n";
                return;
            }
        }

        // Pesduo -- base
    } else if (vec.size() == 2 && vec[0] == "BASE") {
        baseRegister = vec[1];
        //不要輸進operand!!
    }
    // Pesudo -- Start
    else if (vec.size() >= 2 && vec[1] == "START") {
        program_name = vec[0];
        newAddressing = stoi(vec[2], nullptr, 16); //用10進位存
        cout << "Program name is " << vec[0]
             << "startNewAddress: " << newAddressing << endl;
        start = true;
    } else if (!start) {
        errorQue.push(errorStructe(line, 0));
        return;
    }

    // pesudo --END
    else if (end) {
        errorQue.push(errorStructe(line, 10));
        cout << "end error : already end\n";
        return;
    } else if (vec.size() == 2 && !end && vec[0] == "END") {
        cout << " here end ";
        if (SymbolTable.count(vec[1])) {
            end = true;
            cout << "END NAME: " << vec[1];
            endAddress = SymbolTable[vec[1]];
        } else {
            errorQue.push(errorStructe(line, 9));
            cout << "error9 about (end+) symbol!";
        }
        return;
    }
    // normal code!!

    // error
    else {
        cout << "line: " << line;
        cout << " error message: ";
        for (auto v : vec) {
            cout << v << " ";
        }
        cout << endl;
        return;
    }

    if (!check_insertSymbolTable(symbol)) {
        errorQue.push(errorStructe(line, 8));
        cout << "error8 Duplicate Symbol\n";
        return;
    }
    cout << " insert_symbol: " << symbol << " "
         << "symbolLoc" << SymbolTable[symbol] << "|" << endl;
}
void Scanner::firstpass() {
    string inputStr;
    ifstream ifs;
    ifs.open(filepath);
    // while (getline(ifs, inputStr) && !ifs.eof()) {
    while (getline(ifs, inputStr)) {
        // if (ifs.eof()) {
        // cout << "wrong\n";
        // break;

        string str = getSubstringUntilFirstDot(inputStr);
        if (str.empty())
            continue;
        vector<string> vec = removeAnnotation_Blank(str);
        if (vec.size() == 0)
            continue;
        // print all
        for (int i = 0; i < vec.size(); i++) {
            cout << "|" << vec[i];
        }
        cout << endl;
        identifyCodeType_ErrorType(vec);
    }
}
void Scanner::func(string filepath) {
    this->filepath = filepath;
    firstpass();
    // secondpass();
    printErrorQue();
}
int main() {
    ifstream ifs;
    string filepath = "testprog1.S";
    Scanner scanner;
    scanner.func(filepath);
    return 0;
}