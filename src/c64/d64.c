/*
Conundrum 64: Commodore 64 Emulator

MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

-------------------------------------------------------------------------------
MODULE: d64.c
D64 file format...


WORK ITEMS:

KNOWN BUGS:

*/

#include "emu.h"
#include "cpu.h"


#define D64_BYTES_PER_SECTOR 256


typedef struct {
	byte data[D64_BYTES_PER_SECTOR];
} D64_SECTOR;



word d64_track_to_sector(byte track)  {

	int i = 1;
	int j = 0;

	while (i < track) {
		j+= 17;
		if (i < 31) j++;
		if (i < 25) j++;
		if (i < 18) j+= 2;
		i++;
	}

	return j;
}

/*
  Track #Sect #SectorsIn D64 Offset   Track #Sect #SectorsIn D64 Offset
  ----- ----- ---------- ----------   ----- ----- ---------- ----------
    1     21       0       $00000      21     19     414       $19E00
    2     21      21       $01500      22     19     433       $1B100
    3     21      42       $02A00      23     19     452       $1C400
    4     21      63       $03F00      24     19     471       $1D700
    5     21      84       $05400      25     18     490       $1EA00
    6     21     105       $06900      26     18     508       $1FC00
    7     21     126       $07E00      27     18     526       $20E00
    8     21     147       $09300      28     18     544       $22000
    9     21     168       $0A800      29     18     562       $23200
   10     21     189       $0BD00      30     18     580       $24400
   11     21     210       $0D200      31     17     598       $25600
   12     21     231       $0E700      32     17     615       $26700
   13     21     252       $0FC00      33     17     632       $27800
   14     21     273       $11100      34     17     649       $28900
   15     21     294       $12600      35     17     666       $29A00
   16     21     315       $13B00      36(*)  17     683       $2AB00
   17     21     336       $15000      37(*)  17     700       $2BC00
   18     19     357       $16500      38(*)  17     717       $2CD00
   19     19     376       $17800      39(*)  17     734       $2DE00
   20     19     395       $18B00      40(*)  17     751       $2EF00
*/






