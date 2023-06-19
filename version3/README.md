# System_Software

# codeType
0: pesduo
1: WORD或BYTE         (這得operand都是轉好的 (hex與ascii對應的hex))
2: #(+數字)
3: other

# errorType
0: 沒有出現START 就有程式 [V]

1: pesudo code 的label使用opcode
2: RESB format error 後面不是加decimal
3: RESW format error 後面不是加decimal
4: BYTE error C 後面不是加 ASCII
5: BYTE error X 後面不是加 HEX
6: BYTE format 不符合 (X或C) ' '或是其他錯誤
7: WORD error 後面要加decimal
8: Duplicate Symbol [V]

9: 找不到END 後接的symbol [V]
10: 已經END 但還有程式  [V]

11: label error  label不可與opcode同名 [V]
12: mnemonic error (找不到該opcode) [V]
13: undfine symbol (在base 或普通) [V]
14: not find operand 

15: START 要接非十六進位 [V]
16: 沒有找到END [V]
17: wrong END format [V]


operand 吃不同各數register!!!
->435 !!!!
# handleNextAddress
0: normal   : +opcode format
1: "RESB"   : +operand (decimal)
2: "RESW"   : stoi(operand) * 3
3: "WORD"   : +3
4: "BYTE" X : +1
5: "BYTE" C : +operand.size()
