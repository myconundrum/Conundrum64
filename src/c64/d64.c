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
#include "d64.h"
#include "string.h"


#define D64_BYTES_PER_SECTOR 256


typedef struct {

	byte nTrack;
	byte nSector;
	byte data[D64_BYTES_PER_SECTOR-2];

} D64_SECTOR;


typedef struct {

	byte freeSectors;
	byte bitmap1;
	byte bitmap2;
	byte bitmap3;

} D64_BAM_ENTRY;

typedef struct {

	byte dTrack;				// location to first directory entry track. ignore. always use 18
	byte dSector; 				// location to first directory entry sector. ignore. always use 1
	byte dosVersion;        	// should be 'A', 0x41
	byte unused1;
	D64_BAM_ENTRY entries[35]; 	// BAM entries per track
	byte diskName[16];			// Disk name padded with 0xA0
	byte A01; 					// Filled with 0xA0
	byte A02;   				// Filled with 0xA0
	byte diskId[2];  			// DiskId;
	byte A03;  					// Usually filled with 0xA0
	byte dosType[2]; 			// Usually "2A"
	byte A04;
	byte A05;
	byte A06;
	byte A07;
	byte unused2[84]; 			// unused

} D64_BAM;





#define D64_FILETYPE_DEL 0x00
#define D64_FILETYPE_SEQ 0x01
#define D64_FILETYPE_PRG 0x02
#define D64_FILETYPE_USR 0x03
#define D64_FILETYPE_REL 0x04
#define D64_FILE_LOCKED  0x40
#define D64_FILE_CLOSED  0x80

typedef struct {

	byte nTrack; 				// next directory entry track
	byte nSector; 				// next directory entry sector
	byte fType;					// file type.
	byte fTrack;  				// file first track
	byte fSector; 				// file first sector
	byte fName[16]; 			// file name
	byte rTrack;				// relative side first track
	byte rSector;  				// relative side first sector
	byte rFileLength; 			// relative file length (max 254)
	byte unused[6]; 			// used with GEOS file system
	byte lFileSize;				// low byte of file size (in sectors)
	byte hFileSize;   			// high byte of file size (in sectors)

} D64_DIRECTORY_ENTRY;


#define D64_MAX_DIRECTORY_ENTRIES 144

typedef struct {

	D64_DIRECTORY_ENTRY entries[D64_MAX_DIRECTORY_ENTRIES];
	byte used; 

} D64_DIRECTORY;



typedef struct {

	FILE * 				file;  
	const char * 		path;

	D64_BAM 			bam; 
	D64_DIRECTORY 		dir; 

} D64_DATA;





D64_DATA g_d64 = {0};


/*
  Bytes: $00-1F: First directory entry
          00-01: Track/Sector location of next directory sector ($00 $00 if
                 not the first entry in the sector)
             02: File type.
                 Typical values for this location are:
                   $00 - Scratched (deleted file entry)
                    80 - DEL
                    81 - SEQ
                    82 - PRG
                    83 - USR
                    84 - REL
                 Bit 0-3: The actual filetype
                          000 (0) - DEL
                          001 (1) - SEQ
                          010 (2) - PRG
                          011 (3) - USR
                          100 (4) - REL
                          Values 5-15 are illegal, but if used will produce
                          very strange results. The 1541 is inconsistent in
                          how it treats these bits. Some routines use all 4
                          bits, others ignore bit 3,  resulting  in  values
                          from 0-7.
                 Bit   4: Not used
                 Bit   5: Used only during SAVE-@ replacement
                 Bit   6: Locked flag (Set produces ">" locked files)
                 Bit   7: Closed flag  (Not  set  produces  "*", or "splat"
                          files)
          03-04: Track/sector location of first sector of file
          05-14: 16 character filename (in PETASCII, padded with $A0)
          15-16: Track/Sector location of first side-sector block (REL file
                 only)
             17: REL file record length (REL file only, max. value 254)
          18-1D: Unused (except with GEOS disks)
          1E-1F: File size in sectors, low/high byte  order  ($1E+$1F*256).
                 The approx. filesize in bytes is <= #sectors * 254
          20-3F: Second dir entry. From now on the first two bytes of  each
                 entry in this sector  should  be  $00  $00,  as  they  are
                 unused.
          40-5F: Third dir entry
          60-7F: Fourth dir entry
          80-9F: Fifth dir entry
          A0-BF: Sixth dir entry
          C0-DF: Seventh dir entry
          E0-FF: Eighth dir entry

*/


void d64_track_to_sector(byte track, word * sector)  {

	int i = 1;

	(*sector) = 0;

	while (i < track) {
		(*sector) += 17;
		if (i < 31) (*sector)++;
		if (i < 25) (*sector)++;
		if (i < 18) (*sector)+= 2;
		i++;
	}
}

void d64_sector_to_track(word sector, byte * track, word * remainder)  {

	(*track) = 1;
	(*remainder) = sector;
	while (1) {
		if ((*track) < 18) {
			if ((*remainder) < 21) return;
			(*remainder) -= 21;
			(*track)++;
		} else if ((*track) < 25) {
			if ((*remainder) < 19) return;
			(*remainder) -= 19;
			(*track)++;
		} else if ((*track) < 31) {
			if ((*remainder) < 18) return;
			(*remainder) -= 18;
			(*track)++;
		} else  {
			if ((*remainder) < 17) return;
			(*remainder) -= 17;
			(*track)++;
		}
	}
}





void d64_directory(FILE * file) {

	word s;
	D64_DIRECTORY_ENTRY d[8];
	bool done = false;


	d64_track_to_sector(18,&s);
	s++;
	fseek(file, s * 256, SEEK_SET);
	fread(d,sizeof(D64_DIRECTORY_ENTRY),8,file); // read directory sector.

	while (!done) {

		done = d[0].nTrack == 0;

		for (int i = 0; i < 8; i++) {
			DEBUG_PRINT("Vdrive directory entry\n");
			DEBUG_PRINT("%-40s [0x%02X]\n","\tFile Type:",d[i].fType);
			DEBUG_PRINT("%-40s [%s]\n","\tFile Name:",d[i].fName);
		}
		
		d64_track_to_sector(d[0].nTrack,&s);
		s += d[0].nSector; 

		fseek(file, s * 256, SEEK_SET);
		fread(d,sizeof(D64_DIRECTORY_ENTRY),8,file);
	}
}



void d64_seek(byte track, byte sector) {

	word s;
	
	d64_track_to_sector(track,&s);
	s+=sector;

	fseek(g_d64.file,s*256,SEEK_SET);
}

void d64_read(void * ptr, size_t size, byte track, byte sector) {

	d64_seek(track,sector);
	fread(ptr, size, 1, g_d64.file);
}


bool d64_match_string(const char * str1, const char * str2) {


	if (strlen(str1) != strlen(str2)) {
		return false;
	}

	for (int i = 0; i < strlen(str1); i++) {
		if (toupper(str1[i]) != toupper(str2[i])) {
			return false;
		}
	}
	return true;
}

D64_DIRECTORY_ENTRY * d64_directory_entry_by_name(const char * name) {

	for (int i = 0; i < g_d64.dir.used; i++) {
		if (d64_match_string(name,(char *) g_d64.dir.entries[i].fName)) {
			return &g_d64.dir.entries[i];
		}
	}

	return NULL;
}

byte d64_count_file_sectors(D64_DIRECTORY_ENTRY *e) {

	byte c = 0;
	bool done = false;
	
	D64_SECTOR s; 
	s.nTrack = e->fTrack;
	s.nSector = e->fSector;

	while (!done) {
		d64_read(&s,sizeof(D64_SECTOR),s.nTrack,s.nSector);
		done = s.nTrack == 0;
		c++; 
	}

	return c;

}


void d64_close_file(D64_FILE *f) {
	free(f->data);
}

bool d64_open_file(D64_FILE * file, const char *name) {

	D64_DIRECTORY_ENTRY * e = d64_directory_entry_by_name(name);
	word cur = 0; 
	bool done = false; 
	D64_SECTOR s; 

	if (!e) {return false;}



	file->size = d64_count_file_sectors(e) * 256;
	DEBUG_PRINT("opening file with size %d.\n",file->size);
	fflush(g_debug);
	file->data = (byte *) malloc(file->size * sizeof(byte));
	
	if (!file->data) {return false;}

	s.nTrack = e->fTrack;
	s.nSector = e->fSector;

	while (!done) {

		d64_read(&s,sizeof(D64_SECTOR),s.nTrack,s.nSector);
		done = s.nTrack == 0;
		for (int i = 0; i < D64_BYTES_PER_SECTOR - 2; i++) {
			file->data[cur++] = s.data[i];
		
		}
	}

	
	return true;
}


void d64_read_bam() {d64_read(&g_d64.bam,sizeof(D64_BAM),18,0);}

void d64_read_directory() {

	bool done = false;
	D64_DIRECTORY_ENTRY d[8];
	
	g_d64.dir.used = 0;
	d[0].nTrack = 18;
	d[0].nSector = 1;

	while (!done) {

		d64_read(&d,sizeof(D64_DIRECTORY_ENTRY)*8,d[0].nTrack,d[0].nSector);
		done = d[0].nTrack == 0;
		
		for (int i = 0; i < 8;i++) {
			g_d64.dir.entries[g_d64.dir.used++] = d[i];
		}
	}
}

void d64_eject_disk() {

	if (g_d64.file != NULL) {

		fclose (g_d64.file);
		g_d64.file = NULL;
	}
}

void d64_insert_disk(const char * path) {

	d64_eject_disk();
	g_d64.file = fopen(path,"rb");
	g_d64.path = path;
	if (g_d64.file == NULL) {
		DEBUG_PRINT("D64: Failed to open %s\n",path);
	}
	else {
		DEBUG_PRINT("D64: disk %s inserted.\n",path);
		d64_read_bam();
		d64_read_directory();

	}
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


 Bytes:$00-01: Track/Sector location of the first directory sector (should
                be set to 18/1 but it doesn't matter, and don't trust  what
                is there, always go to 18/1 for first directory entry)
            02: Disk DOS version type (see note below)
                  $41 ("A")
            03: Unused
         04-8F: BAM entries for each track, in groups  of  four  bytes  per
                track, starting on track 1 (see below for more details)
         90-9F: Disk Name (padded with $A0)
         A0-A1: Filled with $A0
         A2-A3: Disk ID
            A4: Usually $A0
         A5-A6: DOS type, usually "2A"
         A7-AA: Filled with $A0
         AB-FF: Normally unused ($00), except for 40 track extended format,
                see the following two entries:
         AC-BF: DOLPHIN DOS track 36-40 BAM entries (only for 40 track)
         C0-D3: SPEED DOS track 36-40 BAM entries (only for 40 track)

Note: The BAM entries for SPEED, DOLPHIN and  ProLogic  DOS  use  the  same
      layout as standard BAM entries.

  One of the interesting things from the BAM sector is the byte  at  offset
$02, the DOS version byte. If it is set to anything other than $41 or  $00,
then we have what is called "soft write protection". Any attempt  to  write
to the disk will return the "DOS Version" error code 73  ,"CBM  DOS  V  2.6
1541". The 1541 is simply telling  you  that  it  thinks  the  disk  format
version is incorrect. This message will normally come  up  when  you  first
turn on the 1541 and read the error channel. If you write a $00  or  a  $41
into 1541 memory location $00FF (for device 0),  then  you  can  circumvent
this type of write-protection, and change the DOS version back to  what  it
should be.

  The BAM entries require a bit (no pun intended) more of a breakdown. Take
the first entry at bytes $04-$07 ($12 $FF $F9 $17). The first byte ($12) is
the number of free sectors on that track. Since we are looking at the track
1 entry, this means it has 18 (decimal) free sectors. The next three  bytes
represent the bitmap of which sectors are used/free. Since it is 3 bytes (8
bits/byte) we have 24 bits of storage. Remember that at  most,  each  track
only has 21 sectors, so there are a few unused bits.




*/








