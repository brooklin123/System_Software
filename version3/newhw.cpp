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
    bitset<4> xbpe;
    bitset<2> ni;
    string opcode; //(hex)
    string operand;
    int loc, len, line, codeType; // decimal
    vector<string> reg;
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
    void printObjectProgram();
    int countLen_handleNextNewAddress(int, string);
    void identifyCodeType_ErrorType(vector<string>&);
    char intToHexChar(int);
    bool checkDecimal(string);
    string decimalToHex(int);
    string getSubstringUntilFirstDot(string);
    string removeFirstChar(string);
    string generateObjectProgram(midStructure*); // 處理1 #num 與 (op r1) (op )
    string handleRelative(midStructure*);
    vector<string> removeAnnotation_Blank(string str);
    string hexPlusHex(string, string);
    string padString(int, string);

    bool start, end, base; // 未出現 //出現兩次要報錯 (?)
    int newAddressing, line, programStartAddr, totalLen, startAddr, baseAddr,
        baseLine;                               // decimal!
    string program_name, filepath, baseOperand; // operand暫存在class中!
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
    start = end = base = false;
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
        cout << errorQue.front().line << " " << errorQue.front().errorType
             << "| ";
        errorQue.pop();
    }
    cout << " ";
}
string Scanner::padString(int n, string str) {
    if (str.size() < n) {
        string paddedString = str;
        paddedString.insert(paddedString.begin(), n - str.size(), '0');
        return paddedString;
    }
    if (str.size() == n) {
        // cout << "str " << str << " == size<<" << n << "\n";
        return str;
    }
    cout << "str " << str << " over size<<" << n << "\n";
    return str;
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
string Scanner::hexPlusHex(string s1, string s2) {
    // 將十六進位數字轉換為整數
    stringstream ss;
    ss << hex << s1;
    unsigned int num1;
    ss >> num1;
    ss.clear();
    ss << hex << s2;
    unsigned int num2;
    ss >> num2;
    // 計算加總
    unsigned int sum = num1 + num2;
    // 將加總轉換為十六進位字串
    stringstream result_ss;
    result_ss << std::hex << sum;
    return result_ss.str();
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
    // line++;
    midStructure* tmp = new midStructure();
    tmp->line = line;
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

        } else if (SymbolTable.count(vec[0])) {
            errorQue.push(errorStructe(line, 8));
            cout << "error8 Duplicate Symbol\n";
            return;
        } else {
            symbol = vec[0];
        }
        // operand
        // RESB
        if (vec[1] == "RESB") {
            tmp->loc = newAddressing;
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
            tmp->loc = newAddressing;
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
        cout << "set: " << symbol << " in " << tmp->loc << " ";
        SymbolTable[symbol] = tmp->loc;

        // Pesduo -- base
    } else if (vec.size() == 2 && vec[0] == "BASE") {
        baseOperand = vec[1];
        baseLine = line;
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
            cout << "END operand: " << vec[1];
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
        string possible_opcode = removeFirstChar(vec[0]); // 避免e.g. +LDA A
        if (!opcodeTable.count(vec[0]) && !opcodeTable.count(possible_opcode)) {
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
        bool normal = true;
        //如果有+，消去掉
        if (vec[ptr][0] == '+') {
            vec[ptr] = removeFirstChar(vec[ptr]);
            if (opcodeTable.count(vec[ptr]) &&
                opcodeTable[vec[ptr]].first == 3) {
                normal = false;
                tmp->len = 4;
                tmp->opcode = opcodeTable[vec[ptr]].second;
                tmp->xbpe = tmp->xbpe.set(0); // 0001
                cout << " [extend format] ";
            } else {
                cout << "format error : happen +opcode\n";
            }
        } else {
            if (!opcodeTable.count(vec[ptr])) {
                errorQue.push(errorStructe(line, 12));
                cout << "cannot find the opcode\n";
                return;
            }
            tmp->len = opcodeTable[vec[ptr]].first;
            tmp->opcode = opcodeTable[vec[ptr]].second;
        }
        ptr += 1;
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
        // (限只有一個,) 處理有, x (STA	DATA, X )且
        if (tmp->len != 2 && vec.size() - 1 == ptr + 1 &&
            vec[ptr][vec[ptr].size() - 2] != ',' &&
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
                tmp->xbpe = tmp->xbpe.set(3); //???
            }
        }
        // 處理 e.g: LDA DATA 與 COMPR r1, r2
        if (normal) {
            tmp->operand = vec[ptr];
            tmp->codeType = 3;
            // 處理COMPR r1, r2 (ptr目前指向r1,) 或CLEAR R1
            if (tmp->len == 2) {
                // operand =  (r1)(r2)
                // string r1 = 去掉','
                string r1 = "";
                if (vec[ptr][vec[ptr].size() - 1] == ',') {
                    for (int i = 0; i < vec[ptr].size() - 1; i++) {
                        r1 += vec[ptr][i];
                    }
                } else {
                    r1 = vec[ptr];
                }
                tmp->reg.push_back(r1);
                // 有r2
                if (vec.size() - 1 == ptr + 1) {
                    string r2 = vec[ptr + 1];
                    tmp->reg.push_back(r2);
                }

                cout << "see the the register";
                for (string r : tmp->reg) {
                    cout << r << " ";
                }
                cout << endl;
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
        line++;
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

string Scanner::generateObjectProgram(midStructure* tmp) {
    string operand = "";
    // 求出ni與xbpe的string
    stringstream s_xbpe, s_ni;
    s_xbpe << hex << tmp->xbpe.to_ulong();
    s_ni << hex << tmp->ni.to_ulong();
    string xbpe = s_xbpe.str(), ni = s_ni.str();

    if (tmp->len == 4) {          // extend
        if (tmp->codeType == 2) { // #(+數字)
            operand = tmp->operand;
        } else {
            operand = decimalToHex(SymbolTable[tmp->operand]);
        }
        // cout << "[check gene len=4] ";
        // cout << padString(2, hexPlusHex(tmp->opcode, ni)) + "1" +
        //             padString(5, operand);
        return padString(2, hexPlusHex(tmp->opcode, ni)) + "1" +
               padString(5, operand);
    }
    if (tmp->operand == "") {
        // cout << "opeprand of RSUB: "
        //      << padString(2, hexPlusHex(tmp->opcode, ni)) +
        //             padString(tmp->len * 2 - 2, "0");
        return padString(2, hexPlusHex(tmp->opcode, ni)) +
               padString(tmp->len * 2 - 2, "0");
    }
    if (tmp->len == 3) {
        if (tmp->codeType == 2) { // #(+數字)
            // e.g. COMP #3 -> (28+1) 00 03
            operand = tmp->operand;
            // cout << "len = 3 and #num: "
            //      << padString(2, hexPlusHex(tmp->opcode, ni)) + "0" +
            //             padString(3, operand);
            return padString(2, hexPlusHex(tmp->opcode, ni)) + "0" +
                   padString(3, operand);
        }
        // return  (opcode+ni)(xbpe)(operand)
        //大部分: xbpe == 1000 || xbpe ==  0000
        string dispResult = handleRelative(tmp);
        // xbpe 要重算
        stringstream s_xbpe;
        s_xbpe << hex << tmp->xbpe.to_ulong();
        xbpe = s_xbpe.str();
        if (dispResult == "") { //沒有base
            errorQue.push(errorStructe(baseLine, 13));
            cout << "not find base\n";
            return "";
        }
        if (dispResult == "-1") {
            errorQue.push(errorStructe(tmp->line, 13));
            cout << "undefine operand\n";
            return "";
        }
        // cout << "(" << padString(2, hexPlusHex(tmp->opcode, ni)) << " " <<
        // xbpe
        //      << " " << padString(3, dispResult) << ")\n";
        return padString(2, hexPlusHex(tmp->opcode, ni)) + xbpe +
               padString(3, dispResult);
    }
    // format == 2 || format == 1  [r1 r2]
    // 還沒補0!!
    vector<string> regs = tmp->reg;
    for (int i = 0; i < regs.size(); i++) {
        operand += registerTable[regs[i]];
    }
    // cout << "reg size" << regs.size() << " operand of format2:" << operand
    //      << endl;
    return padString(2, tmp->opcode) +
           padString(2, operand); // not sure format == 1的情況
}
string Scanner::handleRelative(midStructure* tmp) {
    if (!SymbolTable.count(tmp->operand)) {
        return "-1";
    }
    int operandAddr = SymbolTable[tmp->operand]; // decimal
    int pc = tmp->loc + tmp->len;
    // PC relative
    int disp = operandAddr - pc;
    // cout << "disp of pc: " << tmp->operand << "/" << operandAddr << " - " <<
    // pc
    //      << " = " << disp << " \n";
    if (disp >= 0 && disp <= 2047) {
        // cout << "operand of pc: " << disp << endl;
        tmp->xbpe.set(1);
        // cout << "xbpe!! " << tmp->xbpe << " ";
        return decimalToHex(disp);
    }
    if (disp < 0 && disp >= -2048) {
        // cout << "-> twoComplememt: " << ~disp + 1;
        return to_string(~disp + 1);
    }
    // BASE relative
    if (!base) { //沒有找到base
        return "";
    }
    disp = operandAddr - baseAddr;
    if (disp >= 0 && disp <= 4095) {
        return decimalToHex(disp);
    }
    // exetened
    return "0" + decimalToHex(operandAddr);
}
void Scanner::printObjectProgram() {
    cout << "H " << program_name << " "
         << padString(6, decimalToHex(startAddr)) + " " +
                padString(6, decimalToHex(totalLen))
         << endl;
    //讀中間檔
    int ctr = 0;
    string aPeiceOfObjectPorgam = " ",
           objectprogram_Startaddr = decimalToHex(startAddr);
    for (int i = 0; i < intermediate_file.size(); i++) {
        midStructure* tmp = intermediate_file[i];

        if (ctr + tmp->len >= 30) {
            cout << "T " << objectprogram_Startaddr << "  "
                 << padString(2, decimalToHex(ctr)) << "  "
                 << aPeiceOfObjectPorgam << endl;
            ctr = 0;
            // objectprogram_Startaddr = decimalToHex(tmp->loc);
            // aPeiceOfObjectPorgam = "";
        }
        if (ctr == 0) {
            objectprogram_Startaddr = decimalToHex(tmp->loc);
            aPeiceOfObjectPorgam = "";
        }
        //遇到RESB、RESW換行 pesudo
        if (tmp->codeType == 0) {
            // 把還沒輸出完的先印
            if (aPeiceOfObjectPorgam != "") {
                cout << "T " << padString(6, objectprogram_Startaddr) << " "
                     << padString(2, decimalToHex(ctr)) << " "
                     << aPeiceOfObjectPorgam << endl;
                ctr = 0;
                aPeiceOfObjectPorgam = "";
            }
            // cout << "[RESB, RESW] \n";
            continue;
        }
        // WORD
        else if (tmp->codeType == 1) {
            // cout << "(this pesudo pass)";
            if (aPeiceOfObjectPorgam != "") {
                cout << "T " << padString(6, objectprogram_Startaddr) << "  "
                     << padString(2, decimalToHex(ctr)) << "  "
                     << aPeiceOfObjectPorgam << endl;
                ctr = 0;
            }
            // 如果WORD、BYTE是連續 我沒有處理!
            cout << "T " << padString(6, decimalToHex(tmp->loc)) << "  "
                 << padString(2, decimalToHex(tmp->len)) << "  " << tmp->operand
                 << endl;
            continue;
        } else {
            string genstring = generateObjectProgram(tmp);
            // cout << "\n(" << tmp->line << ") is a middle line: " <<
            // genstring;
            aPeiceOfObjectPorgam += genstring + " ";
            ctr += tmp->len;
        }
    }
    cout << "E " << padString(6, decimalToHex(programStartAddr));
}
void Scanner::secondpass() {
    // setbaseOperand -> 可以多寫成function 但name要改!
    if (SymbolTable.count(baseOperand)) {
        baseAddr = SymbolTable[baseOperand];
        base = true;
    }
    printObjectProgram();
}
void Scanner::func(string filepath) {
    this->filepath = filepath;
    firstpass();
    cout << "\n~~~~~~~~secondPass~~~~~~\n";
    secondpass();
    printErrorQue();
}
int main() {
    ifstream ifs;
    string filepath = "test.S";
    Scanner scanner;
    scanner.func(filepath);
    return 0;
}