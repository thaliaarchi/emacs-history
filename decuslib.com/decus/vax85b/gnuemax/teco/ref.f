.TCA
.NS 0 2 
.PH "'\*(DT'TECO REFERENCE LIST'- % -'" 
\  
.SP 20 
.CE 
TECO REFERENCE LIST 
.SP 2 
.CE 
Tom Almy 
.SK 
.HU "INTRODUCTION" 
.P 
This is a concise reference of all commands on TEK UNIX TECO. 
All commands are listed in upper case (lower is always valid). 
Control characters may be either typed in directly or using "^" construct, 
most of the time. 
The escape character is represented as "$"; the dollar sign is not used as 
a command in TECO. 
Lower case letters represent arguments as follows: 
.VL 10 
.LI n 
Optional numeric argument, signed. 
When absent, default is 0 or 1 depending on which makes most sense! 
A lone "-" is interpreted to be "-1". 
In "line oriented" commands, positive values mean "after the nth linefeed", 
while negative (or zero) values mean "just after the 1-nth previous linefeed". 
.LI m,n 
Pair of numeric arguments in most commands means from the m+1th 
to and including the nth character. 
In flag commands means AND current value with m then OR it with n (and store 
this value). 
.LI q 
Specifies a "Q-register". 
Valid values are A-Z, 0-9. 
.LI s 
A string argument either <string>$ or <arbitrary character> 
<string not including arbitrary char> <arbitrary char>. 
In the latter case, the command is preceded with "@". 
.LI ss 
Two string arguments, either <string>$<string>$ or <arbitrary char> 
<string not including arbitrary char> <arb char> <string not including arb char> 
<arb char>. 
In the latter case, the command is preceded with "@". 
.LE 
.HU "IMMEDIATE ACTION AIDS" 
.P 
These commands are valid immediately after TECO has prompted for input. 
.VL 10 
.LI *q 
Will save previous command string in Q-register. 
.LI ? 
If previous command caused an error, will print command string up to error. 
.LI linefeed 
Performs "1lt". 
.LI BACKSPACE 
Performs "-1lt". 
.LE 
.HU "NON-COMMANDS" 
.VL 15 
.LI "CR,LF,SPACE" 
These characters are ignored in commands (note that tab is not ignored). 
.LI NULL 
This character is screened out on input. 
.LI "?" 
Toggles trace mode. 
.LI "!text!" 
Comments 
.LI $ 
The escape character, singly, is a NO-OP command that will use up any 
numeric value that precedes it. 
.LE 
.HU "IMMEDIATE MODE COMMANDS" 
.P 
These commands are executed immediately when TECO is prompting for command 
string input. 
Those commands that contain control characters must be entered using 
the characters and not the caret-character equivalents. 
.VL 10 
.LI "$$" 
Start command execution. 
.LI Backspace 
Delete previous character. 
.LI "^U" 
Delete current line of command. 
.LI "^G^G" 
Delete entire command string. 
.LI "^G<space>" 
Retype the current command line. 
.LI "^G*" 
Retype the current command. 
.LI "^C" 
Delete entire command string. 
.LE 
.HU "EXECUTION MODE COMMANDS" 
.P 
The following commands may be executed when TECO is executing a command 
string: 
.VL 10 
.LI "^O" 
Toggles the printout off and on. 
Return to command mode forces printout on. 
.LI "^S" 
Temporarily stops printout. 
.LI "^Q" 
Resumes printout stopped via "^S". 
.LI "^C" 
Aborts execution and causes "XAB" error. 
.LE 
.HU "FILE SPECIFICATION COMMANDS" 
.VL 10 
.LI ERs 
Specifies the input file, any previous file is "closed". 
.LI EWs 
An EF is performed then a temporary file is created on the same directory 
for output. 
When the output file is closed, that file is renamed "s". 
.LI EBs 
An EF is performed then an ER and an EW is performed for the named file. 
When the file is closed, The original file is renamed with "," prepended 
to its filename. 
(See also ED&32 mode control flag.) 
.LI EF 
Closes the current output file by deleting any file with the same name 
and then renaming the temporary file. 
.LI EC 
Moves the contents of the text buffer and the remainder of the current input 
file to the current output file then closes both files (see EF). 
.LI EX 
Same as EC, but exits TECO when finished. 
.LI EK 
Purges the current temporary output file leaving any file of the same name 
intact. 
Also undoes the EB command. 
.LI EQ 
Performs an EK then leaves TECO. 
.LI EIs 
Specifies an indirect command file. 
All further input will come from this file until end of file is reached 
or an error occurs. 
Immediate mode commands are not treated as such with the exception of "$$", 
which will start execution. 
.LI EI$ 
Stops execution of an indirect command file by forcing end of file. 
.LI E!$ 
Invokes the UNIX shell. 
Return with "^D". 
.LI E!s 
Invokes the UNIX shell for the command "s". 
.LE 
.P 
Colon modified ER, EW, EB, EI, or E! will return numeric value (-1 for 
success, 0 for failure) instead of giving error messages. 
.HU "PAGE MANIPULATION COMMANDS" 
.VL 10 
.LI A 
Append the next page of the input file into the text buffer. 
A page is delimited by a formfeed character. 
The appending action may stop without reading a formfeed (see ED flag). 
.LI Y 
Delete entire text buffer then perform "A". 
.LI nPW 
Write the text buffer to the output file and append a form feed. 
Do this n times. 
.LI m,nPW 
Write the m+1th through nth characters to the output file. 
.LI HPW 
Same as PW, except no formfeed is appended. 
.LI nP 
Writes the contents of the buffer to the output file; 
appends a formfeed if the last page read in (via A, Y, or P) was 
terminated with a formfeed. 
A "Y" command is then performed. 
.LI m,nP 
Same as "m,nPW". 
.LE 
.HU "BUFFER POINTER MANIPULATION COMMANDS" 
.P 
The buffer pointer is always positioned between characters.  
The position before the first character is "0", or "B". 
The position after the last character is "Z". 
The current position is ".". 
.VL 10 
.LI nJ 
Move the pointer to position n. 
An error occurs if the pointer is moved beyond the text. 
.LI nC 
Advance the pointer n characters forward (n may be negative). 
An error occurs if the pointer is moved beyond the text. 
.LI nR 
Move the pointer n characters backwards (n may be negative). 
An error occurs if the pointer is moved beyond the text. 
.LI nL 
Line oriented command to move pointer n lines forward. 
.LE 
.HU "TEXT TYPEOUT COMMANDS" 
.VL 10 
.LI nT 
Line oriented text typeout from current position to specified relative line. 
.LI m,nT 
Character oriented text typeout. 
HT will type entire buffer. 
.LI nV 
Type out n-1 lines on each side of the current line. 
Equivalent to 1-nTnT. 
.LI "n^T" 
Types out the ASCII character whose value is n. 
.LI "^Atext^A" 
Outputs "text" to the terminal. 
.LE 
.P 
See also Q-register commands. 
.HU "TEXT DELETION AND INSERTION COMMANDS" 
.VL 10 
.LI nD 
Delete the first n characters from the current pointer position. 
.LI nK 
Line oriented deletion from the pointer position to the nth linefeed. 
.LI "m,nK" 
Delete character m+1 through character n. 
The pointer moves to position m. 
.LI "m,nD" 
Same as "m,nK". 
.LI "HK" 
Delete the entire text buffer. 
.LI Is 
Insert the string, s, at the current position. 
Pointer is positioned at the end of the string. 
.LI "<tab>s" 
Insert a tab character followed by the string, s. 
.LI "nI$" 
Insert a single character whose ASCII value is n at the current buffer 
position. 
.LI "n\e" 
The ASCII representation of the number n, in the current radix, is inserted 
in the text. 
"n" must be present. 
.LI "FRs" 
Replace the preceding p characters with the string s, where p is the 
length of the most recent successful search string or insertion. 
.LE 
.P 
See also Q-register commands. 
.HU "SEARCH COMMANDS" 
.P  
All search commands may be preceded by ":" to suppress search failure 
errors and return the value -1 on success or 0 on failure. 
If a search in an interation loop is followed by a ";" then the command 
behaves as though it had a leading ":", otherwise if the search fails 
a warning message is given and execution proceeds after the closing 
angle bracket. 
In general, if a search fails, the pointer is unchanged. 
If the search succeeds, the pointer is placed at the end of the matched 
string. 
Searches may be performed in a backwards direction (except those that 
do "P" or "Y" commands). 
.VL 10 
.LI nSs 
Search for the nth occurrence of s in the text buffer starting at the 
current pointer position. 
If the string is not found the the pointer is positioned at the beginning 
of the buffer (exception--see ED flag). 
.LI "m,nSs" 
same as nSs, but m serves as a bound limit. 
The search succeeds if the pointer need not advance more than ABS(m)-1 
places. 
If m=0, then search is unbounded. 
.LI nFBs 
A line oriented bounded search where the search starts at the current 
pointer position and continues up to the nth linefeed. 
.LI "m,nFBs" 
Searchs from the m+1th to the nth character. 
HFBs would be an unbounded search of the entire buffer. 
.LI nNs 
Same as the "nSs" command but will do "P" commands as necessary to find 
the string. 
String may not cross a page boundary. 
.LI "n_s" 
Same as the "nSs" command but will do "Y" commands as necessary to find 
the string. 
String may not cross a page boundary. 
.LI "nFSss" 
Same as "nSsFRs". 
.LI "m,nFSss" 
Same as "m,nSsFRs". 
.LI "nFNss" 
Same as "nNsFRs". 
.LI "nFCss" 
Same as "nFBsFRs". 
.LI "m,nFCss" 
Same as "m,nFBsFRs". 
.LI "::Ss" 
Compare command.  Same as "1,1:Ss" or ".,.FBs". 
.LE 
.HU "SEARCH STRING FUNCTIONS" 
.VL 10 
.LI "^" 
The caret character means that the following character is to be 
used as its control character equivalent. 
May be disabled (see ED flag). 
.LI "^Q" 
The following character is to be used literally rather than as a 
match control character. 
.LI "^EQq" 
The string stored in Q-register q is to be used in this position 
in the search string. 
.LI "^\e" 
Toggle between exact and either case match in the search string. 
.LI "^X" 
Matches any character in this position. 
.LI "^S" 
Matches any non-alphanumeric character in this position. 
.LI "^N" 
Matches any character that doesn't match the following character or 
match control character. 
.LI "^EA" 
Matches any alphabetic character (regardless of case). 
.LI "^EC" 
Matches any radix-50 character. 
.LI "^ED" 
Matches any digit. 
.LI "^EL" 
Matches any linefeed, vertical tab, or formfeed. 
.LI "^ER" 
Matches any alphanumeric character in this position. 
.LI "^ES" 
Matches any non-null string of spaces and/or tabs in this position. 
.LI "^EX" 
same as "^X". 
.LI "^E[...]" 
Matches any single character that is in "...". 
.LE 
.HU "Q-REGISTER LOADING COMMANDS" 
.VL 10 
.LI "^Uqs" 
The string, s, is inserted into Q-register q. 
.LI ":^Uqs" 
The string, s, is appended into Q-register q. 
.LI "n^Uq$" 
The character with ascii code n is inserted in the Q-register. 
.LI "n:^Uq$" 
The character with ASCII code n is appended to Q-register q. 
.LI "nXq" 
Line oriented text insertion from pointer to nth line feed into Q-register. 
.LI "n:Xq" 
Same as "nXq" but appends to Q-register. 
.LI "m,nXq" 
Inserts m+1th through nth characters into Q-register q. 
.LI "m,n:Xq" 
Appends m+1th through nth characters into Q-register q. 
.LI "nUq" 
Store n into numeric Q-register q. 
.LI "m,nUq" 
Same as nUqm. 
.LI "n%q" 
Adds n to the numeric Q-register q. 
Returns updated value. 
.LI "]q" 
Pop from the Q-register push-down list into Q-register q. 
Numeric values are passed through this command. 
.LI ":]q" 
Same as "]q", but returns a value -1 if an item was popped or 0 
if the list was empty (Q-register unchanged). 
.LE 
.HU "Q-REGISTER RETRIEVAL COMMANDS" 
.VL 10 
.LI "Gq" 
Copy the text in Q-register q into the text buffer at the 
current buffer position. 
The pointer is then positioned after the inserted text. 
.LI ":Gq" 
Print the contents of the Q-register on the terminal. 
.LI "Qq" 
Returns the numeric value in the Q-register. 
.LI "nQq" 
Returns the numeric value of the nth character in the Q-register, or 
-1 if n is greater or equal to the number of characters in the Q-register. 
.LI "Mq" 
Execute the contents of text in Q-register as a command. 
M commands may be recursively invoked. 
Arguments are passed through this command to commands in the 
Q-register. 
Likewise, values can be returned. 
.LI "[q" 
Copy the contents of the numeric and text storage areas of the Q-register 
into the Q-register push-down list. 
Numeric values are passed through this command. 
If teco enters command mode, the push-down list is deleted. 
.LE 
.HU "BRANCHING COMMANDS" 
.VL 13 
.LI "n<" 
Marks the start of an iteration loop. 
Must be matched with a ">" later in the command. 
If n<=0 then the iteration loop is skipped. 
If n is absent then loop will iterate indefinitely. 
.LI ">" 
Marks the end of an iteration loop. 
The iteration count is decremented, and control returns to the 
command following the "<" if the remaining count is greater than zero. 
.LI "F>" 
Branch to the end of the current iteration (before the ">") or to the 
end of the current command string if not in an iteration. 
.LI "F<" 
Branch to the beginning of the current iteration or to the beginning 
of the current command string if not in an iteration. 
.LI "F'" 
Branch to the end of the current conditional. 
.LI "F|" 
Branch to the else clause of the current conditional. 
If none found, then branch to end of current conditional. 
.LI "^[$" 
Exit the current macro level or return to command level if not in a 
macro. 
Numeric arguments can be returned. 
.LI "n;" 
Branch out of the current iteration if n>=0. 
.LI "!tag!" 
Labels location for "Os" command. 
Also useful for comments. 
.LI "Os" 
Branch to the first occurrence of the specified label (tag) in the 
current macro level. 
Branching to the left of the current iteration start is not permitted. 
.LI n"Xcom' 
Conditional expression, com, will be executed only if n meets criterion X. 
.LI n"Xcm1|cm2' 
Conditional expression, cm1, will be executed only if n meets criterion X. 
Conditional expression, cm2, will be executed only if n fails criterion X. 
.sp 2 
Conditional's criterions are: 
.VL 5 
.LI A 
ASCII Alphabetical (upper or lower case) 
.LI C 
RADIX 50. 
.LI D 
ASCII digit 
.LI E 
zero 
.LI F 
false or failed (zero) 
.LI G 
Greater than zero 
.LI L 
Less than zero 
.LI N 
Not equal to zero 
.LI R 
ASCII Alphanumeric 
.LI S 
Successful (less than zero) 
.LI T 
True (less than zero) 
.LI U 
Unsuccessful (equal to zero) 
.LE 
.LE 
.HU "NUMERIC QUANTITIES" 
.P 
Note that colon modified searches return values. 
.VL 10 
.LI B 
Zero (beginning of text buffer). 
.LI Z 
Length of text buffer. 
.LI "." 
Current pointer position 
.LI H 
Whole buffer, equivalent to "B,Z" 
.LI nA 
The ASCII code for the character to the right of buffer position .+n. 
"n" must be present. 
.LI Mq 
Macro command may return a value if the last command in the string returns 
a value and is not followed by an ESCAPE. 
.LI :Qq 
The number of characters in the text storage area of Q-register q. 
.LI \e 
The numeric value (in the current radix) of the number to the right of the 
pointer (if any). 
The pointer is moved past the number. 
If there is no number, zero is returned and the pointer is unchanged. 
.LI "^E" 
-1 if the last Y or A type command was terminated with a formfeed. 
Otherwise, 0. 
.LI "^F" 
Process ID. 
.LI "^N" 
-1 if the currently open input file is at end-of-file, otherwise 0. 
.LI "n^Q" 
Line oriented command, equivalent to the number of characters to the 
nth linefeed. 
"n^QC" is equivalent to "nL". 
.LI "^S" 
Minus the number of characters in the last successful search command or 
insertion command. 
.LI "^T" 
Teco pauses and accepts one character of input from the terminal. 
The ASCII code for that character is returned. 
See the ET flag. 
.LI "^Y" 
Equivalent to ".+^S,.". 
.LI "^Z" 
The number of characters in the Q-register storage area. 
.LI "^^x" 
The value of the ASCII code for character "x". 
.LE 
.HU "NUMERIC OPERATORS" 
.VL 10 
.LI "+" 
Monadic--ignored; diadic (infix)--addition. 
.LI "-" 
Monadic--negation; diadic (infix)--subtraction. 
.LI "*" 
Multiplication (infix). 
.LI "/" 
"Unsigned" integer division (infix). 
.LI "&" 
Bitwise logical AND function (infix). 
.LI "#" 
Bitwise logical OR function (infix). 
.LI "^_" 
One"s complement (note--postfix function). 
.LI "(...)" 
Parenthesis have usual meaning of forcing order of execution, otherwise 
expressions are evaluated left to right. 
.LE 
.HU "CONVERSION COMMANDS" 
.VL 10 
.LI n= 
n is printed on the terminal, in signed decimal, followed by a newline. 
.LI n:= 
n is printed on the terminal in signed decimal. 
.LI n== 
n is printed on the terminal, in octal, followed by a newline. 
.LI n:== 
n is printed on the terminal in octal. 
.LI "^O" 
Changes radix on all numeric input to octal. 
Also affects "n\e" command. 
.LI "^D" 
Changes radix on all numeric input to decimal. 
Also affects "n\e" command. 
.LE 
.HU "MODE CONTROL FLAGS" 
.P 
These control various operating characteristics of TECO. 
All flags are initially zero unless otherwise indicated. 
The flags may be manipulated as follows: 
.VL 12 
.LI <flag> 
Returns value of flag. 
.LI n<flag> 
Sets flag to n. 
.LI m,n<flag> 
Sets the flag to its current value ANDed to m and then ORed to  n. 
.LE 
.P 
The flags are: 
.VL 8 
.LI ^X 
Search mode flag. 
If zero then searches match independent of case, otherwise the cases 
must match. 
.LI ED 
Edit level flag, a bit encoded word: 
.VL 8 
.LI ED&1 
Allow caret in search strings. 
.LI ED&2 
Allow all "Y" and "_" commands. 
Otherwise, these commands are not allowed if there is an open output file 
and the text buffer is not empty. 
.LI ED&4 
Limits memory expansions when reading text from a file and no formfeed is 
found. 
Mysterious algorithm. 
.LI ED&16 
Failing searches preserves dot. 
.LI ED&32 
This bit, when set and when the output file is to be closed (EF, EC, or EX 
commands) causes any existing file of the same name to be renamed with "," 
prepended before renaming the temporary output file to the output file name. 
This bit is set automatically upon issuing the EB command and cleared with EF,EC, 
or EK. 
When set, EW and EB commands are disallowed. 
.LI "ED&64" 
This bit, when set and when the output file is to be closed (EF, EC, or EX 
commands) causes files to be copied instead of relinked.   
This will retain any links to the edited file. 
Any input file is closed when the output file is closed if this bit is set. 
.LE 
Initially, ED is set to 5. 
.LI EH 
Help level. 
If EH&3=1 then short error messages are printed, otherwise long error 
messages are printed. 
IF EH&4 is set then the command string in error is printed up to and including 
the character in error. 
.LI EO 
File mode. 
Output files are created using this mode. 
Initially 644 octal. 
.LI ES 
Search verification flag, controls typing of messages after successful 
bottom level (not in iteration loop or macro) searches. 
If zero, nothing happens. 
If -1 then the current line is typed out. 
If greater than zero, then the current line is typed out up to the pointer, 
a character is printed, then the rest of the line is typed. 
That character is the ASCII character coresponding to ES&127 if >31. 
If 0<ES&127<32 then the character is a linefeed (a real one), otherwise 
no character is printed. 
M additional lines to each side can be printed by adding 256*M to the flag. 
.LI ET 
Typeout control flag; a bit encoded word: 
.VL 8 
.LI ET&1 
Image mode typeout, except tabs are converted to spaces if terminal does not 
have hardware tabs. 
.LI ET&2 
Scope mode editing 
.LI ET&4 
DEC mode. 
CR -> CRLF on input. 
On output, CR will not display as "^M". 
.LI ET&8 
No echo of input. 
Not reset upon reaching command level. 
If terminal does local echo (stty -echo) then this bit will be set by 
default. 
.LI ET&16 
Cancel ^O on printout. 
This bit is reset when ^O has been canceled. 
.LI ET&128 
Any errors will cause TECO to terminate. 
Memory messages suppressed. 
This bit is reset upon reaching command level. 
.LI ET&32768 
If set, "interupt" signal will cause this bit to be reset and no other action will 
take place. 
This bit is reset automatically upon reaching command level. 
.LE 
The initial value for ET is 2 (or 10). 
.LI EV 
Edit verify flag. 
If nonzero, then TECO does a "EVV" command just before printing 
the prompting "*". 
.LE 
.TC 
 
