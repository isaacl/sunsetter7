/****************************************************************************
 *     Copyright 2002-2004 Ben Nye                                          *
 *  For license terms, see the file COPYING that came with this program.    *
 *  book.cpp  simple opening book routine for Sunsetter                     *
 *  primary function:                                                       *
 *    bookMove(move *rightMove, boardStruct &where)                         *
 *  returns a move which the book suggests playing on the passed in board.  *
 ****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "variables.h"
#include "definitions.h"
#include "board.h"

extern FILE *findFile(const char *name, const char *mode);

// dynamicly allocated array of pointers to arrays of characters
// each array of characters will be one line from the book.
static char **booklines=NULL;
static int booklen=0;

/* todo for a decent book
init{
  read in from file(done)
  validate lines, truncate line at first illegal move.
  remove any lines with no legal moves.
  sort the strings(done/needs testing)
}
bookMove{
  use binsearch to find a matching line, then scan to find the
  first and last matching line.
  use rand() to pick a random line between the first and last.
}
*/

static int
qstrcmp(const void *a,const void *b){
	return strcmp((const char *)a,(const char *)b);
}

// load book file into ram, parsing it and sorting it.
static void
initBook(void)
{
	FILE *bookfile;
	char buff[MAX_STRING];
	char len;

	// initialize the book as being empty.
	booklines=(char **)malloc(1*sizeof(char *));
	booklines[0]=NULL;
	booklen=0;

	// open the book file, if no book file then this function is done.
	bookfile=findFile("noon.txt", "r");
	if(!bookfile){
		fprintf(stderr, "bookfile 'noon.txt' not found\n");
		return;
	}

	// read each line in from the file, and store a copy of that line.
	while(!feof(bookfile)){
		if(!fgets(buff, MAX_STRING, bookfile)) break;
		strtok(buff, "#;\n\r"); // strip off end of line and some comments
		len=strlen(buff);
		if(len){ // if the line contained some non-comment data, store it
			booklen++;
			booklines=(char **)realloc(booklines, (1+booklen)*sizeof(char *));
			booklines[booklen-1]=(char *)malloc(len+1);
			strcpy(booklines[booklen-1],buff);
		}
	}
	// end the array with a string which should sort after all legal moves.
	booklines[booklen]=(char *)malloc(6);
	strcpy(booklines[booklen], "zzzzz");

	// sort the array in lexical order
	qsort(booklines, booklen, sizeof(char *), qstrcmp);

	// close the book file
	fclose(bookfile);
}

int
bookMove(move *rightMove, boardStruct &where)
{
	static bool init_called = false;
	// rem s points to static memory, don't free
	const char *s=where.getLineText();
	char stringres[16];
	int curlen=strlen(s);
	int i;

	if(!init_called){
		init_called=true;
		initBook();
	}
	for(i=0;i<booklen;i++) if(strncmp(s, booklines[i], curlen)==0){
		strncpy(stringres, booklines[i]+curlen+1, 6);
		if(stringres[4]==' ') stringres[4]='\0';
		if(stringres[5]==' ') stringres[5]='\0';
		*rightMove=where.algebraicMoveToDBMove(stringres);
		if(rightMove->isBad()) return 0;
		return 1;
	}
	return 0;
}

