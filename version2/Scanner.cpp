#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
// 寫一支程式讓使用者讀取範例程式碼，印出行數、token (label mnemonic operand) 和
// addressing mode (direct / index
// addressing。程式要能忽略排版錯誤、註解、空白行。
class Scanner {
  public:
    Scanner();
    void insertSymbolTable(string);
    string searchOpcode(string);
    string hexAdd(string, string);
    void func(string);

  private:
    void handleNextNewAddress(int, string);
    void initOpcode();
    string decimalToHex(int);
    char intToHexChar(int);
    bool DirectOrIndex(vector<string>);
    void printSymbolTable();
    map<string, int> SymbolTable;
    map<string, string> opcodetable;
    bool start, end; // 未出現 //出現兩次要報錯 (?)
    int newAddressing, line;
    string program_name, operand; // operand暫存在class中!
    vector<vector<string>>
};
Scanner::Scanner() {
    initOpcode();
    start = end = false;
    line = 1;
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

string Scanner::hexAdd(string hex1, string hex2) {
    // 將兩個十六進位數字轉換為十進位數字
    int decimal1 = std::stoi(hex1, nullptr, 16);
    int decimal2 = std::stoi(hex2, nullptr, 16);
    int sum = decimal1 + decimal2;
    return decimalToHex(sum);
}

void Scanner::initOpcode() {
    ifstream in;
    in.open("opcode.txt");
    if (in.fail()) {
        cout << "input file opening failed";
        exit(1);
    }
    string mnem, opcode;
    while (in >> mnem >> opcode) {
        opcodetable[mnem] = opcode;
    }
    in.close();
}
void Scanner::insertSymbolTable(string label) {
    if (!SymbolTable.count(label)) {
        SymbolTable[label] = newAddressing;
    } else {
        cout << "duplicate label error \n";
    }
}

string Scanner::searchOpcode(string m) {
    if (opcodetable.count(m))
        return opcodetable[m];
    else {
        // cout << "unvalid\n";
        return "-1";
    }
}
// 0: normal +3
// 1: "RESB" handleNextNewAddress(1, vec[2]);
// 2: "RESW" handleNextNewAddress(2, vec[2]);
// 3: "BYTE" handleNextNewAddress(3, vec[2])
// 4. vec[1] == "WORD" handleNextNewAddress(4, vec[2])
void Scanner::handleNextNewAddress(int kind, string val) { // not finish
    if (kind == 0 || kind == 4) {
        newAddressing += 3;
    } else if (kind == 1) { //"RESB"
        newAddressing += stoi(val);
    } else if (kind == 2) { // "RESW"
        newAddressing += stoi(val) * 3;
    } else if (kind == 3) { // "BYTE"
        if (val[0] == 'X')
            newAddressing += 1;
        else if (val[0] == 'C')
            newAddressing += val.size() - 3; //未去確認格式!!!
        else {
            cout << "BYTE format wrong\n";
        }
    } else {
        cout << "handleNextNewAddress function can't handle this "
                "condition\n";
    }
    return;
}
// return true : Index
// return false : Direct
bool Scanner::DirectOrIndex(vector<string> vec) {
    bool hasComma = false, hasX = false;
    for (int i = 0; i < vec.size(); i++) {
        if (vec[i] == ".") {
            break; //後面不用看了
        }
        if (!hasComma) {
            if (vec[i].find(",") != string::npos) {
                hasComma = true;
            }
        }
        //已找到, 接著找X
        if (hasComma and !hasX) {
            if (vec[i].find("X") != string::npos) {
                hasX = true;
            }
        }
    }
    return (hasComma && hasX);
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
    if (vec[0].find("RSUB.") == string::npos &&
        vec[0].find(".") != string::npos) {
        return;
    }
    //處理start的虛指令
    if (!start && str.find("START") != string::npos) {
        // 待check format!
        program_name = vec[0];
        newAddressing = stoi(vec[2], nullptr, 16); //用10進位存
        cout << "Program name is " << vec[0] << endl;
        cout << "start from this line\n\n";
        return;
    }
    if (vec[0] == "END") {
        cout << "End of the program\n\n";
        end = 1;
        printSymbolTable();
        return;
    }
    if (vec[0] == "RSUB") {
        handleNextNewAddress(0, "");
        cout << "Label:"
             << " Mnemonic: " << vec[0] << " Operand: " << endl
             << endl;
        return;
    }
    bool pesudo = false;
    string label = "", mnemonic = "";
    operand = "";
    if (vec.size() >= 2) {
        // 沒有label
        if (searchOpcode(vec[0]) != "-1") {
            // cout << "size >= 2 and vec[0]!=opcode\n";
            mnemonic = vec[0];
        }
        //有label 第二個值是opcode
        else if (searchOpcode(vec[0]) == "-1" && searchOpcode(vec[1]) != "-1") {
            // cout << "no one can here???";
            label = vec[0];
            insertSymbolTable(vec[0]);
            mnemonic = vec[1];
        } else { //要馬有問題，不然就是pesudo
            pesudo = true;
        }
    }

    // 處理addressing 與印東西!!!
    if (pesudo) {
        if (vec.size() <= 2) {
            cout << vec[0] << " " << vec[1] << endl;
            cout << "pesudo: something wrong\n";
            pesudo = false;
            return;
        }
        if (vec[1] == "RESB" || vec[1] == "RESW" || vec[1] == "BYTE" ||
            vec[1] == "WORD") {
            insertSymbolTable(vec[0]);
            if (vec[1] == "RESB")
                handleNextNewAddress(1, vec[2]);
            else if (vec[1] == "RESW")
                handleNextNewAddress(2, vec[2]);
            else if (vec[1] == "BYTE")
                handleNextNewAddress(3, vec[2]);
            else if (vec[1] == "WORD")
                handleNextNewAddress(4, vec[2]);

            cout << vec[1] << " is pesudo instruction code\n\n";
        }

        else {
            cout << " something wrong\n";
            pesudo = false;
            return;
        }

    } else { // !pesudo
        // opcode存在 判斷direct or index
        // return true : Index
        // return false : Direct
        // cout << "0:" << vec[0] << " 1: " << vec[1] << endl;
        if (DirectOrIndex(vec)) {
            cout << "This is index addressing!\n";
        } else {
            cout << "This is direct addressing!\n";
        }
        cout << "Label:" << label << " Mnemonic: " << mnemonic
             << " Operand: " << operand << endl
             << endl;
        handleNextNewAddress(0, "");
    }
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
// if (start && str.find("start")) {
//     cout << "wrong!\n";
//     return;
// }

//　Base 要跳過，還沒處理
// 沒處理 . 黏在字串的左邊、右邊
// RSUB 沒有處理格式
// 沒有處理 [0]:opcode [1]: opcode 出現兩次的