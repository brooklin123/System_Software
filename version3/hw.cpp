#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <pair>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;
// searchCode 還沒改完!!!!!!!!!!!!!!!!!
struct midStructure {
    // Error type 有幾種沒確定
    // codeType 也沒確定
    // 0: easy format
    // 1: # 數字(十進位)
    // 2: # label
    // 3: @
    // 4: ,x
    // 5: +
    vector<bool> errorType; // 0 mean no error -> 用bitwise就可以了!!
    vector<bool> codeType;  // 用bitwise!
    string opcode;          // hex
    int opcodeformat;
    string operand;
    int loc, line; // 用10進位處理
    midStructure() {
        this->errorType = vector<int>(11, 0);
        this->codeType = vector<int>(11, 0);
    }
};

class Scanner {
  public:
    Scanner();
    void insertSymbolTable(string);
    string searchOpcode(string);
    void func(string);
    // void firstpass(string);
    // void secondpass(string);

  private:
    void handleNextNewAddress(int, string);
    void initOpcode();
    void intiRegesterCode();
    string decimalToHex(int);
    char intToHexChar(int);
    void printSymbolTable();
    void nixbpe(string);
    bool checkDecimal(string);
    string removeFirstChar(string);
    unordered_map<string, int> SymbolTable;
    unordered_map<string, pair<int, string>> opcodetable; // format, opcodeVal
    unordered_map<string, char> registerTable;
    bool start, end; // 未出現 //出現兩次要報錯 (?)
    int newAddressing, line;
    string program_name, operand; // operand暫存在class中!
    vector<midStructure*> intermediate_file;
};
Scanner::Scanner() {
    initOpcode();

    intiRegesterCode();
    start = end = false;
    line = 1;
}
string Scanner::removeFirstChar(string str) {
    string s = "";
    for (int i = 1; i < str.size(); i++) {
        s += str[i];
    }
    return s;
}
bool Scanner::checkDecimal(string str) {
    for (int j = 0; j < str.size(); j++) {
        if ((str[j] < '0' || str[j] > '9')) {
            return false;
        }
    }
    return true;
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
        opcodetable[mnem] = make_pair(format, opcode);
    }
    in.close();
}
void Scanner::intiRegesterCode() {
    string name[] = {"A", "X", "L", "B", "S", "T", "F", "PC", "SW"};
    char val[] = {'0', '1', '2', '3', '4', '5', '6', '8', '9'};
    for (int i = 0; i < 9; i++) {
        registerTable[name[i]] = val[i];
    }
}
void Scanner::insertSymbolTable(string label) {
    if (!SymbolTable.count(label)) {
        SymbolTable[label] = newAddressing;
    } else {
        cout << "duplicate label error \n";
    }
}

pair<int, string> Scanner::searchOpcode(string m) {
    if (opcodetable.count(m))
        return opcodetable[m];
    else {
        // cout << "unvalid\n";
        return NULL;
    }
}
// 0: normal +3
// 1: "RESB" handleNextNewAddress(1, vec[2]);
// 2: "RESW" handleNextNewAddress(2, vec[2]);
// 3. "WORD" handleNextNewAddress(3, vec[2])
// 4: "BYTE" X
// 5: "BYTE" C

void Scanner::handleNextNewAddress(int kind, string val) { // not finish
    if (kind == 0 || kind == 3) {
        newAddressing += 3;
    } else if (kind == 1) { //"RESB"
        newAddressing += stoi(val);
    } else if (kind == 2) { // "RESW"
        newAddressing += stoi(val) * 3;
    } else if (kind == 4) { // "BYTE" X
        newAddressing += 3; // hex -> +3
    } else if (kind == 5) { // "BYTE" C
        newAddressing += val.size();
        else {
            cout << "BYTE format wrong\n";
        }
    } else {
        cout << "handleNextNewAddress function can't handle this "
                "condition\n";
    }
    return;
}
void Scanner::printSymbolTable() {
    cout << "Symbol Table: {";
    auto i = SymbolTable.begin();
    for (auto it = SymbolTable.begin(); it != SymbolTable.end(); it++) {
        cout << "'" << it->first << "': '0x" << hex << it->second << "', ("
             << end;
    }
    cout << "}\n";
}
void Scanner::func(string str) {
    midStructure* tmp;
    if (end)
        return;
    cout << line++ << ": " << str << endl;
    if (str == "")
        return;
    // 將字串根據空白拆開，塞進vector裡
    istringstream iss(str); // C++風格的串流的输入操作
    vector<string> vec(istream_iterator<string>{iss},
                       istream_iterator<string>());

    // why要這行??
    // if (vec[0].find("RSUB.") == string::npos &&
    //     vec[0].find(".") != string::npos) {
    //     return;
    // }
    //處理start的虛指令
    if (!start && str.find("START") != string::npos) {
        // 待check format!
        program_name = vec[0];
        newAddressing = stoi(vec[2], nullptr, 16); //用10進位存
        cout << "Program name is " << vec[0] << endl;
        return;
    }
    if (vec[0] == "END") {
        end = 1;
        printSymbolTable();
        return;
    }
    if (vec[0] == "RSUB") {
        handleNextNewAddress(0, "");
        return;
    }

    // 接下來 Check codeType 與errorType
    bool pesudo = false, error = false;
    bool findOpcode = false, findLabel = false, findOperand = false;
    bool easyformat = true;
    // ??
    // 先處理+opcdoe
    if (vec[1][0] = '+') {
        //+ 拿掉，後把opcode 塞回去
        vec[1] = removeFirstChar(vec[i]);
        tmp->codeType[5] = 1;
        cout << "Type: extended; after removefirstChar:" << vec[1] << endl;
    }

    for (int i = 0; i < vec.size(); i++) {
        // / 如果RSUB . 不可以錯???? (有嗎)
        if (vec[i] == '.' && ((findOpcode && findOperand) || pesudo))
            break;
        // 錯誤: 如果有: label .或 label ADD .
        // 應該要用opcode的format去判斷

        // 錯誤: 例如 label RESW 3 3 (第3是.已經被上面的做完了)
        if (pesudo) {
            cout << "error : pesduo foramt | e.g. label RESW 3 3 \n";
            return;
        }
        if (vec[i] == '.') {
            // RUSB .
            if (tmp->opcodeformat == 1 && findOpcode) {
                break;
            }
            // opcodeformat > 1
            if (!findOpcode) {
                cout << "error: not find opcode before annotation | e.g. label "
                        ".\n";
                return;
            }

            if (findOpcode && !findOperand) {
                cout << "error: not find operand before annotation | e.g. "
                        "label ADD ."
            }
            cout << "test .\n"; // 不知道有沒有沒想到的
        }

        // find label and opcode
        if (!findOpcode) {
            // label 不可與opcode 同名!
            if (!findLabel && !searchCode(vec[i])) {
                findLabel = true;
                insertSymbolTable(vec[i]); // 還未處理重複symbol
            } else if (searchCode(vec[i])) {
                findOpcode = true;
                tmp->opcodeformat = searchCode(vec[i]).first;
                tmp->opcode = searchCode(vec[i]).second;
                continue;
            } else { // !findopcode and findlabel => error
                cout << "erorr format: not find opcode\n";
            }
        }

        string tmp_operand = "";
        // 處理pesudo 寫完，have not tested!!!!!!!!!!!
        if (vec[1] == "RESB" || vec[1] == "RESW" || vec[1] == "BYTE" ||
            vec[1] == "WORD") {
            if (vec[1] == "RESB") {
                if (checkDecimal(vec[2])) {
                    handleNextNewAddress(1, vec[2]);
                    tmp->operand = vec[2];
                } else {
                    cout << "RESB format error (decimal)\n";
                    return;
                }
            } else if (vec[1] == "RESW") {
                if (checkDecimal(vec[2])) {
                    handleNextNewAddress(2, vec[2]);
                    tmp->operand = vec[2];
                } else {
                    cout << "RESW format error (decimal)\n";
                    return;
                }

            } else if (vec[1] == "BYTE") {
                // e.g.:  C'EOF' X'F1'
                if (vec[2][0] == 'C' && vec[2][1] == '\'' &&
                    vec[2][vec[2].size() - 1] == '\'') {
                    // check isAscii
                    for (int j = 2; j < vec[2].size() - 1; j++) {
                        if (!isascii(vec[2][j])) {
                            cout << "pesudo BYTE operand error!(ASCII)\n";
                            return;
                        } else {
                            stringstream hexAscii;
                            // 強制類型轉換為整數
                            int AsciiCode = static_cast<int>(vec[2][j]);
                            // string str_asciiCode = to_string(AsciiCode);
                            hexAscii << hex << AsciiCode;
                            tmp_operand += hexAscii.str();
                        }
                    }
                    tmp->operand = tmp_operand;
                    handleNextNewAddress(5, tmp_operand);
                }
                if (vec[2][0] == 'X' && vec[2][1] == '\'' &&
                    vec[2][vec[2].size() - 1] == '\'') {
                    // check hex
                    for (int j = 2; j < vec[2].size() - 1; j++) {
                        if (!((vec[2][j] >= '0' and vec[2][j] <= '9') ||
                              (vec[2][j] >= 'A' and vec[2][j] <= 'F'))) {
                            cout << "pesudo BYTE operand error! (HEX)\n";
                            return;
                        } else {
                            tmp_operand += vec[2][i];
                        }
                    }
                    tmp->operand = tmp_operand;
                    handleNextNewAddress(4, tmp_operand);
                }
            } else { // vec[1] == "WORD"
                // check decimal
                if (checkDecimal(vec[2])) {
                    handleNextNewAddress(3, vec[2]);
                    tmp->operand = vec[2];
                } else {
                    return;
                }
            }
            pesudo = true;
            i += 2; // 跳到vec[3]
        }           // have handle pesudo

        // operand
        // not sure 有沒想到的
        if (findOpcode && !findOperand) {
            if (vec[i].size() >= 2) {
                //不可以 #@operand
                if ((vec[i][0] == '@' && vec[i][1] == '#') ||
                    (vec[i][1] == '@' && vec[i][0] == '#')) {
                    cout << "operand error: @# or #@\n";
                    return;
                }
                if (vec[i][0] == '#') {
                    tmp->operand = removeFirstChar(vec[i]);
                    if (checkDecimal(tmp->operand)) {
                        tmp->codeType[1] = 1;
                    } else {
                        tmp->codeType[2] = 1;
                    }
                    easyformat = false;
                }
                if (vec[i][0] == '@') {
                    tmp->operand = removeFirstChar(vec[i]);
                    tmp->codeType[3] = 1;
                    easyformat = false;
                }
                // ,X  (限只有一個,)
                if (vec[i][vec[i].size() - 3] != ',' &&
                    vec[i][vec[i].size() - 2] == ',' &&
                    vec[i][vec[i].size() - 1] == 'X') {
                    // ,X 不可與#、@ 一起出現
                    if (easyformat) {
                        easyformat = false;
                        // remove last two char
                        string s = "";
                        for (int j = 0; j < vec[i].size() - 2; j++) {
                            s += vec[i][j];
                        }
                        tmp->operand = s;
                        tmp->codeType[3] = 1;
                    } else { // 同時存在(# or @) 與 ,X
                        cout << "opernad error: (# || @) and ,X\n";
                        return;
                    }
                }
                if (easyformat) {
                    tmp->operand = vec[i];
                    tmp->codeType[0] = 1;
                }
            }
            // if vec[i].size() == 1 e.g. opernad = a
            else {
                if (vec[i][0] == '@' || vec[i][0] == ',' || vec[i][0] == '#') {
                    cout << "operand error: when size() == 1 \n";
                    return;
                }
                findOperand = true;
                tmp->operand = vec[i];
                tmp->codeType[2] = ; // easy format
            }
        }
    }
    // findoperand記得設!
    intermediate_file.push_back(tmp);
}
int main() {
    ifstream ifs;
    ifs.open("testprog.S");
    string str;
    Scanner scanner;
    while (getline(ifs, str) && !ifs.eof()) {
        scanner.func(str);
    }
    return 0;
}
// // 發現第二次start (但還需要end已出現)

//　Base 要跳過，還沒處理
// 沒處理 . 黏在字串的左邊、右邊
// RSUB 沒有處理格式

// 1. 註解
// 2. 重複symbol
// 3 loc (handleNewAddr 前)、line