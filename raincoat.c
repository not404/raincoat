
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

 /*
  2005-02-09  gentoox@shallax.com  + Added a load of flash types
  2003-01-27  andy@warmcat.com     + Cosmetic edits, using character bars for 
											    progress
                                   + Support for 28xxx flash
  2003-01-06  andy@warmcat.com     + Created
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/io.h>
#include <ctype.h>
#include <signal.h>

#include "boot.h"

#include "BootFlash.h"

#define RAINCOAT_VERSION "0.6"

bool FlashingCallback(void * pvoidObjectFlash, ENUM_EVENTS ee, DWORD dwPos, DWORD dwExtent);

OBJECT_FLASH objectflash;

KNOWN_FLASH_TYPE aknownflashtype[1024] = { // Max of 1024 flash types.
#include "flashtypes.h"
};


void Progress(int nPercent) {
	int n=0;
	printf("  %3d%% .:(", nPercent);
	while(n++<((40 * nPercent)/100)) printf("*");
	while(n++<40) printf(".");
	printf("):.\r");
}

void ClearProgress(void) {
	int n=0;
	while(n++<54) printf(" ");
	printf("\r");
}

bool FlashingCallback(void * pvoidof, ENUM_EVENTS ee, DWORD dwPos, DWORD dwExtent)
{
//	OBJECT_FLASH * pof=(OBJECT_FLASH *)pvoidof;

	switch(ee) {
		case EE_ERASE_START:
			printf(" Erasing...\n");
			break;
		case EE_ERASE_UPDATE:
			Progress((dwPos*100)/dwExtent);
			break;
		case EE_ERASE_END:
			ClearProgress();
			printf("  Done  \n");
			break;
		case EE_ERASE_ERROR:
			printf("  ERASE ERROR AT +0x%lX...read 0x%02lX\n", dwPos, dwExtent&0xff);
			break;
		case EE_PROGRAM_START:
			printf(" Programming...\n");
			break;
		case EE_PROGRAM_UPDATE:
			Progress((dwPos*100)/dwExtent);
			break;
		case EE_PROGRAM_END:
			ClearProgress();
			printf("  Done      \n");
			break;
		case EE_PROGRAM_ERROR:
			printf("  PROGRAM ERROR AT +0x%lX...wrote 0x%02lX read 0x%02lX\n", dwPos, (dwExtent>>8), dwExtent&0xff);
			break;
		case EE_VERIFY_START:
			printf(" Verifying...\n");
			break;
		case EE_VERIFY_UPDATE:
			Progress((dwPos*100)/dwExtent);
			break;
		case EE_VERIFY_END:
			ClearProgress();
			printf("  Done      \n");
			break;
		case EE_VERIFY_ERROR:
			printf("  VERIFY ERROR AT +0x%lX...wrote 0x%02lX read 0x%02lX\n", dwPos, (dwExtent>>8), dwExtent&0xff);
			break;
	}
	return true;
}




int main(int argc, char * argv[])
{
	bool fProgram=false;
	bool fReadback=false;
	bool fVerbose=false;
	char szFilepathProgram[256]="";
	char szFilepathReadback[256]="";
	int fileMem;
	char szConfigFile[1024];

	//For some reason (anyone?) we OCCASIONALLY get a SIGTRAP, which
	//kills us, if we don't ignore it (or handle it).
	signal(SIGTRAP, SIG_IGN);
	
	strcpy(szConfigFile,"/etc/raincoat.conf");

		// construct the flash object

	objectflash.m_bManufacturerId=0;
	objectflash.m_bDeviceId=0;
	objectflash.m_dwLengthInBytes=0;
	objectflash.m_dwStartOffset=0;
	objectflash.m_dwLengthUsedArea=0;
	objectflash.m_pcallbackFlash=FlashingCallback;
	objectflash.m_fDetectedUsing28xxxConventions=false;

	strcpy(&objectflash.m_szFlashDescription[0], "Unknown");

	printf("raincoat Flasher  "RAINCOAT_VERSION"  " __DATE__ "  andy@warmcat.com  http://xbox-linux.sf.net\n");

		// map the BIOS region 0xff000000 - 0xffffffff so that we can touch it

	fileMem = open("/dev/mem", O_RDWR);
	if(!fileMem) { printf("Must be run as root\n"); return 1; }
	objectflash.m_pbMemoryMappedStartAddress = (BYTE *)mmap(0, 0x1000000, PROT_READ | PROT_WRITE, MAP_SHARED, fileMem , 0xff000000);
	if(objectflash.m_pbMemoryMappedStartAddress==NULL) { printf("Unable to map register memory\n"); return 1; }

	if (iopl(3)) {perror("iopl"); return 1;}

		// parse arguments

	{
		int n=1;
		while(n<argc) {

			if(strcmp(argv[n], "-p")==0) { // program
				n++;
				if(n>=argc) {
					printf("Missing argument after -p\n");
					return 1;
				}
				strncpy(szFilepathProgram, &argv[n][0], sizeof(szFilepathProgram)-1);
				fProgram=true;
			}

			if(strcmp(argv[n], "-r")==0) { // readback
				n++;
				if(n>=argc) {
					printf("Missing argument after -r\n");
					return 1;
				}
				strncpy(szFilepathReadback, &argv[n][0], sizeof(szFilepathReadback)-1);
				fReadback=true;
			}

			if(strcmp(argv[n], "-a")==0) { // start offset
				n++;
				if(n>=argc) {
					printf("Missing argument after -a\n");
					return 1;
				}
				if(strlen(argv[n])>8) {
					printf("-a argument too long\n");
					return 1;
				}
				sscanf(argv[n], "%lx", (DWORD *)&objectflash.m_dwStartOffset);
			}

			if(strcmp(argv[n], "-v")==0) { // verbose
				fVerbose=true;
			}

			if(strcmp(argv[n], "-c")==0) { // verbose
				n++;
				strcpy(szConfigFile,argv[n]);
			}

			n++;
		}
	}

		// bring in the conf file

	{
		int fileRead;
		struct stat statFile;
		printf("Reading %s... ",szConfigFile);

		fileRead = open(szConfigFile, O_RDONLY);
		if(fileRead>0) {

			fstat(fileRead, &statFile);

			if(fVerbose) printf("\n");

			{
				KNOWN_FLASH_TYPE *pkft=&aknownflashtype[0];
				BYTE * pbFile = (BYTE *)malloc(statFile.st_size+1);
				char * sz=(char *)pbFile;
				int nCountMaximumFlashTypes=(sizeof(aknownflashtype)/sizeof(KNOWN_FLASH_TYPE))-1;
				int nCountSeen=0;

				if(pbFile==NULL) { printf("unable to allocate %u bytes of memory\n", (unsigned int)statFile.st_size); return 1; }
				if(read(fileRead, &pbFile[0], statFile.st_size)<statFile.st_size) {
					printf("Failed to read full file\n");
					return 1;
				}
				pbFile[statFile.st_size]='\0';
				close(fileRead);

				while((*sz) && (nCountMaximumFlashTypes)) {
					if((strncmp(sz, "flash", 5)==0) || (strncmp(sz, "Flash", 5)==0) ) { // candidate
						sz+=5;
						while((*sz) && (isspace(*sz))) sz++;
						if(*sz=='=') {
							while((*sz) && (*sz!='x')) sz++;
							if(*sz) {
								int n;
								char *szHex;
								sz++;
								szHex=sz;
								n=9;
								while((n--)&&(*sz!=',')) sz++;
								if(n>=0) {
									sscanf(szHex, "%x", &n);
									pkft->m_bManufacturerId=(BYTE)(n>>8);
									pkft->m_bDeviceId=(BYTE)n;
									while((*sz) && (*sz!='\"')) sz++;
									n=sizeof(pkft->m_szFlashDescription)-1;
									if(*sz) {
										int nPos=0;
										sz++;
										while((n--) && (*sz) && (*sz!='\"')) {
											pkft->m_szFlashDescription[nPos++]=*sz++;
										}
										pkft->m_szFlashDescription[nPos++]='\0';
										if(*sz) {
											while((*sz) && (*sz!='x')) sz++;
											if(*sz) {
												sz++;
												szHex=sz;
												n=9;
												while((n--)&&(!isspace(*sz)) ) sz++;
												if(n>=0) {
													sscanf(szHex, "%lx", &pkft->m_dwLengthInBytes);

													if(fVerbose) printf("  0x%02X, 0x%02X, '%s', %08lX\n",
														pkft->m_bManufacturerId,
														pkft->m_bDeviceId,
														pkft->m_szFlashDescription,
														pkft->m_dwLengthInBytes
													);

													pkft++; nCountSeen++;
													if((--nCountMaximumFlashTypes)==0) { //
														printf("  (note, raincoat only supports %d flash types, rest ignored)\n", (sizeof(aknownflashtype)/sizeof(KNOWN_FLASH_TYPE))-1);
													}
												}
											}
										}
									}
								}
							}

						}

					}
					while((*sz) && (*sz!='\n') && (*sz!='\r')) sz++;
					while((*sz) && ((*sz=='\n') || (*sz=='\r'))) sz++;
				}

				free(pbFile);

				printf("%d flash types read\n", nCountSeen);

						// terminating entry is all zeros

				memset(pkft, 0, sizeof(KNOWN_FLASH_TYPE));
			}
			close(fileRead);
		} else {
			printf("(unable to open %s, using default list)\n",szConfigFile);
		}
	}


		// check device type, and exit if we don't recognize it

	{
		if(BootFlashGetDescriptor(&objectflash, &aknownflashtype[0])) {
			printf("\nDETECTED: %s\n", objectflash.m_szFlashDescription);
		} else {
			if(!fReadback) {
				printf("\nUNKNOWN DEVICE %s\n", objectflash.m_szFlashDescription);
				printf("Try adding the device ID to /etc/raincoat.conf\n");
				
				return(1);
			}
		}

		if(objectflash.m_dwStartOffset >= objectflash.m_dwLengthInBytes) {
			printf("-a start offset 0x%lX is too large for ROM size 0x%lX\n", objectflash.m_dwStartOffset, objectflash.m_dwLengthInBytes);
		}
	}

	if(argc==1) {
		printf(
			"\nraincoat [-p filetoprog] [-r filetodumpto] [-a hexoffset] [-v]\n\n"
			" -p filetoprog    Program flash with given file\n"
			" -r filetodumpto  Read whole flash back into file\n"
			" -a hexoffset     Optional start offset in flash, default 0\n"
			" -v               Verbose informational messages\n\n"
			" -c configfile    Use a userdefined config File\n" 
			"Example:  raincoat -p cromwell.bin\n\n"
			"Please note, -p will reprogram your BIOS flash\n"
			"  Please do not use if you don't understand what that\n"
			"  means, there is no simple undo for this if you\n"
			"  programmed the wrong thing.\n"
			"  -r is always safe to use, as is running with no args\n"
		);
		return 1;
	}



		// perform the selected actions according to commandline switches

	if(fProgram) { // perform programming action
		struct stat statFile;
		int fileRead;

		printf("Programming with %s...", szFilepathProgram);

		if(!objectflash.m_fIsBelievedCapableOfWriteAndErase) {
			printf("\nFlash is locked, unable to write\n");
			return 1;
		}

		fileRead = open(szFilepathProgram, O_RDONLY);
		if(fileRead<=0) {
			printf("unable to open file\n");
			return 1;
		}

		fstat(fileRead, &statFile);

		{
			BYTE * pbFile = (BYTE *)malloc(statFile.st_size);
			if(pbFile==NULL) { printf("unable to allocate %u bytes of memory\n", (unsigned int)statFile.st_size); return 1; }
			if(read(fileRead, &pbFile[0], statFile.st_size)<statFile.st_size) {
				printf("Failed to read full file\n");
				return 1;
			}
			printf("Read %u bytes from file\n",(unsigned int)statFile.st_size);
			close(fileRead);

			objectflash.m_dwLengthUsedArea=(DWORD)statFile.st_size;

			if(statFile.st_size > (objectflash.m_dwLengthInBytes - objectflash.m_dwStartOffset)) {
				printf("File is too large for available space\n");
				return 1;
			}

			if(BootFlashEraseMinimalRegion(&objectflash)) {
				if(!BootFlashProgram(&objectflash, pbFile)) {
					printf("   Programming Error: %s\n", objectflash.m_szAdditionalErrorInfo);
				}
			} else {
				printf("   Error: %s\n   Could your chip be write-protected?\n", objectflash.m_szAdditionalErrorInfo);
				if(objectflash.m_bManufacturerId==0xbf) {
					printf(
						"   Matrix/Xodus users need their switches both off\n"
						"   With a flashing orange LED.  Note that you need\n"
						"   to cycle power to change switch mode\n"
					);
				}
			}

			free(pbFile);
		}
	}



	if(fReadback) { // perform readback
		int fileDump;

		printf("Reading back to %s...\n", szFilepathReadback);

		fileDump = open(szFilepathReadback, O_CREAT | O_TRUNC | O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
		if(objectflash.m_dwLengthInBytes == 0) objectflash.m_dwLengthInBytes = 0x100000;
		write(fileDump, (BYTE *)objectflash.m_pbMemoryMappedStartAddress, objectflash.m_dwLengthInBytes);
		close(fileDump);
	}


		// finished with mapping

	if (iopl( 0)) {perror("ioperm"); return 1;}

	munmap((void *)objectflash.m_pbMemoryMappedStartAddress, 0x1000000);
	close(fileMem);

	printf("Completed\n");

	return 0;
}

