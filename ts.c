/*********************************************************************/
/* TS.C - A text search utility that meets MY needs.                 */
/* Bob Perper                                                        */
/* 6/11/98                                                           */
/*********************************************************************/
/* Test of git                                                       */
/*********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <fcntl.h>
#include <string.h>
#include <io.h>
#include <process.h>
#ifndef WIN32
   #define  INCL_BASE
   #define  INCL_DOS
   #define  INCL_SUB

   #include <os2.h>
#endif
#ifdef WIN32
   #include <windows.h>
   #include <wincon.h>
   #include <direct.h>
#endif

#ifdef WIN32
   #define  dAtomicSize    1024
#else
   #define  dAtomicSize    255
#endif

unsigned char  vlcaPattern[dAtomicSize];
unsigned char  vlcaSpec[dAtomicSize];
unsigned char  vlbSubdirs = 0;
unsigned char  vlbIgnoreCase = 0;

unsigned long  vllTotalHits = 0;
unsigned long  vllFileHits = 0;
unsigned char  vlbThisFileHit = 0;
unsigned long  vllErrorLine = 0;
unsigned char  vlbClearedScreen = 0;
unsigned char  vlbSkipFile = 0;
unsigned char  vlbSkipDir  = 0;

struct   tFilesList  {
   unsigned char  mcaFile[dAtomicSize];
   struct   tFilesList  *mspFilesListNext;
};

struct   tSkipList   {
   unsigned char  mcaSkip[dAtomicSize];
   struct   tSkipList   *mspSkipListNext;
};

#define  dNumDisplayLines  7
#define  dDisplayLine      3
#define  dMaxLineSize      512
unsigned char  vscaLines[dNumDisplayLines][dMaxLineSize];
#ifdef WIN32
CHAR_INFO      vlsCHAR_INFO[dNumDisplayLines * dMaxLineSize];
#else
unsigned char  vscaCellLines[dNumDisplayLines * dMaxLineSize][2];
#endif
#ifdef WIN32
   HANDLE   vlhConsoleOut;
   HANDLE   vlhConsoleIn;
#endif

struct   tSkipList   *vlspSkipListFiles = NULL;
struct   tSkipList   *vlspSkipListDirs = NULL;

unsigned long  vslTotalLines = 0;
unsigned long  vslTotalFiles = 0;

/*********************************************************************/
/* flvClearScreen                                                    */
/* Purpose  : Clears the screen (if it hasn't been done yet).        */
/* Inputs   : None.                                                  */
/* Outputs  : None.                                                  */
/* Returns  : None.                                                  */
/* NOTE     : Contains OS/2 specific routines.                       */ 
/*********************************************************************/

void   flvClearScreen(void)

{
   if (!vlbClearedScreen) {
#ifdef WIN32
      CONSOLE_SCREEN_BUFFER_INFO vssCONSOLE_SCREEN_BUFFER_INFO;
      DWORD                      vslConsoleSize;
      COORD                      vssCOORD = { 0, 0 };
      DWORD                      vslCharsWritten;
      GetConsoleScreenBufferInfo(vlhConsoleOut,&vssCONSOLE_SCREEN_BUFFER_INFO);
      vslConsoleSize = vssCONSOLE_SCREEN_BUFFER_INFO.dwSize.X * vssCONSOLE_SCREEN_BUFFER_INFO.dwSize.Y;
      printf("Console size: %d",vslConsoleSize);
      FillConsoleOutputCharacter(vlhConsoleOut,(TCHAR)' ',vslConsoleSize,vssCOORD,&vslCharsWritten);
      GetConsoleScreenBufferInfo(vlhConsoleOut,&vssCONSOLE_SCREEN_BUFFER_INFO);
      vslConsoleSize = vssCONSOLE_SCREEN_BUFFER_INFO.dwSize.X * vssCONSOLE_SCREEN_BUFFER_INFO.dwSize.Y;
      FillConsoleOutputAttribute(vlhConsoleOut,vssCONSOLE_SCREEN_BUFFER_INFO.wAttributes,vslConsoleSize,vssCOORD,&vslCharsWritten);
      SetConsoleCursorPosition(vlhConsoleOut,vssCOORD);
#else
      BYTE  vsbaBackground[2];
      vsbaBackground[0] = 0x20; /* space */
      vsbaBackground[1] = 7; /* Normal? */
      VioScrollUp(0,0,0xffff,0xffff,0xffff,vsbaBackground,0);
      VioSetCurPos(1,0,0);
#endif
      vlbClearedScreen = 1;
   }
}
/* flvClearScreen */




/*********************************************************************/
/* flvReportError                                                    */
/* Purpose  : Reports an error during a search.                      */
/* Inputs   : unsigned char vscaError[]                              */
/*                What's wrong.                                      */
/* Outputs  : None.                                                  */
/* Returns  : None.                                                  */
/* NOTE     : Contains OS/2 specific routines.                       */ 
/*********************************************************************/

void   flvReportError( 
   unsigned char  vscaError[])

{
   flvClearScreen();
#ifdef WIN32
   { 
      COORD vssCOORD;
      
      vssCOORD.X = 0;
      vssCOORD.Y = (short)(16 + vllErrorLine);
      SetConsoleCursorPosition(vlhConsoleOut,vssCOORD);
   }
#else
   VioSetCurPos((USHORT)(16 + vllErrorLine),0,0);
#endif
   printf("%s",vscaError);
   vllErrorLine++;
}
/* flvReportError */




/*********************************************************************/
/* flvReportStatus                                                   */
/* Purpose  : Reports a status message during a search.              */
/* Inputs   : unsigned char vscaStatus[]                             */
/*                What's going on.                                   */
/* Outputs  : None.                                                  */
/* Returns  : None.                                                  */
/* NOTE     : Contains OS/2 specific routines.                       */ 
/*********************************************************************/

void   flvReportStatus(
   unsigned char  vscaStatus[])

{
   flvClearScreen();
#ifdef WIN32
   { 
      COORD vssCOORD;
      
      vssCOORD.X = 0;
      vssCOORD.Y = 15;
      /* Clear the line.  */
      SetConsoleCursorPosition(vlhConsoleOut,vssCOORD);
      printf("%79s"," ");
      SetConsoleCursorPosition(vlhConsoleOut,vssCOORD);
   }
#else
   VioSetCurPos((USHORT)15,0,0);
   VioWrtNChar(" ",80,15,0,0); /* Clear the line */
   VioSetCurPos((USHORT)15,0,0);
#endif
   printf("%s",vscaStatus);
}
/* flvReportStatus */




/*********************************************************************/
/* flvBadParameter                                                   */
/* Purpose  : Reports a bad parameter.                               */
/* Inputs   : unsigned char vscaDescription[]                        */
/*                What's wrong.                                      */
/* Outputs  : None.                                                  */
/* Returns  : None.                                                  */
/*********************************************************************/

void   flvBadParameter(
   unsigned char  vscaDescription[])

{
   printf("%s\n",vscaDescription);
#ifdef WIN32
   printf("TS: A text search utility for 32-bit Windows\n");
#else
   printf("TS: A text search utility\n");
#endif
   printf("Form: TS <spec> <pattern> [/s] [/i] [/d<dir to skip>] [/f<file to skip>] [/u]\n");
   printf("   /s  : Include subdirectories in your search\n");
   printf("   /i  : Ignore case in the pattern\n");
   printf("   /d<dir to skip> : A directory name to skip (not fully qualified)\n");
   printf("   /f<file to skip>: A file name to skip (not fully qualified)\n");
   printf("   /u  : UPSTREAM skips for fast finds\n");
   printf("Skip options can repeat\n");
}
/* flvBadParameter */




/*********************************************************************/
/* flbAddSkip - Adds a skip entry to the list.                       */
/* Inputs   : vsbDirectory                                           */
/*               1 if this should be added to the dir list, 0 for the*/
/*               file list.                                          */
/*            vscaSkip[]                                             */
/*               The entry to add.                                   */
/* Outputs  : Globals                                                */
/* Returns  : 1 if its ok to continue.                               */
/*********************************************************************/

unsigned int   flbAddSkip(
   unsigned int   vsbDirectory,
   unsigned char  vscaSkip[])
   
{
   struct   tSkipList   *vsspSkipList;
   
   if (!vscaSkip[0]) {
      flvBadParameter("A skip parameter must have no spaces between the switch and the value");
      return(0);
   }
   if (!(vsspSkipList = malloc(sizeof(struct tSkipList)))) {
      printf("Insufficient memory allocating skip list\n");
      return(0);
   }
   memset(vsspSkipList,0,sizeof(struct tSkipList));
   strcpy(vsspSkipList->mcaSkip,vscaSkip);
   if (vsbDirectory) {
      vsspSkipList->mspSkipListNext = vlspSkipListDirs;
      vlspSkipListDirs = vsspSkipList;
      printf("Skip directory %s\n",vscaSkip);
   }
   else {
      vsspSkipList->mspSkipListNext = vlspSkipListFiles;
      vlspSkipListFiles = vsspSkipList;
      printf("Skip file %s\n",vscaSkip);
   }
   return(1);
}
/* flbAddSkip */




/*********************************************************************/
/* flbValidateParameters                                             */
/* Inputs   : Command line.                                          */
/* Outputs  : Globals                                                */
/* Returns  : 1 if its ok to continue.                               */
/*********************************************************************/

unsigned int   flbValidateParameters(
   unsigned int   vsiNumParameters,
   unsigned char  *vscpcaParameters[])

{
   unsigned int   vsiIndex = 3;

   if (vsiNumParameters < 3) {
      flvBadParameter("You must specify a pattern and file(s) to search");
      return(0);
   }
   strcpy(vlcaSpec,vscpcaParameters[1]);
   if (!(strchr(vlcaSpec,'\\'))) {
      /* Add in the current directory */
      char  vscaPath[dAtomicSize];
      getcwd(vscaPath,dAtomicSize);
      if (vscaPath[strlen(vscaPath) - 1] == '\\') {
         vscaPath[strlen(vscaPath) - 1] = 0;
      }
      sprintf(vlcaSpec,"%s\\%s",vscaPath,vscpcaParameters[1]);
   }
   strcpy(vlcaPattern,vscpcaParameters[2]);
   while (vsiIndex < vsiNumParameters) {
      if (vscpcaParameters[vsiIndex][0] != '/') {
         flvBadParameter("Switches must begin with a slash (/)");
         return(0);
      }
      switch (vscpcaParameters[vsiIndex][1]) {
         case  'S'   :
         case  's'   :
            printf("Subdirectory search\n");
            vlbSubdirs = 1;
            break;
         case  'I'   :
         case  'i'   :
            printf("Ignore case\n");
            vlbIgnoreCase = 1;
            strupr(vlcaPattern);
            break;
         case  'D'   :
         case  'd'   :
            if (!(flbAddSkip(1,&vscpcaParameters[vsiIndex][2]))) {
               return(0);
            }
            break;
         case  'F'   :
         case  'f'   :
            if (!(flbAddSkip(0,&vscpcaParameters[vsiIndex][2]))) {
               return(0);
            }
            break;
         case  'U'   :
         case  'u'   :
            printf("Add UPSTREAM skips\n");
            if ((!(flbAddSkip(1,"test"))) ||
                (!(flbAddSkip(1,"customer"))) ||
                (!(flbAddSkip(1,"diag256d"))) ||
                (!(flbAddSkip(1,"dist"))) ||
                (!(flbAddSkip(1,"document"))) ||
                (!(flbAddSkip(1,"gateway"))) ||
                (!(flbAddSkip(1,"ims"))) ||
                (!(flbAddSkip(1,"proposal"))) ||
                (!(flbAddSkip(1,"test"))) ||
                (!(flbAddSkip(1,"testdocs"))) ||
                (!(flbAddSkip(1,"usjava12")))) {
               return(0);
            }
            printf("Subdirectory search\n");
            vlbSubdirs = 1;
            break;
         default  :
            flvBadParameter("Unknown switch");
            return(0);
      }
      vsiIndex++;
   }
   return(1);
}
/* flvValidateParameters */




/*********************************************************************/
/* flvDisplayHit                                                     */
/* Purpose  : Displays a hit and prompts the user.                   */
/* Inputs   : unsigned char vscaFileName[]                           */
/*                Searches in the file for the pattern.              */
/*            unsigned long vslLines                                 */
/*                Total number of lines read.                        */
/*          : unsigned long vslFoundLine                             */
/*                The line it was found on.                          */
/*            unsigned long vslColumn                                */
/*                The column it was found on.                        */
/* Outputs  : None.                                                  */
/* Returns  : None.                                                  */
/* NOTE     : Contains OS/2 specific routines.                       */ 
/*********************************************************************/

void  flvDisplayHit(
   unsigned char  vscaFileName[],
   unsigned long  vslLines,
   unsigned long  vslFoundLine, 
   unsigned long  vslColumn)

{
   unsigned int   vsiLength = 0;
   unsigned long  vslCurrentLine;
   unsigned long  vslByte = 0;
   unsigned long  vslLineColumn = 0;
   unsigned int   vsiLineLength = 0;

   vllErrorLine = 0;
   vllTotalHits++;
   if (!vlbThisFileHit) {
      vllFileHits++;
      vlbThisFileHit = 1;
   }
   /* Clear the screen.  */
   vlbClearedScreen = 0;
   flvClearScreen();
   /* Display the title.  */
#ifdef WIN32
   { 
      COORD vssCOORD;
      
      vssCOORD.X = 0;
      vssCOORD.Y = 1;
      /* Clear the line.  */
      SetConsoleCursorPosition(vlhConsoleOut,vssCOORD);
   }
#else
   VioSetCurPos(1,0,0);
#endif
   /* Copy to the cells.  */
   if (vslFoundLine < (dNumDisplayLines / 2)) {
      vslCurrentLine = 0;
   }
   else {
      vslCurrentLine = vslFoundLine - (dNumDisplayLines / 2);
   }
   printf("File: %s at line %ld column %ld\n",
          vscaFileName,
          vslFoundLine + 1,
          vslColumn);
   while (vslCurrentLine < vslLines) {
      if ((vscaLines[vslCurrentLine % dNumDisplayLines][vslLineColumn] == '\n') ||
          (!vscaLines[vslCurrentLine % dNumDisplayLines][vslLineColumn])) {
         /* Write what we've got.  */
#ifdef WIN32
         /* For Win32, advance to the next line.  */
         CONSOLE_SCREEN_BUFFER_INFO vssCONSOLE_SCREEN_BUFFER_INFO;
         GetConsoleScreenBufferInfo(vlhConsoleOut,&vssCONSOLE_SCREEN_BUFFER_INFO);
         vssCONSOLE_SCREEN_BUFFER_INFO.dwCursorPosition.X = 0;
         vssCONSOLE_SCREEN_BUFFER_INFO.dwCursorPosition.Y++;
         SetConsoleCursorPosition(vlhConsoleOut,vssCONSOLE_SCREEN_BUFFER_INFO.dwCursorPosition);
#else
         USHORT   vsiRow;
         USHORT   vsiCol;
         VioGetCurPos(&vsiRow,&vsiCol,0);
         VioWrtCellStr((PCH)vscaCellLines,(USHORT)(vslByte * 2),vsiRow,vsiCol,0);
#endif
         vsiLineLength = strlen(vscaLines[vslCurrentLine % dNumDisplayLines]);
#ifdef WIN32
#else
         VioSetCurPos(vsiRow + (vsiLineLength / 80) + 1,0,0);
#endif
         vslByte = 0;
         vslCurrentLine++;
         vslLineColumn = 0;
      }
      else {
#ifdef WIN32
         CONSOLE_SCREEN_BUFFER_INFO vssCONSOLE_SCREEN_BUFFER_INFO;
         GetConsoleScreenBufferInfo(vlhConsoleOut,&vssCONSOLE_SCREEN_BUFFER_INFO);
#else
         vscaCellLines[vslByte][0] = vscaLines[vslCurrentLine % dNumDisplayLines][vslLineColumn];
#endif
         if ((vslCurrentLine == vslFoundLine) &&
             (vslLineColumn >= vslColumn) &&
             (vslLineColumn < vslColumn + strlen(vlcaPattern))) {
#ifdef WIN32
            SetConsoleTextAttribute(vlhConsoleOut,BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY);
#else
            vscaCellLines[vslByte][1] = 0x70; /* inverse */
#endif
         }
         else {
#ifdef WIN32
            SetConsoleTextAttribute(vlhConsoleOut,FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
#else
            vscaCellLines[vslByte][1] = 10; /* let's try it... */
#endif
         }
#ifdef WIN32
         printf("%c",vscaLines[vslCurrentLine % dNumDisplayLines][vslLineColumn]);
         SetConsoleTextAttribute(vlhConsoleOut,vssCONSOLE_SCREEN_BUFFER_INFO.wAttributes);
         vssCONSOLE_SCREEN_BUFFER_INFO.dwCursorPosition.X++;
         SetConsoleCursorPosition(vlhConsoleOut,vssCONSOLE_SCREEN_BUFFER_INFO.dwCursorPosition);
#endif
         vslByte++;
         vslLineColumn++;
      }
   }
#ifdef WIN32
   /* Done above.  */
#else
   {
      USHORT   vsiRow;
      USHORT   vsiCol;
      VioGetCurPos(&vsiRow,&vsiCol,0);
      VioWrtCellStr((PCH)vscaCellLines,(USHORT)(vslByte * 2),vsiRow,vsiCol,0);
   }
#endif
#ifdef WIN32
   {
      COORD vssCOORD = { 0,15 };
      SetConsoleCursorPosition(vlhConsoleOut,vssCOORD);
   }
#else
   VioSetCurPos(15,0,0);
#endif
   printf("Continue searching (Yes, No, Skip, Directory skip, Edit, neW edit) -> ");
   {
      int   ch;
      ch = getch();
      printf("%c\n",ch);
      switch (ch) {
         case  'N'   :
         case  'n'   :
#ifdef WIN32
            {
               COORD vssCOORD = { 0,17 };
               SetConsoleCursorPosition(vlhConsoleOut,vssCOORD);
            }
#else
            VioSetCurPos(17,0,0);
#endif
            printf("\nUser requested end of search\n");
            printf("%ld hits in %ld files\n",vllTotalHits,vllFileHits);
            exit(0);
         case  'S'   :
         case  's'   :
#ifdef WIN32
            {
               COORD vssCOORD = { 0,16 };
               SetConsoleCursorPosition(vlhConsoleOut,vssCOORD);
            }
#else
            VioSetCurPos(16,0,0);
#endif
            printf("Skip file at user request\n");
            vlbSkipFile = 1;
            break;
         case  'D'   :
         case  'd'   :
#ifdef WIN32
            {
               COORD vssCOORD = { 0,16 };
               SetConsoleCursorPosition(vlhConsoleOut,vssCOORD);
            }
#else
            VioSetCurPos(16,0,0);
#endif
            printf("Skip directory at user request\n");
            vlbSkipFile = 1;
            vlbSkipDir = 1;
            break;
         case  'E'   :
         case  'e'   :
         case  'W'   :
         case  'w'   :
            {
               unsigned char *vscpEditor;
               unsigned char vscaLine[255];
               unsigned char vscaEditFileName[dAtomicSize];
               if (!(vscpEditor = getenv("EDITOR"))) {
                  vscpEditor = "vs";
               }
               if ((ch == 'W') || (ch == 'w')) {
                  sprintf(vscaLine,"+new -#%lu",vslFoundLine + 1);
               }
               else {
                  sprintf(vscaLine,"-#%lu",vslFoundLine + 1);
               }
               /* We need to surround "file name" with quotes in case there are spaces in the name.  */
               sprintf(vscaEditFileName,"\"%s\"",vscaFileName);
               if (spawnlp(P_NOWAIT,vscpEditor,vscpEditor,vscaEditFileName,vscaLine,NULL) == -1) {
#ifdef WIN32
                  {
                     COORD vssCOORD = { 0,16 };
                     SetConsoleCursorPosition(vlhConsoleOut,vssCOORD);
                  }
#else
                  VioSetCurPos(16,0,0);
#endif
                  printf("Error %d starting editor %s\n",errno,vscpEditor);
               }
            }
            break;
      }
   }
}
/* DisplayHit */




/*********************************************************************/
/* flbCheckLine                                                      */
/* Purpose  : Checks a single line/column for a hit.                 */
/* Inputs   : unsigned char vscaFileName[]                           */
/*                Searches in the file for the pattern.              */
/*            unsigned long vslLines                                 */
/*                Total number of lines read.                        */
/*            unsigned long vslCheckLine                             */
/*                The current line number to be checked.             */
/* I/O      : unsigned long *vslpColumn                              */
/*                The column being checked.  Must be initialzed      */
/*                to 0 when a new line is read.                      */
/* Outputs  : None.                                                  */
/* Returns  : 1 if we're not yet done.                               */
/*********************************************************************/

int   flbCheckLine(
   unsigned char  vscaFileName[],
   unsigned long  vslLines,
   unsigned long  vslCheckLine,
   unsigned long  *vslpColumn)

{
   unsigned char  vscaUpperLine[dMaxLineSize];
   unsigned char  *vscpLine = &vscaLines[(vslCheckLine) % dNumDisplayLines][*vslpColumn];
   unsigned char  *vscpFound;

   //vslLines--; /* Reduce because this is pointing one past where we are.  */
   if (vlbIgnoreCase) {
      strcpy(vscaUpperLine,vscpLine);
      strupr(vscaUpperLine);
      vscpLine = vscaUpperLine;
   }
   if (vscpFound = strstr(vscpLine,vlcaPattern)) {
      flvDisplayHit(vscaFileName,vslLines,vslCheckLine,(*vslpColumn) + (vscpFound - vscpLine));
      //(*vslpColumn) = (*vslpColumn) + (vscpLine - vscpFound + strlen(vlcaPattern) + 1);
      //flbCheckLine(vscaFileName,vslLines,vslCheckLine,vslpColumn);
      if (vlbSkipFile) {
         return(1);
      }
   }
   *vslpColumn = 0;
   if ((vslCheckLine) > vslLines) {
      return(0);
   }
   return(1);
}
/* flbCheckLine */




/*********************************************************************/
/* flcpReadLine                                                      */
/* Purpose  : Reads a line.                                          */
/* Inputs   : int vsiFileHandle                                      */
/*                The file handle.                                   */
/*            int vsiLineSize                                        */
/*                The max number of bytes to read.                   */
/*            int vsbFirstCall                                       */
/*                dTrue if this is the first call after the open.    */
/* Outputs  : char *vscaLine                                         */
/*                The line we read.                                  */
/* Returns  : A pointer to vscaLine if it worked.                    */
/*********************************************************************/

char  *flcpReadLine(
   int   vsiFileHandle,
   int   vsiLineSize,
   int   vsbFirstCall,
   char  *vscaLine)

{
   #define  dBufferSize 16384
   static   char     vlcaBuffer[dBufferSize];
   static   int      vliBytesInBuffer;
   static   int      vliPosition;

   int   vsiStringIndex = 0;

   if (vsbFirstCall) {
      vliPosition = 0;
      vliBytesInBuffer = 0;
   }
   vscaLine[0] = 0;
   do {
      char   *vscpLF;

      /* Get some data if we need to.  */
      if (vliPosition >= vliBytesInBuffer) {
         vliBytesInBuffer = read(vsiFileHandle,vlcaBuffer,dBufferSize);
         if (!vliBytesInBuffer) {
            /* Done.  */
            /* Return all that's in the buffer.  */
            if (vsiStringIndex) {
               return(vscaLine);
            }
            return(NULL);
         }
         if (vliBytesInBuffer == -1) {
            char  vscaError[dAtomicSize];
            sprintf(vscaError,"Error #%d in read",errno);
            flvReportError(vscaError);
            return(NULL);
         }
         /*
         {
            char vscaStatus[1024];
            sprintf(vscaStatus,"!!!Read %d bytes",vliBytesInBuffer);
            flvReportStatus(vscaStatus);
         }
         */
         vliPosition = 0;
      }
      /*
          {
            char vscaStatus[1024];
            sprintf(vscaStatus,"!!!StringIndex %d, Position %d BytesInBuffer %d",vsiStringIndex,vliPosition,vliBytesInBuffer);
            flvReportStatus(vscaStatus);
         }
      */
     /* Copy until the end of a line (suppressing the CR/LFs) */
      if (vscpLF = memchr(&vlcaBuffer[vliPosition],'\n',vliBytesInBuffer - vliPosition)) {
         int   vsiBytesToCopy;
         vsiBytesToCopy = vscpLF - &vlcaBuffer[vliPosition];
         if (vsiBytesToCopy + vsiStringIndex >= vsiLineSize) {
            vsiBytesToCopy = vsiLineSize - vsiStringIndex - 1;
         }
         /*
          {
            char vscaStatus[1024];
            sprintf(vscaStatus,"!!!fFound NewLine, BytesToCopy %d StringIndex %d, Position %d BytesInBuffer %d",vsiBytesToCopy,vsiStringIndex,vliPosition,vliBytesInBuffer);
            flvReportStatus(vscaStatus);
         }
         */
         memcpy(&vscaLine[vsiStringIndex],&vlcaBuffer[vliPosition],vsiBytesToCopy);
         vsiStringIndex += vsiBytesToCopy;
         vliPosition += (vsiBytesToCopy + 1);
         vscaLine[vsiStringIndex] = 0;
         if ((vsiStringIndex) &&
             (vscaLine[vsiStringIndex - 1] == '\r')) {
            vscaLine[vsiStringIndex - 1] = 0;
         }
         /*
         {
            char vscaStatus[1024];
            sprintf(vscaStatus,"!!!Found LF string: %s",vscaLine);
            flvReportStatus(vscaStatus);
         }
         */
         vslTotalLines++;
         return(vscaLine);
      }
      else {
         /* Not found.  Use all we've got and get more... */
         int   vsiBytesToCopy;
         int   vsbDone = 0;

         vsiBytesToCopy = vliBytesInBuffer - vliPosition;
         if (vsiBytesToCopy + vsiStringIndex >= vsiLineSize) {
            vsiBytesToCopy = vsiLineSize - vsiStringIndex - 1;
            vsbDone = 1;
         }
         /*
          {
            char vscaStatus[1024];
            sprintf(vscaStatus,"!!!No NewLine, BytesToCopy %d Done %d StringIndex %d, Position %d BytesInBuffer %d",vsiBytesToCopy,vsbDone,vsiStringIndex,vliPosition,vliBytesInBuffer);
            flvReportStatus(vscaStatus);
         }
         */
         memcpy(&vscaLine[vsiStringIndex],&vlcaBuffer[vliPosition],vsiBytesToCopy);
         /*
          {
            char vscaStatus[1024];
            sprintf(vscaStatus,"!!!Did No NewLine Copy, BytesToCopy %d Done %d StringIndex %d, Position %d BytesInBuffer %d",vsiBytesToCopy,vsbDone,vsiStringIndex,vliPosition,vliBytesInBuffer);
            flvReportStatus(vscaStatus);
         }
         */
         vliPosition += vsiBytesToCopy;
         vsiStringIndex += vsiBytesToCopy;
         vscaLine[vsiStringIndex] = 0;
         if (vsbDone) {
            /*
            {
               char vscaStatus[1024];
               sprintf(vscaStatus,"!!!NO LF string: %s",vscaLine);
               flvReportStatus(vscaStatus);
            }
            */
            vslTotalLines++;
            return(vscaLine);
         }
      }
   } while (1);
}
/* flcpReadLine */




/*********************************************************************/
/* flbSearchInFile                                                   */
/* Purpose  : Performs the pattern search.                           */
/* Inputs   : unsigned char vscaFileName[]                           */
/*                Searches in the file for the pattern.              */
/* Outputs  : None.                                                  */
/* Returns  : 1 most of the time.                                    */
/*********************************************************************/

int   flbSearchInFile(
   unsigned char  vscaFileName[])

{
   int   vsiFileHandle;
   unsigned long  vslLine = 0;
   unsigned long  vslCheckLine = 0;
   unsigned long  vslColumn;
   char  *vscpRead;

   vlbSkipFile = 0;
   flvReportStatus(vscaFileName);
   vlbThisFileHit = 0;
   vsiFileHandle = open(vscaFileName,O_RDONLY | O_BINARY);
   if (vsiFileHandle == -1) {
      char  vscaError[dAtomicSize];
      sprintf(vscaError,"Error %d opening %s",errno,vscaFileName);
      flvReportError(vscaError);
      return(0);
   }
   /* Pre-read until we've filled the lines buffer.  */
   vslTotalFiles++;
   do {
      if (vscpRead = flcpReadLine(vsiFileHandle,dMaxLineSize,vslLine == 0,vscaLines[vslLine % dNumDisplayLines])) {
         vslLine++;
         vslColumn = 0;
      }
   } while ((vscpRead) && (vslLine < (dNumDisplayLines / 2)));
   if (vscpRead) {
      flbCheckLine(vscaFileName,vslLine,vslCheckLine,&vslColumn);
      vslCheckLine++;
      while ((!vlbSkipFile) && (vscpRead = flcpReadLine(vsiFileHandle,dMaxLineSize,0,vscaLines[vslLine % dNumDisplayLines]))) {
         vslLine++;
         vslColumn = 0;
         flbCheckLine(vscaFileName,vslLine,vslCheckLine,&vslColumn);
         vslCheckLine++;
      }
   }
   if (vslLine) {
      while ((!vlbSkipFile) && (flbCheckLine(vscaFileName,vslLine,vslCheckLine,&vslColumn))) {
         vslCheckLine++;
         /* Keep checking.  */
      }
   }
   close(vsiFileHandle);
   return(1);
}
/* flbSearchInFile */


/*********************************************************************/
/* flbSkip                                                           */
/* Purpose  : Determine if we should skip this file or dir.          */
/* Inputs   : vsspSkipList                                           */
/*                The skip list to check.                            */
/*            vscpEntry                                              */
/*                The file or directory.                             */
/* Outputs  : None.                                                  */
/* Returns  : 1 if a file/dir should be skipped.                     */
/*********************************************************************/

int   flbSkip(
   struct   tSkipList   *vsspSkipList,
   unsigned char vscaEntry[])
   
{
   while (vsspSkipList) {
      if (!(stricmp(vscaEntry,vsspSkipList->mcaSkip))) {
         unsigned char  vscaMessage[dAtomicSize];
         sprintf(vscaMessage,"Skip: %s",vscaEntry);
         return(1);
      }
      vsspSkipList = vsspSkipList->mspSkipListNext;
   }
   return(0);
}
/* flbSkip */




/*********************************************************************/
/* flbSearch                                                         */
/* Purpose  : Performs the file search.                              */
/* Inputs   : unsigned char vscaSpec[]                               */
/*                The spec to search.                                */
/* Outputs  : None.                                                  */
/* Returns  : 1 if a file was searched.                              */
/* NOTE     : Contains OS/2 specific routines.                       */ 
/*********************************************************************/

int   flbSearch(
   unsigned char  vscaSpec[])

{
#ifdef WIN32
   HANDLE   vshFind;
   WIN32_FIND_DATA   vssWIN32_FIND_DATA;
   int      vsiReturnCode;
   #define  vsiSearchCount 1
#else
   USHORT   vsiReturnCode;
   HDIR     vsiHDIR = HDIR_SYSTEM;
   FILEFINDBUF2   vssFILEFINDBUF2;
   USHORT   vsiSearchCount = 1;
#endif
   struct   tFilesList  *vsspFilesListHead = NULL;
   struct   tFilesList  *vsspFilesListLast = NULL;
   char  *vscpLastBackslash;
   unsigned char  vscaDir[dAtomicSize];
   unsigned char  vscaPattern[80];

   vlbSkipDir = 0;
   /* Find ALL subdirectories and then recursively call.  */
   vscpLastBackslash = strrchr(vscaSpec,'\\');
   if (vscpLastBackslash) {
      memcpy(vscaDir,vscaSpec,vscpLastBackslash - vscaSpec);
      vscaDir[vscpLastBackslash - vscaSpec] = 0;
      strcpy(vscaPattern,vscpLastBackslash + 1);
   }
   else {
      strcpy(vscaDir,"");
      strcpy(vscaPattern,vscaSpec);
   }
#ifdef WIN32
   vsiReturnCode = 0;
   if ((vshFind = FindFirstFile(vscaSpec,
                                &vssWIN32_FIND_DATA)) == INVALID_HANDLE_VALUE) {
      vsiReturnCode = GetLastError();
      if ((vsiReturnCode != ERROR_NO_MORE_FILES) &&
          (vsiReturnCode != ERROR_FILE_NOT_FOUND) &&
          (vsiReturnCode != ERROR_PATH_NOT_FOUND)) {
         char  vscaError[dAtomicSize];
         sprintf(vscaError,"Search error #%ld (%s)",(unsigned long)vsiReturnCode,vscaSpec);
         flvReportError(vscaError);
         return(0);
      }
   }
#else
   /* Find a file.  */
   if (vsiReturnCode = DosFindFirst2(vscaSpec,
                                     &vsiHDIR,
                                     FILE_NORMAL | FILE_READONLY | FILE_HIDDEN | FILE_SYSTEM,
                                     &vssFILEFINDBUF2,
                                     sizeof(vssFILEFINDBUF2),
                                     &vsiSearchCount,
                                     FIL_QUERYEASIZE,
                                     0)) {
      if ((vsiReturnCode != ERROR_NO_MORE_FILES) &&
          (vsiReturnCode != ERROR_FILE_NOT_FOUND) &&
          (vsiReturnCode != ERROR_PATH_NOT_FOUND)) {
         char  vscaError[dAtomicSize];
         sprintf(vscaError,"Search error #%ld (%s)",(unsigned long)vsiReturnCode,vscaSpec);
         flvReportError(vscaError);
         return(0);
      }
   }
#endif
   while ((!vsiReturnCode) && (vsiSearchCount) && (!vlbSkipDir)) {
      char  vscaFile[dAtomicSize];
      unsigned int   vsbSkip = 0;
      char  *vscpFile;
#ifdef WIN32
      if (!(vssWIN32_FIND_DATA.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
#else
      {
#endif
#ifdef WIN32
         vscpFile = vssWIN32_FIND_DATA.cFileName;
#else
         vscpFile = vssFILEFINDBUF2.achName;
#endif
         if (vscaDir[0]) {
            sprintf(vscaFile,"%s\\%s",
                             vscaDir,
                             vscpFile);
         }
         else {
            strcpy(vscaFile,vscpFile);
         }
         if (!(flbSkip(vlspSkipListFiles,vscpFile))) {
            flbSearchInFile(vscaFile);
         }
      }
#ifdef WIN32
      vsiReturnCode = 0;
      if (!(FindNextFile(vshFind,&vssWIN32_FIND_DATA))) {
         vsiReturnCode = GetLastError();
      }
#else
      vsiReturnCode = DosFindNext(vsiHDIR,
                                  (PFILEFINDBUF)&vssFILEFINDBUF2,
                                  sizeof(vssFILEFINDBUF2),
                                  &vsiSearchCount);
#endif
   }
#ifdef WIN32
   FindClose(vshFind);
#endif
   if ((!vlbSkipDir) &&
       (vsiReturnCode != ERROR_NO_MORE_FILES) &&
       (vsiReturnCode != ERROR_FILE_NOT_FOUND) &&
       (vsiReturnCode != ERROR_PATH_NOT_FOUND)) {
      char  vscaError[dAtomicSize];
      sprintf(vscaError,"Subsequent search error #%ld (%s)",(unsigned long)vsiReturnCode,vscaSpec);
      flvReportError(vscaError);
   }

   if ((vlbSubdirs) && (!vlbSkipDir)) {
      unsigned char  vscaDirSearchSpec[dAtomicSize];

      sprintf(vscaDirSearchSpec,"%s\\*.*",vscaDir);
#ifdef WIN32
   vsiReturnCode = 0;
   if ((vshFind = FindFirstFile(vscaDirSearchSpec,
                                &vssWIN32_FIND_DATA)) == INVALID_HANDLE_VALUE) {
      char  vscaError[dAtomicSize];
      vsiReturnCode = GetLastError();
#else
      vsiHDIR = HDIR_SYSTEM;
      vsiSearchCount = 1;
      if (vsiReturnCode = DosFindFirst2(vscaDirSearchSpec,
                                        &vsiHDIR,
                                        FILE_NORMAL | FILE_READONLY | FILE_HIDDEN | FILE_SYSTEM | FILE_DIRECTORY,
                                        &vssFILEFINDBUF2,
                                        sizeof(vssFILEFINDBUF2),
                                        &vsiSearchCount,
                                        FIL_QUERYEASIZE,
                                        0)) {
         char  vscaError[dAtomicSize];
#endif
         sprintf(vscaError,"Directory search error #%ld (%s)",(unsigned long)vsiReturnCode,vscaDirSearchSpec);
         flvReportError(vscaError);
         return(0);
      }
      do {
         unsigned char  *vscpFileName;
         int            vsiAttributes;
         int            vsbDirectory;
#ifdef WIN32
         vscpFileName = vssWIN32_FIND_DATA.cFileName;
         vsiAttributes = vssWIN32_FIND_DATA.dwFileAttributes;
         vsbDirectory = vsiAttributes & FILE_ATTRIBUTE_DIRECTORY;
#else
         vscpFileName = vssFILEFINDBUF2.achName;
         vsiAttributes = vssFILEFINDBUF2.attrFile;
         vsbDirectory = vssFILEFINDBUF2.attrFile & FILE_DIRECTORY;
#endif
         if ((vsbDirectory) &&
             (strcmp(vscpFileName,".")) &&
             (strcmp(vscpFileName,"..")) &&
             (!(flbSkip(vlspSkipListDirs,vscpFileName)))) {
            /* Save it.  */
            struct   tFilesList  *vsspFilesList;
            if (!(vsspFilesList = malloc(sizeof(struct tFilesList)))) {
               flvReportError("Malloc error in allocating files structure");
               return(0);
            }
            strcpy(vsspFilesList->mcaFile,vscpFileName);
            vsspFilesList->mspFilesListNext = NULL;
            if (!vsspFilesListHead) {
               vsspFilesListHead = vsspFilesList;
            }
            else {
               vsspFilesListLast->mspFilesListNext = vsspFilesList;
            }
            vsspFilesListLast = vsspFilesList;
         }
#ifdef WIN32
         vsiReturnCode = 0;
         if (!(FindNextFile(vshFind,&vssWIN32_FIND_DATA))) {
            vsiReturnCode = GetLastError();
         }
#else
         vsiReturnCode = DosFindNext(vsiHDIR,
                                     (PFILEFINDBUF)&vssFILEFINDBUF2,
                                     sizeof(vssFILEFINDBUF2),
                                     &vsiSearchCount);
#endif
      } while ((!vsiReturnCode) && (vsiSearchCount) && (!vlbSkipDir));
#ifdef WIN32
      FindClose(vshFind);
#endif
      if ((!vlbSkipDir) &&
          (vsiReturnCode != ERROR_NO_MORE_FILES) &&
          (vsiReturnCode != ERROR_FILE_NOT_FOUND) &&
          (vsiReturnCode != ERROR_PATH_NOT_FOUND)) {
         char  vscaError[dAtomicSize];
         sprintf(vscaError,"Directory subsequent search error #%ld (%s)",(unsigned long)vsiReturnCode,vscaDirSearchSpec);
         flvReportError(vscaError);
      }     
   }
   {
      while (vsspFilesListHead) {
         /* Add the file name to the spec.  */
         unsigned char  vscaNewSpec[dAtomicSize];

         if (vscaDir[0]) {
            sprintf(vscaNewSpec,"%s\\%s\\%s",vscaDir,vsspFilesListHead->mcaFile,vscaPattern);
         }
         else {
            sprintf(vscaNewSpec,"%s\\%s",vsspFilesListHead->mcaFile,vscaPattern);
         }
         if (!vlbSkipDir) {
            flbSearch(vscaNewSpec);
            vlbSkipDir = 0;
         }
         vsspFilesListLast = vsspFilesListHead;
         vsspFilesListHead = vsspFilesListHead->mspFilesListNext;
         free(vsspFilesListLast);
      }
   }
   return(1);
}

/*********************************************************************/
/* main                                                              */
/* Inputs   : See flvBadParameter                                    */
/* Outputs  : Nothing.                                               */
/* Returns  : 0 if it made a search attempt.                         */
/*********************************************************************/

int   main(
   unsigned int   vsiNumParameters,
   unsigned char  *vscpcaParameters[])

{
#ifdef WIN32
   vlhConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
   vlhConsoleIn = GetStdHandle(STD_INPUT_HANDLE);
#endif
   if (flbValidateParameters(vsiNumParameters,vscpcaParameters)) {
      if (flbSearch(vlcaSpec)) {
      }
#ifdef WIN32
      {
         COORD vssCOORD;
         
         vssCOORD.X = 0;
         vssCOORD.Y = 17;
         SetConsoleCursorPosition(vlhConsoleOut,vssCOORD);
      }
#else
      VioSetCurPos(17,0,0);
#endif
      printf("\n%ld hits in %ld files (%ld lines and %ld files searched)\n",vllTotalHits,vllFileHits,vslTotalLines,vslTotalFiles);
      return(0);
   }
   return(1);
}
/* main */

