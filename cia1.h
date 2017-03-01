#ifndef CIA1_H
#define CIA1_H

#include "emu.h"

/***
** Below information is from "mapping the C64" 
** at http://www.unusedino.de/ec64/technical/project64/mapping_c64.html
**


56320-56335   $DC00-$DC0F
Complex Interface Adapter (CIA) #1 Registers

Locations 56320-56335 ($DC00-$DC0F) are used to communicate with the
Complex Interface Adapter chip #1 (CIA #1).  This chip is a successor
to the earlier VIA and PIA devices used on the VIC-20 and PET.  This
chip functions the same way as the VIA and PIA:  It allows the 6510
microprocessor to communicate with peripheral input and output
devices.  The specific devices that CIA #1 reads data from and sends
data to are the joystick controllers, the paddle fire buttons, and the
keyboard.

In addition to its two data ports, CIA #1 has two timers, each of
which can count an interval from a millionth of a second to a
fifteenth of a second.  Or the timers can be hooked together to count
much longer intervals.  CIA #1 has an interrupt line which is
connected to the 6510 IRQ line.  These two timers can be used to
generate interrupts at specified intervals (such as the 1/60 second
interrupt used for keyboard scanning, or the more complexly timed
interrupts that drive the tape read and write routines).  As you will
see below, the CIA chip has a host of other features to aid in
Input/Output functions.

Location Range: 56320-56321 ($DC00-$DC01)
CIA #1 Data Ports A and B

These registers are where the actual communication with outside
devices takes place.  Bits of data written to these registers can be
sent to external devices, while bits of data that those devices send
can be read here.

The keyboard is so necessary to the computer's operation that you may
have a hard time thinking of it as a peripheral device.  Nonetheless,
it cannot be directly read by the 6510 microprocessor.  Instead, the
keys are connected in a matrix of eight rows by eight columns to CIA
#1 Ports A and B.  The layout of this matrix is shown below.

WRITE TO PORT A               READ PORT B (56321, $DC01)
56320/$DC00
         Bit 7   Bit 6   Bit 5   Bit 4   Bit 3   Bit 2   Bit 1   Bit 0

Bit 7    STOP    Q       C=      SPACE   2       CTRL    <-      1

Bit 6    /       ^       =       RSHIFT  HOME    ;       *       LIRA

Bit 5    ,       @       :       .       -       L       P       +

Bit 4    N       O       K       M       0       J       I       9

Bit 3    V       U       H       B       8       G       Y       7

Bit 2    X       T       F       C       6       D       R       5

Bit 1    LSHIFT  E       S       Z       4       A       W       3

Bit 0    CRSR DN F5      F3      F1      F7      CRSR RT RETURN  DELETE

As you can see, there are two keys which do not appear in the matrix.
The SHIFT LOCK key is not read as a separate key, but rather is a
mechanical device which holds the left SHIFT key switch in a closed
position.  The RESTORE key is not read like the other keys either.  It
is directly connected to the NMI interrupt line of the 6510
microprocessor, and causes an NMI interrupt to occur whenever it is
pressed (not just when it is pressed with the STOP key).

In order to read the individual keys in the matrix, you must first set
Port A for all outputs (255, $FF), and Port B for all inputs (0),
using the Data Direction Registers.  Note that this is the default
condition.  Next, you must write a 0 in the bit of Data Port A that
corresponds to the column that you wish to read, and a 1 to the bits
that correspond to columns you wish to ignore.  You will then be able
to read Data Port B to see which keys in that column are being pushed.

A 0 in any bit position signifies that the key in the corresponding
row of the selected column is being pressed, while a 1 indicates that
the key is not being pressed.  A value of 255 ($FF) means that no keys
in that column are being pressed.

Fortunately for us all, an interrupt routine causes the keyboard to be
read, and the results are made available to the Operating System
automatically every 1/60 second.  And even when the normal interrupt
routine cannot be used, you can use the Kernal SCNKEY routine at 65439
($FF9F) to read the keyboard.

These same data ports are also used to read the joystick controllers.
Although common sense might lead you to believe that you could read
the joystick that is plugged into the port marked Controller Port 1
from Data Port A, and the second joystick from Data Port B, there is
nothing common about the Commodore 64.  Controller Port 1 is read from
Data Port B, and Controller Port 2 is read from CIA #1 Data Port A.

Joysticks consist of five switches, one each for up, down, right, and
left directions, and another for the fire button.  The switches are
read like the key switches--if the switch is pressed, the
corresponding bit will read 0, and if it is not pressed, the bit will
be set to 1.  From BASIC, you can PEEK the ports and use the AND and
NOT operators to mask the unused bits and inverse the logic for easier
comprehension.  For example, to read the joystick in Controller Port
1, you could use the statement:

S1=NOT PEEK(56321)AND15

The meaning of the possible numbers returned are:

 0 = none pressed
 1 = up
 2 = down
 4 = left
 5 = up left
 6 = down left
 8 = right
 9 = up right
10 = down right

The same technique can be used for joystick 2, by substituting 56320
as the number to PEEK.  By the way, the 3 and 7 aren't listed because
they represent impossible combinations like up-down.

To read the fire buttons, you can PEEK the appropriate port and use
the AND operator to mask all but bit 4:

T1=(PEEK(56321)AND16)/16

The above will return a 0 if the button is pressed, and a 1 if it is
not.  Substitute location 56320 as the location to PEEK for Trigger
Button 2.

Since CIA #1 Data Port B is used for reading the keyboard as well as
joystick 1, some confusion can result.  The routine that checks the
keyboard has no way of telling whether a particular bit was set to 0
by a keypress or one of the joystick switches.  For example, if you
plug the joystick into Controller Port 1 and push the stick to the
right, the routine will interpret this as the 2 key being pressed,
because both set the same bit to 0.  Likewise, when you read the
joystick, it will register as being pushed to the right if the 2 key
is being pressed.

The problem of mistaking the keyboard for the joystick can be solved
by turning off the keyscan momentarily when reading the stick with a
POKE 56333, 127:POKE 56320,255, and restoring it after the read with a
POKE 56333,129.  Sometimes you can use the simpler solution of
clearing the keyboard buffer after reading the joystick, with a POKE
198,0.

The problem of mistaking the joystick for a keypress is much more
difficult--there is no real way to turn off the joystick.  Many
commercially available games just use Controller Port 2 to avoid the
conflict.  So, if you can't beat them, sit back and press your
joystick to the left in order to slow down a program listing (the
keyscan routine thinks that it is the CTRL key).

As if all of the above were not enough, Port A is also used to control
which set of paddles is read by the SID chip, and to read the paddle
fire buttons.  Since there are two paddles per joystick Controller
Port, and only two SID registers for reading paddle positions, there
has to be a method for switching the paddle read from joystick Port 1
to joystick Port 2.

When Bit 7 of Port A is set to 1 and Bit 6 is cleared to 0, the SID
registers will read the paddles on Port 1.  When Bit 7 is set to 0 and
Bit 6 is set to 1, the paddles on Port 2 are read by the SID chip
registers.  Note that this also conflicts with the keyscan routine,
which is constantly writing different values to CIA #1 Data Port A in
order to select the keyboard column to read (most of the time, the
value for the last column is written to this port, which coincides
with the selection of paddles on joystick Port 1).  Therefore, in
order to get an accurate reading, you must turn off the keyscan IRQ
and select which joystick port you want to read.  See POTX at 54297
($D419), which is the SID register where the paddles are read, for the
exact technique.

Although the SID chip is used to read the paddle settings, the fire
buttons are read at CIA #1 Data Ports A and B.  The fire buttons for
the paddles plugged into Controller Port 1 are read at Data Port B
(56321, $DC01), while those for the paddles plugged into Controller
Port 2 are read from Data Port A (56320, $DC00).  The fire buttons are
read at Bit 2 and Bit 3 of each port (the same as the joystick left
and joystick right switches), and as usual, the bit will read 0 if the
corresponding button is pushed, and 1 if it is not.

Although only two of the rout paddle values can be read at any one
time, you can always read all four paddle buttons.  See the game
paddle input description at 54297 ($D419) for the BASIC statements
used to read these buttons.

Finally, Data Port B can also be used as an output by either Timer A
or B.  It is possible to set a mode in which the timers do not cause
an interrupt when they run down (see the descriptions of Control
Registers A and B at 56334-5 ($DC0E-F)).  Instead, they cause the
output on Bit 6 or 7 of Data Port B to change.  Timer A can be set
either to pulse the output of Bit 6 for one machine cycle, or to
toggle that bit from 1 to 0 or 0 to 1.  Timer B can use Bit 7 of this
register for the same purpose.

56320         $DC00          CIAPRA
Data Port Register A

Bit 0:  Select to read keyboard column 0
        Read joystick 2 up direction
Bit 1:  Select to read keyboard column 1
        Read joystick 2 down direction
Bit 2:  Select to read keyboard column 2
        Read joystick 2 left direction
        Read paddle 1 fire button
Bit 3:  Select to read keyboard column 3
        Read joystick 2 right direction
        Read paddle 2 fire button
Bit 4:  Select to read keyboard column 4
        Read joystick 2 fire button
Bit 5:  Select to read keyboard column 5
Bit 6:  Select to read keyboard column 6
        Select to read paddles on Port A or B
Bit 7:  Select to read keyboard column 7
        Select to read paddles on Port A or B

56321         $DC01          CIAPRB
Data Port Register B

Bit 0:  Read keyboard row 0
        Read joystick 1 up direction
Bit 1:  Read keyboard row 1
        Read joystick 1 down direction
Bit 2:  Read keyboard row 2
        Read joystick 1 left direction
        Read paddle 1 fire button
Bit 3:  Read keyboard row 3
        Read joystick 1 right direction
        Read paddle 2 fire button
Bit 4:  Read keyboard row 4
        Read joystick 1 fire button
Bit 5:  Read keyboard row 5
Bit 6:  Read keyboard row 6
        Toggle or pulse data output for Timer A
Bit 7:  Read keyboard row 7
        Toggle or pulse data output for Timer B

Location Range: 56322-56323 ($DC02-$DC03)
CIA #1 Data Direction Registers A and B

These Data Direction Registers control the direction of data flow over
Data Ports A and B.  Each bit controls the direction of the data on
the corresponding bit of the port.  If teh bit of the Direction
Register is set to a 1, the corresponding Data Port bit will be used
for data output.  If the bit is set to a 0, the corresponding Data
Port bit will be used for data input.  For example, Bit 7 of Data
Direction Register A controls Bit 7 of Data Port A, and if that
direction bit is set to 0, Bit 7 of Data Port A will be used for data
input.  If the direction bit is set to 1, however, data Bit 7 on Port
A will be used for data output.

The default setting for Data Direction Register A is 255 (all
outputs), and for Data Direction Register B it is 0 (all inputs).
This corresponds to the setting used when reading the keyboard (the
keyboard column number is written to Data Port A, and the row number
is then read in Data Port B).

56322         $DC02          CIDDRA
Data Direction Register A

Bit 0:  Select Bit 0 of Data Port A for input or output (0=input, 1=output)
Bit 1:  Select Bit 1 of Data Port A for input or output (0=input, 1=output)
Bit 2:  Select Bit 2 of Data Port A for input or output (0=input, 1=output)
Bit 3:  Select Bit 3 of Data Port A for input or output (0=input, 1=output)
Bit 4:  Select Bit 4 of Data Port A for input or output (0=input, 1=output)
Bit 5:  Select Bit 5 of Data Port A for input or output (0=input, 1=output)
Bit 6:  Select Bit 6 of Data Port A for input or output (0=input, 1=output)
Bit 7:  Select Bit 7 of Data Port A for input or output (0=input, 1=output)

56323         $DC03          CIDDRB
Data Direction Register B

Bit 0:  Select Bit 0 of Data Port B for input or output (0=input, 1=output)
Bit 1:  Select Bit 1 of Data Port B for input or output (0=input, 1=output)
Bit 2:  Select Bit 2 of Data Port B for input or output (0=input, 1=output)
Bit 3:  Select Bit 3 of Data Port B for input or output (0=input, 1=output)
Bit 4:  Select Bit 4 of Data Port B for input or output (0=input, 1=output)
Bit 5:  Select Bit 5 of Data Port B for input or output (0=input, 1=output)
Bit 6:  Select Bit 6 of Data Port B for input or output (0=input, 1=output)
Bit 7:  Select Bit 7 of Data Port B for input or output (0=input, 1=output)

Location Range: 56324-56327 ($DC04-$DC07)
Timers A and B Low and High Bytes

These four timer registers (two for each timer) have different
functions depending on whether you are reading from them or writing to
them.  When you read from these registers, you get the present value
of the Timer Counter (which counts down from its initial value to 0).
When you write data to these registers, it is stored in the Timer
Latch, and from there it can be used to load the Timer Counter using
the Force Load bit of Control Register A or B (see 56334-5 ($DC0E-F)
below).

These interval timers can hold a 16-bit number from 0 to 65535, in
normal 6510 low-byte, high-byte format (VALUE=LOW BYTE+256*HIGH BYTE).
Once the Timer Counter is set to an initial value, and the timer is
started, the timer will count down one number every microprocessor
clock cycle.  Since the clock speed of the 64 (using the American NTSC
television standard) is 1,022,730 cycles per second, every count takes
approximately a millionth of a second.  The formula for calculating
the amount of time it will take for the timer to count down from its
latch value to 0 is:

TIME=LATCH VALUE/CLOCK SPEED

where LATCH VALUE is the value written to the low and high timer
registers (LATCH VALUE=TIMER LOW+256*TIMER HIGH), and CLOCK SPEED is
1,022,370 cycles per second for American (NTSC) standard television
monitors, or 985,250 for European (PAL) monitors.

When Timer Counter A or B gets to 0, it will set Bit 0 or 1 in the
Interrupt Control Register at 56333 ($DC0D).  If the timer interrupt
has been enabled (see 56333 ($DC0D)), an IRQ will take place, and the
high bit of the Interrupt Control Register will be set to 1.
Alternately, if the Port B output bit is set, the timer will write
data to Bit 6 or 7 of Port B.  After the timer gets to 0, it will
reload the Timer Latch Value, and either stop or count down again,
depending on whether it is in one-shot or continuous mode (determined
by Bit 3 of the Control Register).

Although usually a timer will be used to count the microprocessor
cycles, Timer A can count either the microprocessor clock cycles or
external pulses on the CTN line, which is connected to pin 4 of the
User Port.

Timer B is even more versatile.  In addition to these two sources,
Timer B can count the number of times that Timer A goes to 0.  By
setting Timer A to count the microprocessor clock, and setting Timer B
to count the number of times that Timer A zeros, you effectively link
the two timers into one 32-bit timer that can count up to 70 minutes
with accuracy within 1/15 second.

In the 64, CIA #1 Timer A is used to generate the interrupt which
drives the routine for reading the keyboard and updating the software
clock.  Both Timers A and B are also used for the timing of the
routines that read and write tape data.  Normally, Timer A is set for
continuous operation, and latched with a value of 149 in the low byte
and 66 in the high byte, for a total Latch Value of 17045.  This means
that it is set to count to 0 every 17045/1022730 seconds, or
approximately 1/60 second.

For tape reads and writes, the tape routines take over the IRQ
vectors.  Even though the tape write routines use the on-chip I/O port
at location 1 for the actual data output to the cassette, reading and
writing to the cassette uses both CIA #1 Timer A and Timer B for
timing the I/O routines.

56324         $DC04          TIMALO
Timer A (low byte)

56325         $DC05          TIMAHI
Timer A (high byte)

56326         $DC06          TIMBLO
Timer B (low byte)

56327         $DC07          TIMBHI
Timer B (high byte)

Location Range: 56328-56331 ($DC08-$DC0B)
Time of Day Clock (TOD)

In addition to the two general-purpose timers, the 6526 CIA chip has a
special-purpose Time of Day Clock, which keeps time in a format that
humans can understand a little more easily than microseconds.

This Time of Day Clock even has an alarm, which can cause an interrupt
at a specific time.  It is organized in four registers, one each for
hours, minutes, seconds, and tenths of seconds.  Each register reads
out in Binary Coded Decimal (BCD) format, for easier conversion to
ASCII digits.  A BCD byte is divided into two nybbles, each of which
represents a single digit in base 10.  Even though a four-bit nybble
can hold a number from 0 to 15, only the base 10 digits of 0-9 are
used.  Therefore, 10 0'clock would be represented by a byte in the
hours register with the nybbles 0001 and 0000, which stand for the
digits 1 and 0.  The binary value of this byte would be 16 (16 times
the high nybble plus the low nybble).  Each of the other registers
operates in the same manner.  In addition, Bit 7 of the hours register
is used as an AM/PM flag.  If that bit is set to 1, it indicates PM,
and if it is set to 0, the time is AM.

The Time of Day Clock Registers can be used for two purposes,
depending on whether you are reading them or writing to them.  If you
are reading them, you will always be reading the time.  There is a
latching feature associated with reading the hours register in order
to solve the problem of the time changing while you are reading the
registers.  For example, if you were reading the hours register just
as the time was changing from 10:59 to 11:00, it is possible that you
would read the 10 in the hours register, and by the time you read the
minutes register it would have changed from 59 to 00.  Therefore, you
would read 10:00 instead of either 10:59 or 11:00.

To prevent this kind of mistake, the Time of Day Clock Registers stop
updating as soon as you read the hours register, and do not start
again until you read the tenths of seconds register.  Of course, the
clock continues to keep time internally even though it does not update
the registers.  If you want to read only minutes, or seconds or tenths
of seconds, there is no problem, and no latching will occur.  But
anytime you read hours, you must follow it by reading tenths of
seconds, even if you don't care about them, or else the registers will
not continue to update.

Writing to these registers either sets the time or the alarm,
depending on the setting of Bit 7 of Control Register B (56335,
$DC0F).  If that bit is set to 1, writing to the Time of Day registers
sets the alarm.  If the bit is set to 0, writing to the Time of Day
registers sets the Time of Day clock.  In either case, as with reading
the registers, there is a latch function.  This function stops the
clock from updating when you write to the hours register.  The clock
will not start again until you write to the tenths of seconds
registers.

The only apparent use of the Time of Day Clock by the 64's Operating
System is in the BASIC RND statement.  There, the seconds and tenths
of seconds registers are read and their values used as part of the
seed value for the RND(0) command.

Nonetheless, this clock can be an invaluable resource for the 64 user.
It will keep time more accurately than the software clock maintained
at locations 60-162 ($A0-$A2) by the Timer A interrupt routine.  And
unlike that software clock, the Time of Day Clock will not be
disturbed when I/O operations disrupt the Timer A IRQ, or when the IRQ
vector is diverted elsewhere.  Not even a cold start RESET will
disrupt the time.  For game timers, just set the time for 00:00:00:0
and it will keep track of elapsed time in hours, minutes, seconds and
tenths of seconds format.

The following digital clock program, written in BASIC, will
demonstrate the use of these timers:

10 PRINT CHR$(147):GOSUB 200
20 H=PEEK(56331):POKE 1238,(H AND 16)/16+48:POKE 1239,(H AND 15)+48
30 M=PEEK(56330):POKE 1241,(M AND 240)/16+48:POKE 1242,(M AND 15)+48
40 S=PEEK(56329):POKE 1244,(S AND 240)/16+48:POKE 1245,(S AND 15)+48
50 T=PEEK(56328)AND15:POKE 1247,T+48:GOTO 20
200 INPUT"WHAT IS THE HOUR";H$:IF H$="" THEN 200
210 H=0:IF LEN(H$)>1 THEN H=16
220 HH=VAL(RIGHT$(H$,1)):H=H+HH:POKE56331,H
230 INPUT "WHAT IS THE MINUTE";M$:IF M$=""THEN 200
240 M=0:IF LEN(M$)>1 THEN M=16*VAL(LEFT$(M$,1))
250 MM=VAL(RIGHT$(M$,1)):M=M+MM:POKE56330,M
260 INPUT "WHAT IS THE SECOND";S$:IF S$=""THEN 200
270 S=0:IF LEN(S$)>1 THEN S=16*VAL(LEFT$(S$,1))
280 SS=VAL(RIGHT$(S$,1)):S=S+SS:POKE56329,S:POKE56328,0
290 POKE 53281,1:PRINT CHR$(147):POKE 53281,6
300 POKE 1240,58:POKE 1243,58:POKE 1246,58:GOTO 20

56328         $DC08          TODTEN
Time of Day Clock Tenths of Seconds

Bits 0-3:  Time of Day tenths of second digit (BCD)
Bits 4-7:  Unused

56329         $DC09          TODSEC
Time of Day Clock Seconds

Bits 0-3:  Second digit of Time of Day seconds (BCD)
Bits 4-6:  First digit of Time of Day seconds (BCD)
Bit 7:  Unused

56330         $DC0A          TODMIN
Time of Day Clock Minutes

Bits 0-3:  Second digit of Time of Day minutes (BCD)
Bits 4-6:  First digit of Time of Day minutes (BCD)
Bit 7:  Unused

56331         $DC0B          TODHRS
Time of Day Clock Hours

Bits 0-3:  Second digit of Time of Day hours (BCD)
Bit 4:  First digit of Time of Day hours (BCD)
Bits 5-6:  Unused
Bit 7:  AM/PM Flag (1=PM, 0=AM)

56332         $DC0C          CIASDR
Serial Data Port

The CIA chip has an on-chip serial port, which allows you to send or
receive a byte of data one bit at a time, with the most significant
bit (Bit 7) being transferred first.  Control Register A at 56334
($DC0E) allows you to choose input or output modes.  In input mode, a
bit of data is read from the SP line (pin 5 of the User Port) whenever
a signal on the CNT line (pin 4) appears to let you know that it is
time for a read.  After eight bits are received this way, the data is
placed in the Serial Port Register, and an interrupt is generated to
let you know that the register should be read.

In output mode, you write data to the Serial Port Register, and it is
sent out over the SP line (pin 5 of the User Port), using Timer A for
the baud rate generator.  Whenever a byte of data is written to this
register, transmission will start as long as Timer A is running and in
continuous mode.  Data is sent at half the Timer A rage, and an output
will appear on the CNT line (pin 4 of the User Port) whenever a bit is
sent.  After all eight bits have been sent, an interrupt is generated
to indicate that it is time to load the next byte to send into the
Serial Register.

The Serial Data Register is not used by the 64, which does all of its
serial I/O through the regular data ports.

56333         $DC0D          CIAICR
Interrupt Control Register

Bit 0:  Read / did Timer A count down to 0?  (1=yes)
        Write/ enable or disable Timer A interrupt (1=enable, 0=disable)
Bit 1:  Read / did Timer B count down to 0?  (1=yes)
        Write/ enable or disable Timer B interrupt (1=enable, 0=disable)
Bit 2:  Read / did Time of Day Clock reach the alarm time?  (1=yes)
        Write/ enable or disable TOD clock alarm interrupt (1=enable,
        0=disable)
Bit 3:  Read / did the serial shift register finish a byte? (1=yes)
        Write/ enable or disable serial shift register interrupt (1=enable,
        0=disable)
Bit 4:  Read / was a signal sent on the flag line?  (1=yes)
        Write/ enable or disable FLAG line interrupt (1=enable, 0=disable)
Bit 5:  Not used
Bit 6:  Not used
Bit 7:  Read / did any CIA #1 source cause an interrupt?  (1=yes)
        Write/ set or clear bits of this register (1=bits written with 1 will
        be set, 0=bits written with 1 will be cleared)

This register is used to control the five interrupt sources on the
6526 CIA chip.  These sources are Timer A, Timer B, the Time of Day
Clock, the Serial Register, and the FLAG line.  Timers A and B cause
an interrupt when they count down to 0.  The Time of Day Clock
generates an interrupt when it reaches the ALARM time.  The Serial
Shift Register interrupts when it compiles eight bits of input or
output.  An external signal pulling the CIA hardware line called FLAG
low will also cause an interrupt (on CIA #1, this FLAG line is
connected to the Cassette Read line of the Cassette Port).

Even if the condition for a particular interrupt is satisfied, the
interrupt must still be enabled for an IRQ actually to occur.  This is
done by writing to the Interrupt Control Register.  What happens when
you write to this register depends on the way that you set Bit 7.  If
you set it to 0, any other bit that was written to with a 1 will be
cleared, and the corresponding interrupt will be disabled.  If you set
Bit 7 to 1, any bit written to with a 1 will be set, and the
corresponding interrupt will be enabled.  In either case, the
interrupt enable flags for those bits written to with a 0 will not be
affected.

For example, in order to disable all interrupts from BASIC, you could
POKE 56333, 127.  This sets Bit 7 to 0, which clears all of the other
bits, since they are all written with 1's.  Don't try this from BASIC
immediate mode, as it will turn off Timer A which causes the IRQ for
reading the keyboard, so that it will in effect turn off the keyboard.

To turn on the Timer A interrupt, a program could POKE 56333,129.  Bit
7 is set to 1 and so is Bit 0, so the interrupt which corresponds to
Bit 0 (Timer A) is enabled.

When you read this register, you can tell if any of the conditions for
a CIA Interrupt were satisfied because the corresponding bit will be
set to a 1.  For example, if Timer A counts down to 0, Bit 0 of this
register will be set to 1.  If, in addition, the mask bit that
corresponds to that interrupt source is set to 1, and an interrupt
occurs, Bit 7 will also be set.  This allows a multi-interrupt system
to read one bit and see if the source of a particular interrupt was
CIA #1.  You should note, however, that reading this register clears
it, so you should preserve its contents in RAM if you want to test
more than one bit.

56334         $DC0E          CIACRA
Control Register A

Bit 0:  Start Timer A (1=start, 0=stop)
Bit 1:  Select Timer A output on Port B (1=Timer A output appears on Bit 6 of
        Port B)
Bit 2:  Port B output mode (1=toggle Bit 6, 0=pulse Bit 6 for one cycle)
Bit 3:  Timer A run mode (1=one-shot, 0=continuous)
Bit 4:  Force latched value to be loaded to Timer A counter (1=force load
        strobe)
Bit 5:  Timer A input mode (1=count microprocessor cycles, 0=count signals on
        CNT line at pin 4 of User Port)
Bit 6:  Serial Port (56332, $DC0C) mode (1=output, 0=input)
Bit 7:  Time of Day Clock frequency (1=50 Hz required on TOD pin, 0=60 Hz)

Bits 0-3.  This nybble controls Timer A.  Bit 0 is set to 1 to start
the timer counting down, and set to 0 to stop it.  Bit 3 sets the
timer for one-shot or continuous mode.

In one-shot mode, the timer counts down to 0, sets the counter value
back to the latch value, and then sets Bit 0 back to 0 to stop the
timer.  In continuous mode, it reloads the latch value and starts all
over again.

Bits 1 and 2 allow you to send a signal on Bit 6 of Data Port B when
the timer counts.  Setting Bit 1 to 1 forces this output (which
overrides the Data Direction Register B Bit 6, and the normal Data
Port B value).  Bit 2 allows you to choose the form this output to Bit
6 of Data Port B will take.  Setting Bit 2 to a value of 1 will cause
Bit 6 to toggle to the opposite value when the timer runs down (a
value of 1 will change to 0, and a value of 0 will change to 1).
Setting Bit 2 to a value of 0 will cause a single pulse of a one
machine-cycle duration (about a millionth of a second) to occur.

Bit 4.  This bit is used to load the Timer A counter with the value
that was previously written to the Timer Low and High Byte Registers.
Writing a 1 to this bit will force the load (although there is no data
stored here, and the bit has no significance on a read).

Bit 5.  Bit 5 is used to control just what it is Timer A is counting.
If this bit is set to 1, it counts the microprocessor machine cycles
(which occur at the rate of 1,022,730 cycles per second).  If the bit
is set to 0, the timer counts pulses on the CNT line, which is
connected to pin 4 of the User Port.  This allows you to use the CIA
as a frequency counter or an event counter, or to measure pulse width
or delay times of external signals.

Bit 6.  Whether the Serial Port Register is currently inputting or
outputting data (see the entry for that register at 56332 ($DC0C) for
more information) is controlled by this bit.

Bit 7.  This bit allows you to select from software whether the Time
of Day Clock will use a 50 Hz or 60 Hz signal on the TOD pin in order
to keep accurate time (the 64 uses a 60 Hz signal on that pin).

56335         $DC0F          CIACRB
Control Register B

Bit 0:  Start Timer B (1=start, 0=stop)
Bit 1:  Select Timer B output on Port B (1=Timer B output appears on
        Bit 7 of Port B)
Bit 2:  Port B output mode (1=toggle Bit 7, 0=pulse Bit 7 for one
        cycle)
Bit 3:  Timer B run mode (1=one-shot, 0=continuous)
Bit 4:  Force latched value to be loaded to Timer B counter (1=force
        load strobe)
Bits 5-6:  Timer B input mode
           00 = Timer B counts microprocessor cycles
           01 = Count signals on CNT line at pin 4 of User Port
           10 = Count each time that Timer A counts down to 0
           11 = Count Timer A 0's when CNT pulses are also present
Bit 7:  Select Time of Day write (0=writing to TOD registers sets
        alarm, 1=writing to TOD registers sets clock)

Bits 0-3.  This nybble performs the same functions for Timer B that
Bits 0-3 of Control Register A perform for Timer A, except that Timer
B output on Data Port B appears at Bit 7, and not Bit 6.

Bits 5 and 6.  These two bits are used to select what Timer B counts.
If both bits are set to 0, Timer B counts the microprocessor machine
cycles (which occur at the rate of 1,022,730 cycles per second).  If
Bit 6 is set to 0 and Bit 5 is set to 1, Timer B counts pulses on the
CNT line, which is connected to pin 4 of the User Port.  If Bit 6 is
set to 1 and Bit 5 is set to 0, Timer B counts Timer A underflow
pulses, which is to say that it counts the number of times that Timer
A counts down to 0.  This is used to link the two numbers into one
32-bit timer that can count up to 70 minutes with accuracy to within
1/15 second.  Finally, if both bits are set to 1, Timer B counts the
number of times that Timer A counts down to 0 and there is a signal on
the CNT line (pin 4 of the User Port).

Bit 7.  Bit 7 controls what happens when you write to the Time of Day
registers.  If this bit is set to 1, writing to the TOD registers sets
the ALARM time.  If this bit is cleared to 0, writing to the TOD
registers sets the TOD clock.

Location Range: 56336-56575 ($DC10-$DCFF)
CIA #1 Register Images

Since the CIA chip requires only enough addressing lines to handle 16
registers, none of the higher bits are decoded when addressing the
256-byte area that has been assigned to it.  The result is that every
16-byte area in this 256-byte block is a mirror of every other.  Even
so, for the sake of clarity in your programs it is advisable to use
the base address of the chip, and not use the higher addresses to
communicate with the chip.

*/

/*
  	CIA 1 register definitions 

*/ 


typedef enum {

CIA1_PRA 				=	0x00,  // data port a register
CIA1_PRB 				=	0x01,  // data port b register
CIA1_DDRA 				=	0x02,  // porta data direction register
CIA1_DDRB 				=	0x03,  // portb data direction register
CIA1_TALO				=	0x04,  // Timer A Low Byte 
CIA1_TAHI				=	0x05,  // Timer A High Byte 
CIA1_TBLO				=	0x06,  // Timer A Low Byte 
CIA1_TBHI				=	0x07,  // Timer A High Byte 
CIA1_TODTENTHS			=	0x08,  // BCD Time of Day Tenths of Second
CIA1_TODSECS			=	0x09,  // BCD TOD Seconds 
CIA1_TODMINS			=	0x0A,  // BCD TOD Minutes
CIA1_TODHRS				=	0x0B,  // BCD TOD Hours
CIA1_SDR 				=	0x0C,  // Serial Shift Register 
CIA1_ICR				=	0x0D,  // Interupt control and status 
CIA1_CRA				=	0x0E,  // Timer A control register 
CIA1_CRB 				=	0x0F  // Timer B control register 

} CIA1_REGISTERS;

#define CIA_FLAG_TAUIRQ					0b00000001 		// Timer a irq 
#define CIA_FLAG_TBUIRQ					0b00000010 		// Timer b irq 
#define CIA_FLAG_TODIRQ					0b00000100 		// TOD irq	
#define CIA_FLAG_SHRIRQ					0b00001000 		// shift register irq 
#define CIA_FLAG_FLGIRQ					0b00010000 		// flag irq 
#define CIA_FLAG_UN1IRQ					0b00100000 		// unused three
#define CIA_FLAG_UN2IRQ  				0b01000000 	 	// unused two 
#define CIA_FLAG_FILIRQ					0b10000000    	// Fill bit
#define CIA_FLAG_CIAIRQ					0b10000000    	// set when any cia irq is fired.


#define CIA_CRA_TIMERSTART  			0b00000001   // 1 running, 0 stopped.
#define CIA_CRA_PORTBSELECT				0b00000010   // 1 send to bit 6 of port b 
#define CIA_CRA_PORTBMODE				0b00000100   // 1 toggle bit 6, 0 pule bit 6 1 cycle.
#define CIA_CRA_TIMERRUNMODE			0b00001000 	 // 1 one shot timer, 0 continuous timer. 
#define CIA_CRA_FORCELATCH				0b00010000 	 // force latch into current
#define CIA_CRA_TIMERINPUT				0b00100000 	 // 0 count microprocessor cycles, 1 count CNT press
#define CIA_CRA_SERIALMODE				0b01000000	 // serial mode (1 output, 0 input)
#define CIA_CRA_TODFREQUENCY			0b10000000   // 1 50hz TOD, 0 60hz TOD 

#define ROW_0   0x01
#define ROW_1   0x02
#define ROW_2   0x04
#define ROW_3   0x08
#define ROW_4   0x10
#define ROW_5   0x20
#define ROW_6   0x40
#define ROW_7   0x80


//
// BUGBUG dummy values for c64 keys
//
#define C64KEY_LSHIFT 	0xD0
#define C64KEY_CTRL  	0xD1
#define C64KEY_RUNSTOP	0xD2
#define C64KEY_CURDOWN	0xD3
#define C64KEY_CURLEFT	0xD4
#define C64KEY_CURRIGHT	0xD5
#define C64KEY_CURUP	0xD6
#define C64KEY_F1		0xD6
#define C64KEY_F3		0xD7
#define C64KEY_F5		0xD8
#define C64KEY_F7		0xD9
#define C64KEY_C64		0xDA
#define C64KEY_RSHIFT	0xDB
#define C64KEY_HOME		0xDC
#define C64KEY_BACK		0xDD
#define C64KEY_POUND	0xDD
#define C64KEY_DELETE   0xDE
#define C64KEY_RESTORE  0xDF





void cia1_update();
void cia1_keyup(byte ch);
void cia1_keydown(byte ch);

void cia1_init();
void cia1_destroy(); 
byte cia1_peek(byte register);
void cia1_poke(byte register,byte val);


#endif



