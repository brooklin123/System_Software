#include <bitset>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;
struct midStructure {
    bitset<4> xbpe;
    bitset<2> ni;
    string opcode, operand;
    // codeType
    // 0 : pesduo
    // 1 : WORD或BYTE(這得operand都是轉好的(hex與ascii對應的hex))
    // 2: #(+數字)
    // 3 : other
    int loc, len, line, codeType;
    vector<string> reg; // 存當operand 是register時用
    midStructure() {
        this->xbpe = bitset<4>(string("0000"));
        this->ni = bitset<2>(string("11"));
        this->opcode = "";
        this->operand = "";
        this->codeType = -1;
        this->len = 0;
        this->line = 0;
    }
};

struct errorStructure {
    int line, errorType;
    errorStructure(int l, int e) {
        this->line = l;
        this->errorType = e;
    }
};
class Assembler {
  public:
    Assembler();
    void func(string);

  private:
    void initOpcode();
    void initRegesterCode();
    void firstpass();
    void secondpass();
    void generateObjectProgram();
    void initErrorTable();
    void printErrorQue();
    void identifyCodeType_ErrorType(vector<string>&);
    int countLen_handleNextNewAddress(int, string);
    void init_format2RegNum();
    bool checkDecimal(string);
    bool checkHex(string);
    char intToHexChar(int);
    string decimalToHex(int);
    string convertToLowerToUpper(string);
    string handleHexComplement(int);
    string getSubstringUntilFirstDot(string);
    string removeFirstChar(string);
    string generateObjectCode(midStructure*); // 處理1 #num 與 (op r1) (op )
    string handleRelative(midStructure*);
    vector<string> removeAnnotation_Blank(string str);
    string hexPlusHex(string, string);
    string padString(int, string);

    bool isStart, isEnd, isBase; // 是否出現
    int locationCounter, line, programStartAddr, totalLen, startAddr, baseAddr,
        baseLine; // decimal!
    string program_name, filepath, baseOperand;
    vector<midStructure*> intermediate_file;
    queue<errorStructure> errorQue;
    unordered_map<string, int> SymbolTable;
    set<string> format2_2reg;
    set<string> format2_1reg;
    // key: mnem  val: <format, opcodeNum>
    unordered_map<string, pair<int, string>> opcodeTable;
    unordered_map<string, string> registerTable;
    unordered_map<int, string> errorTable;
};
Assembler::Assembler() {
    initOpcode();
    initRegesterCode();
    initErrorTable();
    init_format2RegNum();
    isStart = isEnd = isBase = false;
    line = 0;
}
// 建表 處理 format為2的opcode後所需的參數數量
void Assembler::init_format2RegNum() {
    string two_reg[] = {"ADDR", "COMPR",  "DIVR",   "MULR",
                        "RMO",  "SHIFTL", "SHIFTR", "SUBR"};
    string one_reg[] = {"CLEAR", "SVC", "TD", "TIXR"};
    for (string a : two_reg) {
        format2_2reg.insert(opcodeTable[a].second);
    }
    for (string b : one_reg) {
        format2_1reg.insert(opcodeTable[b].second);
    }
}
void Assembler::initErrorTable() {
    errorTable[0] = "Program appears without START statement.";
    errorTable[1] = "Opcode is used as label in the pesudo code.";
    errorTable[2] = "The values following RESB should be in decimal format.";
    errorTable[3] = "The values following RESW should be in decimal format.";
    errorTable[4] = "BYTE error: 'C' is not followed by ASCII value.";
    errorTable[5] = "BYTE error: 'X' is not followed by HEX value.";
    errorTable[6] = "BYTE must be followed by 'X' or 'C'";
    errorTable[7] = "The values following WORD should be in decimal format.";
    errorTable[8] = "Duplicate Symbol.";
    errorTable[9] = "Symbol after END statement is not found.";
    errorTable[10] = "END statement encountered but program continues.";
    errorTable[11] =
        "Label error: label cannot have the same name as an opcode.";
    errorTable[12] = "mnemonic error";
    errorTable[13] = "Undefined symbol";
    errorTable[14] = "not find operand";
    errorTable[15] = "START is followed by a non-hexadecimal address.";
    errorTable[16] = "Not find END";
    errorTable[17] = "Incorrect END format";
    errorTable[18] = "Incorrect START format";
    errorTable[19] = "The string enclosed by the single quotation mark is not "
                     "in the correct format.";
    errorTable[20] =
        "An empty string is not allowed within the single quotation marks.";
    errorTable[21] = "Program name is limited to 6 characters or fewer.";
    errorTable[22] = "error about regester number";
    errorTable[23] = "RSUB does not have an operand";
    errorTable[24] = "size of contend in BYTE C'' is over 30";
    errorTable[25] = "size of contend in BYTE X'' is over 60";
    errorTable[26] = "format 1 is not allowed with operand";
    errorTable[27] = "over the range of base";
}
char Assembler::intToHexChar(int i) {
    if (i >= 0 && i <= 9) {
        return '0' + i;
    } else if (i >= 10 && i <= 15) {
        return 'A' + i - 10;
    } else {
        return '0';
    }
}
string Assembler::decimalToHex(int decimal) {
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
void Assembler::initOpcode() {
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
void Assembler::initRegesterCode() {
    string name[] = {"A", "X", "L", "B", "S", "T", "F", "PC", "SW"};
    string val[] = {"0", "1", "2", "3", "4", "5", "6", "8", "9"};
    for (int i = 0; i < 9; i++) {
        registerTable[name[i]] = val[i];
    }
}
bool Assembler::checkDecimal(string str) {
    for (int j = 0; j < str.size(); j++) {
        if ((str[j] < '0' || str[j] > '9')) {
            return false;
        }
    }
    return true;
}
bool Assembler::checkHex(string str) {
    for (char i : str) {
        if (!isxdigit(i)) {
            return false;
        }
    }
    return true;
}
void Assembler::printErrorQue() {
    // 差: 查errorTable
    cout << "\nprintErrorQue: \n";
    int s = errorQue.size();
    for (int i = 0; i < s; i++) {
        if (errorQue.front().line == -1)
            cout << errorTable[errorQue.front().errorType] << endl;
        else
            cout << errorTable[errorQue.front().errorType]
                 << " in line: " << errorQue.front().line << endl;
        errorQue.pop();
    }
}
string Assembler::padString(int n, string str) {
    if (str.size() < n) {
        string paddedString = str;
        paddedString.insert(paddedString.begin(), n - str.size(), '0');
        return paddedString;
    }
    if (str.size() == n) {
        return str;
    }
    return str;
}
string Assembler::removeFirstChar(string str) {
    string s = "";
    for (int i = 1; i < str.size(); i++) {
        s += str[i];
    }
    return s;
}
string Assembler::convertToLowerToUpper(string input) {
    std::string result;
    for (char ch : input) {
        if (ch >= 'a' && ch <= 'z') { // 判断是否为小写字母
            ch = ch - 'a' + 'A';      // 转换为对应的大写字母
        }
        result += ch; // 添加到结果字符串
    }
    return result;
}
string Assembler::getSubstringUntilFirstDot(string str) {
    size_t dotIndex = str.find('.');
    if (dotIndex != string::npos) {
        return str.substr(0, dotIndex);
    }
    return str;
}
vector<string> Assembler::removeAnnotation_Blank(string str) {
    // 將字串根據空白拆開，塞進vector裡
    istringstream iss(str); // C++風格的串流的输入操作
    vector<string> vec(istream_iterator<string>{iss},
                       istream_iterator<string>());
    return vec;
}
string Assembler::hexPlusHex(string s1, string s2) {
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

int Assembler::countLen_handleNextNewAddress(int kind, string val) {
    if (kind == 1) { //"RESB"
        locationCounter += stoi(val);
        return stoi(val);
    }
    if (kind == 2) { // "RESW"
        locationCounter += stoi(val) * 3;
        return stoi(val) * 3;
    }
    if (kind == 3) {          // "BYTE" X
        locationCounter += 3; // hex -> +3
        return 3;
    }
    if (kind == 4) {
        locationCounter += 1;
        return 1;
    }
    if (kind == 5) { // "BYTE" C
        locationCounter += val.size();
        return val.size();
    }
}
void Assembler::identifyCodeType_ErrorType(vector<string>& vec) {
    midStructure* tmp = new midStructure();
    tmp->line = line;
    string symbol = "", operand = "";
    // Pesudo
    if (vec.size() == 3 && (vec[1] == "RESB" || vec[1] == "RESW" ||
                            vec[1] == "BYTE" || vec[1] == "WORD")) {
        // label
        // label != opcodeName
        if (opcodeTable.count(vec[0])) {
            errorQue.push(errorStructure(line, 1));
            return;

        } else if (SymbolTable.count(vec[0])) {
            errorQue.push(errorStructure(line, 8));
            return;
        } else {
            symbol = vec[0];
        }
        // operand
        // RESB
        if (vec[1] == "RESB") {
            tmp->loc = locationCounter;
            if (checkDecimal(vec[2])) {
                tmp->len = countLen_handleNextNewAddress(1, vec[2]);
                tmp->codeType = 0;
            } else {
                errorQue.push(errorStructure(line, 2));
                return;
            }
        }
        // RESW
        else if (vec[1] == "RESW") {
            tmp->loc = locationCounter;
            if (checkDecimal(vec[2])) {
                tmp->len = countLen_handleNextNewAddress(2, vec[2]);
                tmp->codeType = 0;
            } else {
                errorQue.push(errorStructure(line, 3));
                return;
            }
        }
        // BYTE
        else if (vec[1] == "BYTE") {
            tmp->loc = locationCounter;
            // e.g.:  C'EOF'
            if (vec.size() == 3 && vec[2][0] == 'C' && vec[2][1] == '\'' &&
                vec[2][vec[2].size() - 1] == '\'') {
                // check isAscii
                if (vec[2].size() == 3) {
                    errorQue.push(errorStructure(line, 20));
                    return;
                }
                if (vec[2].size() - 3 > 30) {
                    errorQue.push(errorStructure(line, 2));
                }
                for (int j = 2; j < vec[2].size() - 1; j++) {
                    if (!isascii(vec[2][j])) {
                        errorQue.push(errorStructure(line, 24));
                        return;
                    } else {
                        // 強
                        stringstream hexAscii;
                        // 強制類型轉換為整數
                        int AsciiCode = static_cast<int>(vec[2][j]);
                        // string str_asciiCode = to_string(AsciiCode);
                        hexAscii << hex << AsciiCode;
                        operand += hexAscii.str();
                        tmp->codeType = 1;
                    }
                }
                // handleNextNewAddress
                locationCounter += operand.size() / 2;
                tmp->len =
                    operand.size() / 2; // // 因為目前operand是翻成hex的ascii!
            }
            // e.g.:  X'F1'
            else if (vec.size() == 3 && vec[2][0] == 'X' && vec[2][1] == '\'' &&
                     vec[2][vec[2].size() - 1] == '\'') {
                if (vec[2].size() == 3) {
                    errorQue.push(errorStructure(line, 20));
                    return;
                }
                if (vec[2].size() - 3 > 60) {
                    errorQue.push(errorStructure(line, 25));
                    return;
                }
                // check hex
                for (int j = 2; j < vec[2].size() - 1; j++) {
                    if (!((vec[2][j] >= '0' and vec[2][j] <= '9') ||
                          (vec[2][j] >= 'A' and vec[2][j] <= 'F'))) {
                        errorQue.push(errorStructure(line, 5));
                        return;
                    } else {
                        operand += vec[2][j];
                        tmp->codeType = 1;
                    }
                }
                tmp->len = countLen_handleNextNewAddress(4, operand);
            } else if (!(vec[2][0] == 'X' || vec[2][0] == 'C')) {
                errorQue.push(errorStructure(line, 6));
                return;
            } else {
                errorQue.push(errorStructure(line, 19));
                return;
            }
            tmp->operand = operand;
        }
        // WORD
        else {
            tmp->loc = locationCounter;
            // check decimal
            if (checkDecimal(vec[2])) {
                // operand要轉hex
                tmp->operand = decimalToHex(stoi(vec[2]));
                tmp->codeType = 1;
                tmp->len = countLen_handleNextNewAddress(3, vec[2]);
            } else {
                errorQue.push(errorStructure(line, 7));
                return;
            }
        }
        SymbolTable[symbol] = tmp->loc;

        // Pesduo -- base
    } else if (vec.size() == 2 && vec[0] == "BASE") {
        baseOperand = vec[1];
        baseLine = line;
        return;
    }
    // Pesudo -- Start
    else if (vec.size() == 1 && vec[0] == "START") {
        errorQue.push(errorStructure(line, 18));
        return;
    } else if (vec.size() >= 2 && vec[1] == "START") {
        if (vec.size() != 3) {
            errorQue.push(errorStructure(line, 18));
            return;
        }
        if (vec[0].size() > 6) {
            errorQue.push(errorStructure(line, 21));
            return;
        }
        program_name = vec[0];
        if (!checkHex(vec[2])) {
            errorQue.push(errorStructure(line, 15));
            return;
        }
        startAddr = locationCounter = stoi(vec[2], nullptr, 16); //用10進位存
        isStart = true;
        return;
    } else if (!isStart) {
        errorQue.push(errorStructure(line, 0));
        return;
    }
    // pesudo --END
    else if (isEnd) {
        errorQue.push(errorStructure(line, 10));
        return;
    } else if (vec.size() != 2 && vec[0] == "END") {
        errorQue.push(errorStructure(line, 17));
        return;
    } else if (vec.size() == 2 && vec[0] == "END") {
        if (SymbolTable.count(vec[1])) {
            isEnd = true;
            programStartAddr = SymbolTable[vec[1]];
            totalLen = locationCounter - startAddr;
        } else {
            errorQue.push(errorStructure(line, 9));
        }
        return;
    }
    // normal code!!
    // C X長度

    else if (vec.size() == 1 || (vec.size() >= 2 && vec[0] == "RSUB")) {
        // RSUB operand
        if (vec.size() >= 2 && vec[0] == "RSUB") {
            errorQue.push(errorStructure(line, 23));
            return;
        }
        if (!opcodeTable.count(vec[0])) {
            errorQue.push(errorStructure(line, 12));
            return;
        }
        if (vec[0] != "RSUB" && opcodeTable[vec[0]].first != 1) {
            errorQue.push(errorStructure(line, 14));
            return;
        }
        tmp->len = opcodeTable[vec[0]].first;
        tmp->opcode = opcodeTable[vec[0]].second;
        tmp->loc = locationCounter;
        locationCounter += 3;
    }
    // 對的label opcode1 ,錯的:opcode1 operand1
    // 處理formt == 1的opcode
    else if (vec.size() == 2 &&
             (vec[1] == "RSUB" ||
              (opcodeTable.count(vec[0]) && opcodeTable[vec[0]].first == 1) ||
              (opcodeTable.count(vec[1]) && opcodeTable[vec[1]].first == 1))) {
        if ((opcodeTable.count(vec[0]) && opcodeTable[vec[0]].first == 1)) {
            errorQue.push(errorStructure(line, 26));
            return;
        }
        tmp->len = opcodeTable[vec[1]].first;
        tmp->opcode = opcodeTable[vec[1]].second;
        tmp->loc = locationCounter;
        if (vec[1] == "RSUB") {
            locationCounter += 3;
        } else
            locationCounter += 1;
    } else if (vec.size() >= 2 && vec.size() <= 4) {
        int ptr = 0; // 將指向opcode 與operand
        // label
        string possible_opcode = removeFirstChar(vec[0]); // 避免e.g. +LDA A
        if (!opcodeTable.count(vec[0]) && !opcodeTable.count(possible_opcode)) {
            if (!SymbolTable.count(vec[0])) {
                SymbolTable[vec[0]] = locationCounter;
                ptr = 1;
            } else {
                errorQue.push(errorStructure(line, 8));
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
                tmp->len = 4;
                tmp->opcode = opcodeTable[vec[ptr]].second;
                tmp->xbpe = tmp->xbpe.set(0); // 0001
            } else {
                errorQue.push(errorStructure(line, 12));
                cout << line << " format error : happen +opcode\n";
            }
        } else {
            if (!opcodeTable.count(vec[ptr])) {
                errorQue.push(errorStructure(line, 12));
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
        if (ptr >= vec.size()) {
            errorQue.push(errorStructure(line, 14));
            return;
        }
        if (vec[ptr][0] == '#') {
            normal = false;
            tmp->operand = removeFirstChar(vec[ptr]);
            tmp->ni = bitset<2>(string("01"));
            if (checkDecimal(tmp->operand)) {
                tmp->codeType = 2;
                tmp->operand = decimalToHex(stoi(tmp->operand));
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
                tmp->xbpe = tmp->xbpe.set(3);
            }
        }
        if (vec.size() - 1 == ptr && vec[ptr][vec[ptr].size() - 1] == 'X' &&
            vec[ptr][vec[ptr].size() - 2] == ',' &&
            vec[ptr][vec[ptr].size() - 3] != ',') {
            if (normal) {
                normal = false;
                string str = vec[ptr].substr(0, vec[ptr].size() - 2);
                tmp->operand = str;
                tmp->codeType = 3;
                tmp->xbpe = tmp->xbpe.set(3);
            }
        }
        // 處理 e.g: LDA DATA 與 COMPR r1, r2
        if (normal) {
            if (opcodeTable.count(vec[ptr])) {
                errorQue.push(errorStructure(line, 11));
                return;
            }
            tmp->operand = vec[ptr];
            tmp->codeType = 3;
            string r1 = "", r2 = "";
            // 處理COMPR r1, r2 (ptr目前指向r1,) 或CLEAR R1
            if (tmp->len == 2) {
                if (ptr == vec.size() - 1 &&
                    vec[ptr].find(',') != std::string::npos) {
                    if (format2_2reg.count(tmp->opcode)) {
                        size_t commaPos = vec[ptr].find(',');
                        r1 = vec[ptr].substr(0, commaPos);
                        r2 = vec[ptr].substr(commaPos + 1);
                        tmp->reg.push_back(r1);
                        tmp->reg.push_back(r2);
                    } else {
                        errorQue.push(errorStructure(line, 22));
                        return;
                    }
                } else {
                    // operand =  (r1)(r2)
                    // string r1 = 去掉','
                    if (vec[ptr][vec[ptr].size() - 1] == ',') {
                        if (format2_2reg.count(tmp->opcode)) {
                            for (int i = 0; i < vec[ptr].size() - 1; i++) {
                                r1 += vec[ptr][i];
                            }
                        } else {
                            cout << "regerror2";
                            errorQue.push(errorStructure(line, 22));
                            return;
                        }
                        //沒有,
                    } else {
                        if (!format2_1reg.count(tmp->opcode)) {
                            cout << "regerror3";
                            errorQue.push(errorStructure(line, 22));
                            return;
                        }
                        r1 = vec[ptr];
                    }
                    tmp->reg.push_back(r1);
                    // 有r2
                    if (vec.size() - 1 == ptr + 1) {
                        r2 = vec[ptr + 1];
                        tmp->reg.push_back(r2);
                    }
                }
            }
        }
        if (tmp->codeType == -1)
            tmp->codeType = 3;
        // handleNewAddr
        tmp->loc = locationCounter;
        locationCounter += tmp->len;
    }
    intermediate_file.push_back(tmp);
    // 印出中間檔
    // cout << "line:" << tmp->line << " ni:" << tmp->ni << " xbpe: " <<
    // tmp->xbpe
    //      << " operand: " << tmp->operand << " regs: ";
    // for (int i = 0; i < tmp->reg.size(); i++) {
    //     cout << tmp->reg[i] << " ";
    // }
    // cout << "ctype:" << tmp->codeType << " opcodenum:" << tmp->opcode
    //      << " len:" << tmp->len << " loc:" << tmp->loc << " " << endl;
}
void Assembler::firstpass() {
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
        // print all
        // for (int i = 0; i < vec.size(); i++) {
        //     cout << "|" << vec[i];
        // }
        // cout << "size: " << vec.size() << endl;
        identifyCodeType_ErrorType(vec);
    }
    if (!isEnd) {
        errorQue.push(errorStructure(-1, 16));
    }
}
string Assembler::handleHexComplement(int n) {
    int decimalComplement = ~n + 1;
    string HexString = decimalToHex(decimalComplement);
    string str = "";
    string lastHexString = string(1, HexString.back());
    str = decimalToHex(16 - stoi(lastHexString, nullptr, 16));
    HexString.pop_back();
    while (HexString.size() > 0) {
        str =
            decimalToHex(15 - stoi(string(1, HexString.back()), nullptr, 16)) +
            str;
        HexString.pop_back();
    }
    //前面補F
    string paddedString = str;
    // operand 都是3
    paddedString.insert(paddedString.begin(), 3 - str.size(), 'F');
    return paddedString;
}
string Assembler::generateObjectCode(midStructure* tmp) {
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
            if (!SymbolTable.count(tmp->operand)) {
                errorQue.push(errorStructure(baseLine, 13));
                return "";
            }
            operand = decimalToHex(SymbolTable[tmp->operand]);
        }
        // cout << "[check gene len=4] ";
        // cout << padString(2, hexPlusHex(tmp->opcode, ni)) + "1" +
        //             padString(5, operand);
        return padString(2, hexPlusHex(tmp->opcode, ni)) + "1" +
               padString(5, operand);
    }
    if (tmp->operand == "") {
        return padString(2, hexPlusHex(tmp->opcode, ni)) +
               padString(tmp->len * 2 - 2, "0");
    }
    if (tmp->len == 3) {
        if (tmp->codeType == 2) { // #(+數字)
            // e.g. COMP #3 -> (28+1) 00 03
            operand = tmp->operand;
            return padString(2, hexPlusHex(tmp->opcode, ni)) + "0" +
                   padString(3, operand);
        }
        //大部分: xbpe == 1000 || xbpe ==  0000
        string dispResult = handleRelative(tmp);
        stringstream s_xbpe;
        s_xbpe << hex << tmp->xbpe.to_ulong();
        xbpe = s_xbpe.str();
        if (dispResult == "") { //沒有base
            // cout << "line:" << tmp->line << "\n";
            errorQue.push(errorStructure(baseLine, 13));
            // cout << "line:" << tmp->line << "\n";
            return "";
        }
        if (dispResult == "NOTFINDOPERAND") {
            errorQue.push(errorStructure(tmp->line, 13));
            return "";
        }
        if (dispResult == "OVER") { // over the range of base
            // cout <<  << "!\n";
            errorQue.push(errorStructure(tmp->line, 27));
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
        operand = operand + registerTable[regs[i]];
    }
    if (operand.size() < 2) {
        operand += '0';
    }
    return padString(2, tmp->opcode) + operand; // not sure format == 1的情況
}
string Assembler::handleRelative(midStructure* tmp) {
    if (!SymbolTable.count(tmp->operand)) {
        return "NOTFINDOPERAND";
    }
    int operandAddr = SymbolTable[tmp->operand]; // decimal
    int pc = tmp->loc + tmp->len;
    // PC relative
    int disp = operandAddr - pc;
    if (disp >= 0 && disp <= 2047) {
        tmp->xbpe.set(1);
        return decimalToHex(disp);
    }
    if (disp < 0 && disp >= -2048) {
        tmp->xbpe.set(1);
        return handleHexComplement(disp);
    }
    // BASE relative
    if (!isBase) { //沒有找到base
        // cout << "not find base\n";
        return "";
    }

    disp = operandAddr - baseAddr;
    if (disp >= 0 && disp <= 4095) {
        tmp->xbpe.set(2);
        return decimalToHex(disp);
    } else {
        cout << "disp:!" << disp << endl;
        return "OVER";
    }
}
void Assembler::generateObjectProgram() {
    ofstream out("output.txt", ios::trunc); // 打開文本文件並清空內容
    if (out.is_open()) {                    // 確認文件是否成功打開
        out << convertToLowerToUpper(
            "H " + program_name + " " + padString(6, decimalToHex(startAddr)) +
            " " + padString(6, decimalToHex(totalLen)) + "\n");
        //讀中間檔
        int ctr = 0;
        string TRecord = " ", _Startaddr = decimalToHex(startAddr);
        for (int i = 0; i < intermediate_file.size(); i++) {
            midStructure* tmp = intermediate_file[i];

            if (ctr + tmp->len >= 30 && TRecord != "") {
                out << convertToLowerToUpper(
                    "T " + padString(6, _Startaddr) + "  " +
                    padString(2, decimalToHex(ctr)) + "  " + TRecord + "\n");
                ctr = 0;
            }
            if (ctr == 0) {
                _Startaddr = decimalToHex(tmp->loc);
                TRecord = "";
            }
            //遇到RESB、RESW換行 pesudo
            if (tmp->codeType == 0) {
                // 把還沒輸出完的先印
                if (TRecord != "") {
                    out << convertToLowerToUpper(
                        "T " + padString(6, _Startaddr) + "  " +
                        padString(2, decimalToHex(ctr)) + "  " + TRecord +
                        "\n");
                    ctr = 0;
                    TRecord = "";
                }
                continue;
            }
            // WORD、BYTE
            else if (tmp->codeType == 1) {
                ctr += tmp->len;
                TRecord += tmp->operand + " ";
                continue;
            } else {
                string genstring = generateObjectCode(tmp);
                TRecord += genstring + " ";
                ctr += tmp->len;
            }
        }
        // leave
        if (TRecord != "") {
            out << convertToLowerToUpper(
                "T " + padString(6, _Startaddr) + "  " +
                padString(2, decimalToHex(ctr)) + "  " + TRecord + "\n");
        }
        out << convertToLowerToUpper(
            "E " + padString(6, decimalToHex(programStartAddr)));
        out.close();
    } else {
        cout << "cannot open outputFile" << endl;
        return;
    }
}
void Assembler::secondpass() {
    if (SymbolTable.count(baseOperand)) {
        baseAddr = SymbolTable[baseOperand];
        isBase = true;
    }
    generateObjectProgram();
    if (!errorQue.empty()) {
        printErrorQue();
    } else {
        ifstream in("output.txt");
        if (in.is_open()) {
            string outputLine;
            while (getline(in, outputLine)) { // 逐行讀取文件內容
                cout << outputLine << endl;   // 印出每一行的內容
            }
            in.close();
        } else {
            cout << "cannot open outputFile\n";
        }
    }
}
void Assembler::func(string filepath) {
    this->filepath = filepath;
    firstpass();
    secondpass();
}
int main() {
    ifstream ifs;
    string filepath = "test.S";
    Assembler Assembler;
    Assembler.func(filepath);
    return 0;
}
// 可以進步的是: 用setw()與setfill() 代替 自己寫的padString()