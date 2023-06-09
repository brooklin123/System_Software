# System_Software

# codeType
0: easy format
1: # 數字(十進位)
2: # label
3: @
4: ,x
5: +
6: WORD 
7: BASE
8: pesudo

# errorType
0: 沒有出現START 就有程式
1: pesudo code 的label使用opcode
2: RESB format error 後面不是加decimal
3: RESW format error 後面不是加decimal
4: BYTE error C 後面不是加 ASCII
5: BYTE error X 後面不是加 HEX
6: BYTE format 不符合 (X或C) ' '或是長度超過
7: WORD error 後面要加decimal
8: Duplicate Symbol

# handleNextAddress
0: normal +3
1: "RESB"   : +operand (decimal)
2: "RESW"   : stoi(operand) * 3
3: "WORD"   : +3
4: "BYTE" X : +3
5: "BYTE" C : +operand.size()