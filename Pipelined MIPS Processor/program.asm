SUB R1, R1, R2     // R1 = -1
INC R2             // R2++
STORE R10, R2, 0   // [R2] = R10
ADD R10, R10, R1   // R10 = R10 - 1
BEQZ R10, 0, 1     // If R10 == 0 Goto HLT
JMP f, b           // JMP to INC R2 instruction
HLT                // Halt
