// This file is part of www.nand2tetris.org
// and the book "The Elements of Computing Systems"
// by Nisan and Schocken, MIT Press.
// File name: projects/04/Fill.asm

// Runs an infinite loop that listens to the keyboard input.
// When a key is pressed (any key), the program blackens the screen,
// i.e. writes "black" in every pixel;
// the screen should remain fully black as long as the key is pressed. 
// When no key is pressed, the program clears the screen, i.e. writes
// "white" in every pixel;
// the screen should remain fully clear as long as no key is pressed.


(RESET)
  @SCREEN
  D=A
  @location
  M=D

  @24576 // address of last 16-bit word representing the screen
  D=A
  @max
  M=D

(LOOP)
  @KBD
  D=M
  @SETWHITE
  D;JEQ
  @SETBLACK
  D;JNE

  (SETWHITE)
    @color
    M=0
    @DRAW
    0;JMP

  (SETBLACK)
    @color
    M=-1

  (DRAW)
    @location
    D=M
    @max
    D=D-M
    @RESET
    D;JGE

    @color
    D=M
    @location
    A=M
    M=D

    @location
    M=M+1

    @LOOP
    0;JMP
