#include <bitset>
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
    bitset<4> xbpe;
    bitset<2> ni;
    string opcode; // hex
    string operand;
    int opcodeformat, codeType; // opcodeformat 可刪吧!!
    int loc, len;               // 用10進位處理
    midStructure() {
        this->xbpe = bitset<4>(string("0000"));
        this->ni = bitset<2>(string("11"));
        this->opcode = "";
        this->operand = "";
        this->codeType = 0;
        this->len = 0;
    }
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

  private:
    void initOpcode();
    void initRegesterCode();
    void firstpass();
    void secondpass();
    int countLen_handleNextNewAddress(int, string);
    void identifyCodeType_ErrorType(vector<string>&);
    char intToHexChar(int);
    // void printSymbolTable();
    // void nixbpe(string);
    bool checkDecimal(string);
    string decimalToHex(int);
    string getSubstringUntilFirstDot(string);
    string removeFirstChar(string);
    vector<string> removeAnnotation_Blank(string str);
    bool start, end; // 未出現 //出現兩次要報錯 (?)
    int newAddressing, line, programStartAddr, totalLen, startAddr;
    string program_name, filepath, baseRegister; // operand暫存在class中!
    vector<midStructure*> intermediate_file;
    unordered_map<string, int> SymbolTable;
    // mnem <format, opcodeNum>
    unordered_map<string, pair<int, string>> opcodeTable;
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
char Scanner::intToHexChar(int i) {
    if (i >= 0 && i <= 9) {
        return '0' + i;
    } else if (i >= 10 && i <= 15) {
        return 'A' + i - 10;
    } else {
        return '0';
    }
}
string Scanner::decimalToHex(int decimal) {
    string hex = "";
    while (decimal > 0) {
        int remainder = decimal % 16;
        hex = intToHexChar(remainder) + hex;
        decimal = decimal / 16;
    }
    if (hex == "") {
        hex = "0";
    }
    return hex;
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
        opcodeTable[mnem] = make_pair(format, opcode);
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
string Scanner::removeFirstChar(string str) {
    string s = "";
    for (int i = 1; i < str.size(); i++) {
        s += str[i];
    }
    return s;
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

int Scanner::countLen_handleNextNewAddress(int kind, string val) {
    if (kind == 1) { //"RESB"
        newAddressing += stoi(val);
        return stoi(val);
    }
    if (kind == 2) { // "RESW"
        newAddressing += stoi(val) * 3;
        return stoi(val) * 3;
    }
    if (kind == 3) {        // "BYTE" X
        newAddressing += 3; // hex -> +3
        return 3;
    }
    if (kind == 4) {
        newAddressing += 1;
        return 1;
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
    cout << "line: " << line << " ";
    // Pesudo
    if (vec.size() == 3 && (vec[1] == "RESB" || vec[1] == "RESW" ||
                            vec[1] == "BYTE" || vec[1] == "WORD")) {
        // label
        // label != opcodeName
        if (opcodeTable.count(vec[0])) {
            errorQue.push(errorStructe(line, 1));
            cout << line << " error opcode: " << vec[0] << "\n";
            return;
        } else {
            symbol = vec[0];
            cout << "check get symbol: " << symbol << " ";
            // return; // 不用insert進中間檔
        }
        // operand
        // RESB
        if (vec[1] == "RESB") {
            if (checkDecimal(vec[2])) {
                tmp->len = countLen_handleNextNewAddress(1, vec[2]);
                tmp->codeType = 0;
                // cout << "here? ";
                cout << "[check RESB] ";
                // return; // 不用insert進中間檔
            } else {
                errorQue.push(errorStructe(line, 2));
                cout << "error2: RESB format (decimal)\n";
                return;
            }
        }
        // RESW
        else if (vec[1] == "RESW") {
            if (checkDecimal(vec[2])) {
                tmp->len = countLen_handleNextNewAddress(2, vec[2]);
                // tmp->operand = vec[2];
                tmp->codeType = 0;
                cout << "[check RESW] ";
            } else {
                errorQue.push(errorStructe(line, 3));
                cout << "RESW format error (decimal)\n";
                return;
            }
        }
        // BYTE
        else if (vec[1] == "BYTE") {
            tmp->loc = newAddressing;
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
                        stringstream hexAscii;
                        // 強制類型轉換為整數
                        int AsciiCode = static_cast<int>(vec[2][j]);
                        // string str_asciiCode = to_string(AsciiCode);
                        hexAscii << hex << AsciiCode;
                        operand += hexAscii.str();
                        tmp->codeType = 1;
                    }
                }
                cout << "[check BYTE] ";
                // handleNextNewAddress
                newAddressing += operand.size() / 2;
                tmp->len =
                    operand.size() / 2; // // 因為目前operand是翻成hex的ascii!
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
                        tmp->codeType = 1;
                    }
                }
                tmp->len = countLen_handleNextNewAddress(4, operand);
            } else {
                errorQue.push(errorStructe(line, 6));
                cout << "BYTE format error\n";
                return;
            }
            tmp->operand = operand;
        }
        // WORD
        else {
            tmp->loc = newAddressing;
            // check decimal
            if (checkDecimal(vec[2])) {
                // operand要轉hex
                tmp->operand = decimalToHex(stoi(vec[2]));
                tmp->codeType = 1;
                tmp->len = countLen_handleNextNewAddress(3, vec[2]);
                cout << "[check WORD] ";
            } else {
                errorQue.push(errorStructe(line, 7));
                cout << "WORD format error (decimal)\n";
                return;
            }
        }

        // Pesduo -- base
    } else if (vec.size() == 2 && vec[0] == "BASE") {
        baseRegister = vec[1];
        return;
    }
    // Pesudo -- Start
    else if (vec.size() >= 2 && vec[1] == "START") {
        program_name = vec[0];
        startAddr = newAddressing = stoi(vec[2], nullptr, 16); //用10進位存
        cout << "Program name is " << vec[0]
             << " startNewAddress: " << newAddressing << endl;
        start = true;
        return;
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
            programStartAddr = SymbolTable[vec[1]];
            totalLen = newAddressing - startAddr;
            cout << "newAddressing" << newAddressing << " - startAddr"
                 << startAddr << "=  totallen: " << totalLen;
        } else {
            errorQue.push(errorStructe(line, 9));
            cout << "error9 about (end+) symbol!";
        }
        return;
    }
    // normal code!!
    else if (vec.size() == 1) {
        tmp->codeType = 3;
        if (!opcodeTable.count(vec[0])) {
            errorQue.push(errorStructe(line, 12));
            cout << "cannot find the opcode\n";
            return;
        }
        tmp->len = opcodeTable[vec[0]].first;
        tmp->opcode = opcodeTable[vec[0]].second;
        tmp->loc = newAddressing;
        newAddressing += 3;
    } else if (vec.size() >= 2 && vec.size() <= 4) {
        int ptr = 0; // 將指向opcode 與operand
        // label
        if (!opcodeTable.count(vec[0])) {
            if (!SymbolTable.count(vec[0])) {
                SymbolTable[vec[0]] = newAddressing;
                ptr = 1;
                cout << " label: " << vec[0];
            } else {
                errorQue.push(errorStructe(line, 8));
                cout << "error8 Duplicate Symbol\n";
                return;
            }
        }
        // check opcode format
        //如果有+，消去掉
        bool normal = true;
        if (vec[ptr][0] == '+') {
            vec[ptr] = removeFirstChar(vec[ptr]);
            tmp->xbpe = bitset<4>(0);
            normal = false;
            cout << " check extend format ";
        }
        if (!opcodeTable.count(vec[ptr])) {
            errorQue.push(errorStructe(line, 12));
            cout << "cannot find the opcode\n";
            return;
        }
        tmp->len = opcodeTable[vec[ptr]].first;
        tmp->opcode = opcodeTable[vec[ptr++]].second;

        // 處理operand  (ptr以指向operand的位置了!)
        // STA	DATA, X
        // LDA  DATA
        // LDA #0 、LDA #DATA
        // LDA @DATA

        if (vec[ptr][0] == '#') {
            normal = false;
            tmp->operand = removeFirstChar(vec[ptr]);
            tmp->ni = bitset<2>(string("01"));
            if (checkDecimal(tmp->operand)) {
                tmp->codeType = 2;
            } else {
                tmp->codeType = 3;
            }
        }
        if (vec[ptr][0] == '@') {
            normal = false;
            tmp->operand = removeFirstChar(vec[ptr]);
            tmp->ni = bitset<2>(string("10"));
            tmp->codeType = 3;
        }
        // (限只有一個,) 處理有, x (STA	DATA, X )
        if (vec.size() - 1 == ptr + 1 && vec[ptr][vec[ptr].size() - 2] != ',' &&
            vec[ptr][vec[ptr].size() - 1] == ',' && vec[ptr + 1] == "X") {
            // ,X 不可與#、@ 一起出現
            if (normal) {
                normal = false;
                // remove last one char ','
                string s = "";
                for (int j = 0; j < vec[ptr].size() - 1; j++) {
                    s += vec[ptr][j];
                }
                tmp->operand = s;
                tmp->codeType = 3;
                tmp->xbpe.set(3); //可以這樣寫嗎?
            }
        }
        // 處理 e.g: LDA DATA 與 COMPR r1, r2
        if (normal) {
            tmp->operand = vec[ptr];
            tmp->codeType = 3;
            // 處理COMPR r1, r2 (ptr目前指向r1,)
            if (tmp->len == 2 && vec.size() - 1 == ptr + 1) {
                // operand =  (r1)(r2)
                // string r1 = 去掉','
                string r1;
                for (int i = 0; i < vec[ptr].size() - 1; i++) {
                    r1 += vec[ptr][i];
                }
                string r2 = vec[ptr + 1];
                tmp->operand = r1 + r2;
            }
        }
        // handleNewAddr
        tmp->loc = newAddressing;
        newAddressing += tmp->len;
    }
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
    // tmp->loc = newAddressing; // wrong 要在handlenewAddr前
    intermediate_file.push_back(tmp);
    // cout << " insert_symbol: " << symbol << " "
    //      << "symbolLoc: " << SymbolTable[symbol] << "|" << endl;
    cout << " ni:" << tmp->ni << " xbpe: " << tmp->xbpe
         << " operand: " << tmp->operand << " ctype:" << tmp->codeType
         << " opcodenum:" << tmp->opcode << " len:" << tmp->len
         << " loc:" << tmp->loc << " |" << endl;
}
void Scanner::firstpass() {
    string inputStr;
    ifstream ifs;
    ifs.open(filepath);
    while (getline(ifs, inputStr)) {
        string str = getSubstringUntilFirstDot(inputStr);
        if (str.empty())
            continue;
        vector<string> vec = removeAnnotation_Blank(str);
        if (vec.size() == 0)
            continue;

        cout << endl;
        // print all
        for (int i = 0; i < vec.size(); i++) {
            cout << "|" << vec[i];
        }
        cout << endl;
        identifyCodeType_ErrorType(vec);
    }
}

void Scanner::secondpass() {
    // setBaseregister
    //讀中間檔
    int ctr = 0;
    string aPeiceOfObjectPorgam = " ", objectprogram_Startaddr = "";
    for (int i = 0; i < intermediate_file.size(); i++) {
        midStructure* tmp = intermediate_file[i];
        if (ctr >= 30) {
            cout << "T loc:" << objectprogram_Startaddr << " len:" << ctr
                 << aPeiceOfObjectPorgam;
            ctr = 0;
            objectprogram_Startaddr = tmp->loc;
        }
    }
}
void Scanner::func(string filepath) {
    this->filepath = filepath;
    firstpass();
    secondpass();
    printErrorQue();
}
int main() {
    ifstream ifs;
    string filepath = "testprog1.S";
    Scanner scanner;
    scanner.func(filepath);
    return 0;
}