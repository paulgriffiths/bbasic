# BBasic

[![Build Status](https://github.com/paulgriffiths/bbasic/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/paulgriffiths/bbasic/actions/workflows/c-cpp.yml)

An interpreter for a subset of BBC Basic II, as originally implemented for
the BBC Model B Microcomputer.

## Building and installation

To build from a source distribution, extracting the tarball and run:

```
./configure && make
sudo make install
```

To build from a clone of the repository, run:

```
autoreconf --install
./configure && make
```

## Usage

Some sample BBC BASIC II programs can be found in the `samples` directory,
and can be run as follows:

```
bbasic samples/flag.basic
```

## Supported features

Most features of BBC BASIC II are supported, with the main exception of
those that deal with graphics, memory, and hardware.

The following keywords are supported:

* `ABS` - absolute value
* `ACS` - arc-cosine
* `AND` - logical or bitwise AND
* `ASC` - return ASCII value of character
* `ASN` - arc-sine
* `ATN` - arc-tangent
* `BGET#` - get a byte from a file
* `BPUT#` - put a byte to a file
* `CHR$` - generate a character from its ASCII value
* `CLEAR` - forget all variables previously in use, except resident integers
* `CLOSE#` - close a file
* `COLOUR` - sets text and background colour
* `COS` - cosine
* `COUNT` - counts all the characters printed using PRINT
* `DATA`-`READ` - store and read data items
* `DEF FN` - define a function
* `DEF PROC`-`ENDPROC` - define a procedure
* `DEG` - convert radians to degrees
* `DIM` - create an array
* `DIV` - integer division
* `END` - ends a program
* `EOF#` - determine if end-of-file has been reached
* `EOR` - bitwise exclusive-OR
* `ERL` - line number of last error
* `ERR` - error number
* `EXP` - exponential function
* `EXT#` - size of a file
* `FOR`-`NEXT`-`STEP` - loop construct
* `GET` - wait for a key to be pressed and return the ASCII value
* `GET$` - wait for a key to be pressed and return the character
* `GOSUB` - go to a subroutine
* `GOTO` - go to a line number
* `IF`-`ELSE` - sets up a test condition
* `INKEY` - input the number of the key pressed
* `INKEY$` - input the character of the key pressed
* `INPUT` - input information from the user
* `INPUT#` - input information from a file
* `INSTR` - search one string for the occurrence of another string
* `INT` - integer part of a number
* `LEFT$` - left substring
* `LEN` - counts the number of characters in a string
* `LET` - assign a value to a variable
* `LN` - natural logarithm
* `LOCAL` - inform the computer a variable is local to a procedure or function
* `LOG` - base-10 logarithm
* `MID$` - middle substring
* `MOD` - remainder after division
* `NOT` - logical NOT
* `ON` - alter the order in which BASIC executes a program
* `ON ERROR GOTO` - handle errors
* `ON ERROR GOSUB` - handle errors
* `ON ERROR OFF` - stop handling errors
* `OPENIN` - opens a file for reading
* `OPENOUT` - opens a file for writing
* `OPENUP` - opens a file for updating
* `OR` - logical or bitwise OR
* `PI` - value of PI
* `PRINT` - print words and numbers on the screen
* `PRINT#` - print to a file
* `PTR#` - select which item in a long file is to be read or written next
* `RAD` - convert degrees to radians
* `REM` - remark
* `REPEAT`-`UNTIL` - loop construct
* `REPORT` - report what the last error was
* `RESTORE` - move the data pointer from one set of data to another
* `RETURN` - return from a subroutine
* `RIGHT$` - right substring
* `RND` - generate a random number
* `SGN` - determine the sign of a number
* `SIN` - sine
* `SPC` - print multiple spaces on the screen
* `SQR` - square root
* `STOP` - interrupts a program and prints a message
* `STRING$` - produce multiple copies of a string
* `STR$` - converts a number to a string
* `TAN` - tangent
* `TIME` - set or read the internal timer
* `TRACE` - print each line number before execution
* `VAL` - converts a string to a number

## Differences with BBC BASIC II

* Output of scientific format floating point numbers depends on the
underlying standard C library, and may differ from that on the BBC
Micro.
* Floating point arithmetic is performed at a higher level of
precision, being performed internally with a C `double`.
* Flashing colours are not supported by the `COLOUR` keyword.
* When called with a negative argument, the `RND` argument does
reset the random number generator to a number based on the argument
as required, but - unlike on the BBC Micro - it is not deterministic,
and repeated calls with the same negative argument will reset the
random number generator to different, unpredictable values.
* The special internal file format used by the `PRINT#` and `INPUT#`
keywords differs for real variables. The BBC Micro stores them as
`&FF` followed by four bytes of mantissa and one byte exponent,
while this implementation stores them as `&FF` followed by a C
`double` (64-bits if IEC 60559 is supported by the C implementation,
but this is not guaranteed.)
* Some error codes are not implemented, either because the underlying
functionality is not supported (such as "Bad key", "Bad MODE", "Block?"),
because the errors are caught by the parser before the program runs
(such as "Mistake"), or because a hardware limitation is not enforced
(such as "LINE space" and "No room").
* The original BBC BASIC II allows most keywords to be abbreviated
(e.g. `PRINT` can be abbreviated to `P.`, `PR.`, `PRI.` or `PRIN.`).
This feature existed to maximum the use of limited memory, and is
unnecessary on the kind of modern system which can run this
interpreter, so it is omitted for simplicity and style.
* Use of the `INKEY` and `INKEY$` functions with a negative argument
to check the current state of the keyboard directly, rather than the
input buffer, is not supported.

## Unsupported features

Not yet implemented:

* `EVAL` - evaluate an expression

The following keywords are used during interactive operation, which is
not supported:

* `AUTO` - automatic line numbering
* `CHAIN` - load and run a program
* `DELETE` - delete a group of lines from a program
* `LIST` - list the program in memory
* `LISTO` - set list options
* `LOAD` - load a program
* `NEW` - remove a program from memory
* `OLD` - recover a program which has been recently deleted by NEW
* `RENUMBER` - renumber a program
* `RUN` - run a program
* `SAVE` - save a program

In general, any graphical, sound, assembler and hardware capabilities are
intentionally not supported, including the following keywords:

* `ADVAL` - analogue to digital converter value
* `CALL` - execute a piece of machine code
* `CLG` - clear the graphics screen
* `CLS` - clear the text screen
* `COLOUR` - sets text and background colour. Foreground and background
colours are (optionally) supported, but flashing colours are not.
* `DRAW` - draws lines on the screen
* `ENVELOPE` - control the volume and pitch of sounds
* `GCOL` - set the graphics colour
* `HIMEM` - highest address of program and variables
* `LOMEM` - lowest address of variables
* `MODE` - select the display mode
* `MOVE` - move the graphics cursor
* `OPT` - assembler options
* `OSCLI` - operating system command line interpreter
* `PAGE` - address in memory where BASIC will store the user's program
* `PLOT` - multi-purpose drawing statement
* `POINT` - find out the colour of a certain position on the screen
* `POS` - cursor position
* `SOUND` - generate sounds using the internal loudspeaker
* `TAB` - print tabulation
* `TOP` - address of first free memory location after user's program
* `USR` - call section of machine code program
* `VDU` - send control characters to the VDU drivers
* `VPOS` - vertical position of cursor
* `WIDTH` - overall page width
