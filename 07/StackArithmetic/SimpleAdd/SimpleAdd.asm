// Initialize the stack pointer
@256
D=A
@SP
M=D
// push constant 7
@7
D=A
@SP
A=M
M=D
@SP
M=M+1

// push constant 8
@8
D=A
@SP
A=M
M=D
@SP
M=M+1

// add
@SP
M=M-1
@SP
A=M
D=M

@SP
M=M-1

@SP
A=M
M=M+D
@SP
M=M+1

// End with an infinite loop
(END)
@END
0;JMP
