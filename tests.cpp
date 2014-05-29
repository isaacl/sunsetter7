/* ***************************************************************************
 *									  Sunsetter                              *
 *                                                                           *
 *  Copywrite (c) 1997/2000 Ben Dean-Kawamura, Georg v. Zimmermann           *
 *                                                                           *
 *  For license terms, see the file COPYING that came with this program.     *
 *                                                                           *
 *  Name: tests.cc                                                           *
 *                                                                           *
 *  Purpose: go through a crazyhouse game and analyze all or one position    *
 *           or use a crazyhouse game to test make/unmake eval and movegen   *
 *           speed.                                                          *
 *																			 *
 *  The Command line for  testbpgn() is:                                     *
 *  "sunsetter test <bpgn file to read> <* move>"							 *
 *                                                                           *
 *  where <* move> is optional, else the whole game is looked at             *
 *  format is for example "34 WHITE" or "7 BLACK"                            *
 *                                                                           *
 *                                                                           *
 *************************************************************************** */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "board.h"
#include "brain.h"
#include "notation.h"
#include "interface.h"

// number of times to do each test in the speedtest
#define REPEATCOUNT (40000)

/* Function: nextToken
 * Input:    A file and a string to fill
 * Output:   None
 * Purpose:  Gets the next BGPN token from the file
 */

void nextToken(FILE *f, char *str)
{
  char c, buf[MAX_STRING], endChar;

  if(fscanf(f, " %c", &c) == EOF) {
    strcpy(str, "");
    return;
  }
  sprintf(str, "%c", c);
  if(c == '[' || c == '{') {      /* if it's an open brace then go to
                                     the next close brace */
    if(c == '[') endChar = ']';
    else endChar = '}';

    do {
      if(fscanf(f, "%s", buf) == EOF) {
        fprintf(stderr, "Error parsing BGPN file, unterminated %c\n", c);
        strcpy(str, "");
        return;
      }
      strcat(str, buf); strcat(str," "); 
    } while(buf[strlen(buf) - 1] != endChar);

    return;
  }
  fscanf(f, "%s", buf);
  strcat(str, buf);
  return;
}


/* Function: nextMoveOrResult
 * Input:    A BGPN file to read, which game to get the next move from and
 *           a string to fill 
 * Output:   0, -1 if an error occured
 * Purpose:  Reads the next move from a BGPN file.  It fills a string with the
 *           next move.  If there isn't another move in the game then it fills
 *           the string with "OVER"
 */



int nextMoveOrResult(FILE *fin, char game, char *result)
{
  char buf[MAX_STRING];
  int i;
  char ch;

  for (;;) {

  /* Skip over any comments that might be there.  Taking note to see if the
     comments are that the game is over */

    do {
      nextToken(fin, buf);


      if(!strcmp(buf, "1-0") ||
         !strcmp(buf, "0-1") ||
         !strcmp(buf, "1/2-1/2")) {
        strcpy(result, "OVER");
        return 0;
      }
      
    if(feof(fin) || buf[0] == '\0') 
    {        
        strcpy(result, "OVER");
        return -1;
      }

    } while(buf[0] == '[' || buf[0] == '{');

    strcpy(result, "OVER");

    sscanf(buf, "%i%c", &i, &ch);

    if(toupper(ch) == toupper(game)) 
    {

      nextToken(fin, result);

      return 0;
    }

    
  }
}

/* Function: testbpgn
 * Input:    the arguments Sunsetter was called with 
 * Output:   0, -1 if an error occured
 * Purpose:  to go through a .bpgn file and suggest moves for testing or blundercheck.
 */

int testbpgn(int argc, char **argv)
{
 char buf[MAX_STRING]; 
 int skipmoves; // this is non-zero if we only want to look at one specific move
 color skipcolor = WHITE; // the color to Move if we want to look at a specific move
 FILE *fin;


 move m;

 if ((argc < 3) || (argc > 5)) {   // there was no input .bpgn specified or too many args
    output("Usage:\n");
    output("sunsetter test <input file> <positionNr*> <Color*>  * = optional");
    return (1);
    }
 

fin = fopen(argv[2], "rt");   // open the input file




skipmoves = 0; 

if (argc == 5) { skipmoves = atoi(argv[3]); if (!strcmp(argv[4],"BLACK")) {skipcolor = BLACK;} else {skipcolor = WHITE;}  } 

for(;;)    // go through the whole game

{

	if (gameBoard.getColorOnMove() == WHITE) { sprintf (buf, "%d.", (gameBoard.getMoveNum() / 2)+1 ); output (buf); }

	if (nextMoveOrResult(fin, 'A', buf)) {
          output("  EOF \n\n");
          break;
          }

	if (strcmp(buf, "OVER"))
	{
	
    m = gameBoard.algebraicMoveToDBMove(buf);

    if(gameBoard.playMove(m, 0)) {
		  output("Illegal move in file: ");
		  output(buf); 
		  output("\n"); 
          break;
        }
	}
	
    if(!strcmp(buf, "OVER") || (gameBoard.isCheckmate())) {
          sprintf(buf,"\n\n");output(buf); 
          sprintf(buf," Searches overall: %ld\n", stats_overallsearches);output(buf);
		  sprintf(buf,"QSearches overall: %ld\n", stats_overallqsearches);output(buf);
		  sprintf(buf,"ClockTicks       : %ld\n", stats_overallticks);output(buf);
		  sprintf(buf,"mNPS             : %ld\n\n", (stats_overallsearches+stats_overallqsearches) / (stats_overallticks +1)); output(buf);

#ifdef DEBUG_STATS		  
		  sprintf(buf,"Debug A: %d\n", stats_overallDebugA);output(buf);
		  sprintf(buf,"Debug B: %d\n", stats_overallDebugB);output(buf);		  
#endif 		  		  
		  break;
        }	
	


	
	output (buf);
	output (" ");

	gameBoard.setDeepBugColor(gameBoard.getColorOnMove());
	// make sure Sunsetter always thinks it is playing the color to move next

	if ((skipmoves == 0) || ((skipmoves == (((gameBoard.getMoveNum()-1) /2)+1)) && (gameBoard.getColorOnMove() == skipcolor)) ) 
	// search all moves or only the one given as 4th argument

	{
		zapHashValues(); 
		output ("\n\n"); 
		findMove(&m);      // do the search.
	//	Currently Sunsetter takes as much time as if it
	//  would have InitialTime left.



    DBMoveToRawAlgebraicMove(m,buf);
    output ("Sunsetter suggests:"); 
	output (buf); 
	output ("\n\n");


    }

    }
    fclose(fin);


    return(0);        
 }

/* Function: speedtest
 * Input:    the arguments Sunsetter was called with 
 * Output:   0, -1 if an error occured
 * Purpose:  use all positions in the file for make/movegen/eval speedtest
 */

int speedtest(int argc, char **argv)
{
	char buf[MAX_STRING], buf2[MAX_STRING]; 
	move gameMoves[MAX_GAME_LENGTH];
	int a, n;
	int makeUnmakeSpeed; 

	FILE *fin; 
	move m;
	move moves[MAX_MOVES]; 

	clock_t startClockTime, endClockTime;

	int movesInGame = 0;

	if ((argc < 2) || (argc > 3)) {   
		output("Usage:\n");
		output("sunsetter speedtest <input file>");
		return (1);
	}
 

	fin = fopen(argv[2], "rt");   // open the input file
	if(fin){
		printf("starting speedtest on file %s\n", argv[2]);
	} else {
		printf("failed to open file %s for speedtest\n", argv[2]);
		return 1;
	}

	for(;;)    // go through the whole game
	{
		if (nextMoveOrResult(fin, 'A', buf)) 
		{
			output("  EOF \n\n");
			break;
		}

		if (strcmp(buf, "OVER"))
		{
			if (gameBoard.getColorOnMove() == WHITE) 
			{ 
				sprintf (buf2, "%d.",
					(gameBoard.getMoveNum() / 2)+1 ); output (buf2); 
			}
		
			m = gameBoard.algebraicMoveToDBMove(buf);

			if(gameBoard.playMove(m, 0)) 
			{
				output("Illegal move in file: ");
				output(buf); 
				output("\n");
				break;
			}

			movesInGame++;
			gameMoves[movesInGame] = m;

			output (buf);
			output (" ");
		} else break; 
	}

	output ("\n\n Game stored, starting makeUnmake speed test ... \n"); 

	startClockTime = getSysMilliSecs();

	for (a = 0; a < REPEATCOUNT; a++)
	{
		for (n = movesInGame; n >= 1; n--) 
		{
			gameBoard.unchangeBoard(); 		
		}	

		for (n = 1; n <= movesInGame; n++) 
		{
			gameBoard.changeBoard(gameMoves[n]); 				
		}
	}

	endClockTime = getSysMilliSecs();

	makeUnmakeSpeed = (int) (endClockTime - startClockTime);

	sprintf (buf, "%d make/unmake at ", movesInGame * REPEATCOUNT );
	output (buf); 
	sprintf (buf, "%d make/unmake per second. \n",
		(((movesInGame * REPEATCOUNT) / (makeUnmakeSpeed)) * 1000 ) ); 
	output (buf); 

	output ("\n\n starting eval() speed test ... \n"); 
	startClockTime = getSysMilliSecs();

	for (n = movesInGame; n >= 1; n--) 
	{
		gameBoard.unchangeBoard(); 
		for (a = 0; a < REPEATCOUNT; a++) gameBoard.eval(); 
	}	

	for (n = 1; n <= movesInGame; n++) 
	{
		gameBoard.changeBoard(gameMoves[n]); 				
		for (a = 0; a < REPEATCOUNT; a++) gameBoard.eval(); 
	}

	endClockTime = getSysMilliSecs();

	sprintf (buf, "%d eval() at ", movesInGame * REPEATCOUNT *2);
	output (buf); 
	sprintf (buf, "%d eval per second. \n",
	( ((movesInGame * REPEATCOUNT *2) / (((int) (endClockTime - startClockTime)))) * 1000 ) ); 
	output (buf); 

	output ("\n\n starting MoveGen speed test ... \n"); 

	startClockTime = getSysMilliSecs();

	for (n = movesInGame; n >= 1; n--) 
	{
		gameBoard.unchangeBoard(); 
		for (a = 0; a < REPEATCOUNT; a++) AIBoard.aiMoves(moves);
	}	

	for (n = 1; n <= movesInGame; n++) 
	{
		gameBoard.changeBoard(gameMoves[n]); 				
		for (a = 0; a < REPEATCOUNT; a++) AIBoard.aiMoves(moves);
	}

	endClockTime = getSysMilliSecs();

	sprintf (buf, "%d MoveGen at ", movesInGame * REPEATCOUNT *2);
	output (buf); 
	sprintf (buf, "%d MoveGen per second. \n",
		( ((movesInGame * REPEATCOUNT *2) / (((int) (endClockTime - startClockTime)))) * 1000 ) ); 
	output (buf); 

	fclose(fin);

	return(0);        
}

