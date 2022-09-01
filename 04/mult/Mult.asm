// This file is part of www.nand2tetris.org
// and the book "The Elements of Computing Systems"
// by Nisan and Schocken, MIT Press.
// File name: projects/04/Mult.asm

// Multiplies R0 and R1 and stores the result in R2.
// (R0, R1, R2 refer to RAM[0], RAM[1], and RAM[2], respectively.)
//
// This program only needs to handle arguments that satisfy
// R0 >= 0, R1 >= 0, and R0*R1 < 32768.

// i = 0
@i
M=0

// sum = 0
@sum
M=0

(LOOP)
// if (i < R1)
@i
D=M
@R1
D=D-M
@STOP
D;JGE

// sum = sum + R0
@sum
D=M
@R0
D=D+M
@sum
M=D

// i = i  +1
@i
M=M+1

// restart loop
@LOOP
0;JMP

// save the sum into R2
(STOP)
@sum
D=M
@R2
M=D

(END)
@END
0;JMP

