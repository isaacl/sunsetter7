/* ***************************************************************************
 *                                Sunsetter                                  *
 *				 (c) Ben Dean-Kawamura, Georg v. Zimmermann                  *
 *   For license terms, see the file COPYING that came with this program.    *
 *                                                                           *
 *  Name: transposition.cc                                                   *
 *  Purpose: Has the functions relating to the transposition tables.         *
 *                                                                           *
 *  Comments: The transposition tables are used to get more information      *
 *            duplicate positions.  Duplicate positions can occur in two     *
 *            ways.  Either we got there by two different move orders, or    *
 *            we've searched it while doing a shallower search.  Hopefully   *
 *            when we reached it before we searched it to the same depth as  *
 *            we currently want and it returned an exact value.  If so, then *
 *            we don't have to search the node anymore, we can just return   *
 *            that value.  Probobly that didn't happen, but we can use some  *
 *            of the information we got before, like a good move to try      *
 *            first.                                                         *
 *                                                                           *
 *            The tables are orginized like this:  There are two arrays, one *
 *            for each color.  The arrays are indexed with rightmost bits of *
 *            the hash value for the position (The hash value is a 64 bit    *
 *            value that each position maps to).  The arrays have the        *
 *            leftmost 48 bits of the hash value and at least the 16         *
 *            rightmost bits of the hash value are in the index.             *
 *                                                                           *
 *            Hash values are obtained with the Zobrist algorithm.  For each *
 *            piece and square a random number is generated.  The hash value *
 *            is generated by adding the random numbers for all of the       *
 *            pieces and the squares they're on.  When a move is made the    *
 *            random number for the square the piece came from and moves to  *
 *            is "and"-ed.                                                   *
 *            Also there are random numbers generated for                    *
 *            castling posibilities and what square, if any, en passant can  *
 *            be played on.                                                  *                                                                       *
 *************************************************************************** */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "interface.h"
#include "definitions.h"
#include "board.h"
#include "notation.h"
#include "brain.h"



extern int stats_hashFillingUp;
extern int stats_hashSize; 

qword ssrandom64(void); 

transpositionEntry *lookupTable[COLORS]={NULL,NULL};
transpositionEntry *learnTable[COLORS]={NULL,NULL}; 

duword lookupMask;
duword learnMask; 

/* hashNumbers is an array of random numbers for generating a hash value. */

qword hashNumbers[COLORS][PIECES][64];
qword hashHandNumbers[COLORS][PIECES][17];
qword hashCastleNumbers[COLORS][2];
qword hashEnPassantNumbers[66];  /* 66 to include room for OFF_BOARD */

#ifdef DEBUG_HASH
qword hashNumbersT[COLORS][PIECES][64];
qword hashHandNumbersT[COLORS][PIECES][17];
qword hashCastleNumbersT[COLORS][2];
qword hashEnPassantNumbersT[66];  /* 66 to include room for OFF_BOARD */
#endif

/* Function: ssrandom64
 * Input:    None.
 * Output:   A random 64 bit number.
 * Purpose:  Used for the Zobrist hashing. 
 *
 */

qword ssrandom64(void) {
  
 	return (((qword) (rand() & 0xFFFF)) |         
		    ((qword) (rand() & 0xFFFF) << 16) |   
		    ((qword) (rand() & 0xFFFF) << 32) |   
		    ((qword) (rand() & 0xFFFF) << 48)); 

}



/* Function: saveLearnTableToDisk
 * Input:    None.
 * Output:   None.
 * Purpose:  Used to save the learn table to disk when Sunsetter quits. 
 */


void saveLearnTableToDisk()

{
	
	FILE *learnFile; 
	unsigned int n;
	char fileName[MAX_STRING];


	sprintf (fileName,"ss-%s.bin",VERSION); 

	learnFile = fopen(fileName, "wb");
  
	assert (learnFile);

	for(n = 0; n <= learnMask; n++) 
	{
		fwrite(&learnTable[WHITE][n], sizeof(transpositionEntry), 1, learnFile);	
		fwrite(&learnTable[BLACK][n], sizeof(transpositionEntry), 1, learnFile);	
	}

	fclose(learnFile);

}


/* Function:	readLearnTableFromDisk
 * Input:		None.
 * Output:		None.
 * Purpose:		Used to read the learn table from disk when 
 *				Sunsetter is started and gets the command "learn". 
 */

void readLearnTableFromDisk()

{

	FILE *learnFile; 
	unsigned int n;
	char buf[MAX_STRING]; 

#ifdef DEBUG_LEARN

	int highestValue = 0; 
	int numberValue = 0; 

#endif

	sprintf (buf,"ss-%s.bin",VERSION); 
	learnFile = fopen(buf, "rb");

 
	if(!learnFile) 
	{
		saveLearnTableToDisk(); 
		output ("Created empty learn file.\n\n"); 
		learnFile = fopen(buf, "rb");
	}

	for(n = 0; n <= learnMask; n++) 
	{
		fread(&learnTable[WHITE][n], sizeof(transpositionEntry), 1, learnFile);	
		fread(&learnTable[BLACK][n], sizeof(transpositionEntry), 1, learnFile);	

#ifdef DEBUG_LEARN

		if (learnTable[WHITE][n].value) numberValue++; 
		if (learnTable[BLACK][n].value) numberValue++; 

		if (abs(learnTable[WHITE][n].value) > abs(highestValue)) highestValue = learnTable[WHITE][n].value; 
		if (abs(learnTable[BLACK][n].value) > abs(highestValue)) highestValue = learnTable[BLACK][n].value; 

#endif

	}

#ifdef DEBUG_LEARN

	sprintf (buf,"Learn: Learned positions: %d \n",numberValue); output (buf); 
	sprintf (buf,"Learn: Highest value    : %d \n\n",highestValue); output (buf); 

#endif

	fclose(learnFile);

}



/* Function: makeTranspositionTable
 * Input:    the size for the table.
 * Output:   0 if successfull, -1 if not.
 * Purpose:  Used to create the transposition table and learn table
 */

int makeTranspositionTable(unsigned int size)
{
  unsigned int learnSize = LEARN_SIZE; 
  unsigned int logOfSize, n;
  char buf[MAX_STRING];

  if(lookupTable[WHITE] != NULL) free(lookupTable[WHITE]);
  if(lookupTable[BLACK] != NULL) free(lookupTable[BLACK]);

  if(learnTable[WHITE] != NULL) free(learnTable[WHITE]);
  if(learnTable[BLACK] != NULL) free(learnTable[BLACK]);

  /* Am I reading the wrong standard C library specification or is microsoft?
     Acording to mine I shouldn't have to do that. */

  if(size < MIN_HASH_SIZE) {
    sprintf(buf, "The transposition table size must be > %d bytes\n",
	    (int)MIN_HASH_SIZE);
    output(buf);
    lookupTable[WHITE] = lookupTable[BLACK] = NULL;
    return -1;
  }

  size /= sizeof(transpositionEntry) * 2;

  logOfSize = 1;
  for(n = 2; n < size; n *= 2) logOfSize++;
  if(n != size) logOfSize--;
  lookupMask = (1 << logOfSize) - 1;
  size = 1 << logOfSize;

  learnSize /= sizeof(transpositionEntry) * 2;

  logOfSize = 1;
  for(n = 2; n < learnSize; n *= 2) logOfSize++;
  if(n != learnSize) logOfSize--;
  learnMask = (1 << logOfSize) - 1;
  learnSize = 1 << logOfSize;


  learnTable[WHITE] = 
    (transpositionEntry *) calloc(learnSize, sizeof(transpositionEntry));
  learnTable[BLACK] = 
    (transpositionEntry *) calloc(learnSize, sizeof(transpositionEntry));
    
  lookupTable[WHITE] = 
    (transpositionEntry *) calloc(size, sizeof(transpositionEntry));
  lookupTable[BLACK] = 
    (transpositionEntry *) calloc(size, sizeof(transpositionEntry));
  
  if(!lookupTable[WHITE] || !lookupTable[BLACK] ||
	 !learnTable[WHITE] || !learnTable[BLACK]) 
  {
    output("Not enough memory to make the transposition tables\n");
    free(lookupTable[WHITE]);
    free(lookupTable[BLACK]);
    lookupTable[WHITE] = lookupTable[BLACK] = NULL;
	free(learnTable[WHITE]);
    free(learnTable[BLACK]);
    learnTable[WHITE] = learnTable[BLACK] = NULL;
    lookupMask = 0;
	learnMask = 0; 
    return -1;
  }

  for(n = 0; n < size; n++) 
  {
    lookupTable[WHITE][n].hash =  qword(0);
    lookupTable[BLACK][n].hash =  qword(0);

#ifdef DEBUG_HASH
	lookupTable[WHITE][n].hashT =  qword(0);
    lookupTable[BLACK][n].hashT =  qword(0);
#endif  
  }

  for(n = 0; n < learnSize; n++) 
  {
  	learnTable[WHITE][n].hash =  qword(0);
    learnTable[BLACK][n].hash =  qword(0);
	learnTable[WHITE][n].value =  0;
    learnTable[BLACK][n].value =  0;


  }
  

  sprintf(buf, "Created %d byte transposition table and %d byte learn table.\n\n", 
	  (int)(size * sizeof(transpositionEntry) * 2), (int)(learnSize * sizeof(transpositionEntry) * 2));
  
  output(buf);
  stats_hashSize = size; 
  return 0;
}



/* Function: initHash
 * Input:    None.
 * Output:   None.
 * Purpose:  Used to give a position a hash value.  From then on the value is
 *           updated after a move by addToHash() 
 */

void boardStruct::initHash()
{
  color c;
  piece p;
  int n;
  bitboard bb;
  square sq;
  static int generatedNumbers = 0;


  if(!generatedNumbers) {

	
	 /* Use one seed, else the simple learning can't work. */ 
	  
	 srand(80180);
    
	 /* Generate the random numbers in the hash tables */
    

    for(c = FIRST_COLOR; c <= LAST_COLOR; c = (color) (c + 1)) 
	{
      for(p = FIRST_PIECE; p <= LAST_PIECE; p = (piece) (p + 1)) 
	  {
		for(n = 0; n < 64; n++)
		{
		hashNumbers[c][p][n] = ssrandom64();

#ifdef DEBUG_HASH
		hashNumbersT[c][p][n] = ssrandom64();
#endif
		}
      }
    }
	
	for(c = FIRST_COLOR; c <= LAST_COLOR; c = (color) (c + 1)) 
	{
      for(p = FIRST_PIECE; p <= LAST_PIECE; p = (piece) (p + 1)) 
	  {
		for(n = 0; n < 17; n++)
		{
		hashHandNumbers[c][p][n] = ssrandom64();

#ifdef DEBUG_HASH
		hashHandNumbersT[c][p][n] = ssrandom64();
#endif
		}
      }
    }
    
	for(n = 0; n < 66; n++) 
	{
		hashEnPassantNumbers[n] = ssrandom64();

#ifdef DEBUG_HASH
		hashEnPassantNumbersT[n] = ssrandom64();
#endif
	}
    
	
	hashCastleNumbers[WHITE][KING_SIDE] = ssrandom64();
    hashCastleNumbers[WHITE][QUEEN_SIDE] = ssrandom64();
    hashCastleNumbers[BLACK][KING_SIDE] = ssrandom64();
    hashCastleNumbers[BLACK][QUEEN_SIDE] = ssrandom64();

#ifdef DEBUG_HASH
	hashCastleNumbersT[WHITE][KING_SIDE] = ssrandom64();
    hashCastleNumbersT[WHITE][QUEEN_SIDE] = ssrandom64();
    hashCastleNumbersT[BLACK][KING_SIDE] = ssrandom64();
    hashCastleNumbersT[BLACK][QUEEN_SIDE] = ssrandom64();
#endif
 

	srand(time(NULL));
  }

  /* Make a hash value for the position */

  hashValue = qword(0);

#ifdef DEBUG_HASH
  hashValueT = qword(0); 
#endif

  bb = occupied[WHITE];
  while(bb.hasBits()) {
    sq = firstSquare(bb.data);
    bb.unsetSquare(sq);
    hashValue += hashNumbers[WHITE][position[sq]][sq];

#ifdef DEBUG_HASH
	hashValueT += hashNumbersT[WHITE][position[sq]][sq];
#endif
  }

  bb = occupied[BLACK];
  while(bb.hasBits()) {
    sq = firstSquare(bb.data);
    bb.unsetSquare(sq);
    hashValue += hashNumbers[BLACK][position[sq]][sq];

#ifdef DEBUG_HASH
	hashValueT += hashNumbersT[BLACK][position[sq]][sq];
#endif
  }

  hashValue ^=
    hashCastleNumbers[WHITE][KING_SIDE];
  hashValue ^=
    hashCastleNumbers[WHITE][QUEEN_SIDE];
  hashValue ^= 
    hashCastleNumbers[BLACK][KING_SIDE];
  hashValue ^=
    hashCastleNumbers[BLACK][QUEEN_SIDE];
  
#ifdef DEBUG_HASH
  hashValueT ^=
    hashCastleNumbersT[WHITE][KING_SIDE];
  hashValueT ^=
    hashCastleNumbersT[WHITE][QUEEN_SIDE];
  hashValueT ^= 
    hashCastleNumbersT[BLACK][KING_SIDE];
  hashValueT ^=
    hashCastleNumbersT[BLACK][QUEEN_SIDE];
#endif

}



/* Function: zapHashValues
 * Input:    None.
 * Output:   None.
 * Purpose:  Makes all of the hash values useless, use this because something
 *           changed the way Sunsetter values positions or maybe
 *           because the rules have changed (crazyhouse to bug).
 */

void zapHashValues()
{
  unsigned int n;
  int moveNrInit;

  moveNrInit = hashMoveCircle -1; 
  if (moveNrInit == -1) moveNrInit = 7; 

assert ((moveNrInit >= 0) && (moveNrInit <= 7));


  for(n = 0; n <= lookupMask; n++) {
    lookupTable[WHITE][n].hash = qword(0);
    lookupTable[BLACK][n].hash = qword(0);

    lookupTable[WHITE][n].moveNr = moveNrInit;
    lookupTable[BLACK][n].moveNr = moveNrInit;

#ifdef DEBUG_HASH
	lookupTable[WHITE][n].hashT = qword(0);
    lookupTable[BLACK][n].hashT = qword(0);
#endif
	
  }
}


/* Function: addToHash
 * Input:    A color, a piece and a square.
 * Output:   None.
 * Purpose:  Used to update a position's hash value.  This function is called
 *           when a piece is added to or substracted from  a square.  It XORS
 *           the board's hashValue with the hash number corresponding
 *           to the piece and square.
 */

void boardStruct::addToHash(color c, piece p, square sq)
{
  hashValue ^= hashNumbers[c][p][sq];

#ifdef DEBUG_HASH
  hashValueT ^= hashNumbersT[c][p][sq];
#endif
}



/* Function: addToInHandHash
 * Input:    A color and a piece
 * Output:   None.
 * Purpose:  This function is called when a piece is added to a players hand.
 */

void boardStruct::addToInHandHash(color c, piece p)
{
    
  hashValue ^= hashHandNumbers[c][p][hand[c][p]];

#ifdef DEBUG_HASH
  hashValueT ^= hashHandNumbersT[c][p][hand[c][p]];
#endif 

}

/* Function: subtractFromInHandHash
 * Input:    A color and a piece
 * Output:   None.
 * Purpose:  This function is called when a piece is removed from a players
 *           hand.
 */

void boardStruct::subtractFromInHandHash(color c, piece p)
{
  
  hashValue ^= hashHandNumbers[c][p][hand[c][p]+1];

#ifdef DEBUG_HASH
  hashValueT ^= hashHandNumbersT[c][p][hand[c][p]+1];
#endif
  
}

/* Function: updateCastleHash
 * Input:    A side of the board and a color 
 * Output:   None.
 * Purpose:  Used to update a position's hash value.  This function is called
 *           when a side's castling options change.  It and-s the
 *           board's hashValue with the hash number corresponding to the side
 *           of the board and color.
 */

void boardStruct::updateCastleHash(color c, int side)
{
  
  hashValue ^= hashCastleNumbers[c][side];
  
#ifdef DEBUG_HASH
  hashValueT ^= hashCastleNumbersT[c][side];
#endif

}

/* Function: addEnPassantHash
 * Input:    A square.
 * Output:   None.
 * Purpose:  Used to update a position's hash value.  This function is called
 *           when the enpassant square changes.  It adds the hash value for
 *           the square that a piece went to.
 */

void boardStruct::addEnPassantHash(square sq)
{
  
  hashValue ^= hashEnPassantNumbers[sq];

#ifdef DEBUG_HASH
  hashValueT ^= hashEnPassantNumbersT[sq];
#endif
  
}


/* Function: store
 * Input:    How deep the search was, what the best move it found was, what it
 *           thought the value of the position was and what kind of value it
 *           was (fail-high, fail-low or exact), deduced from the original
 *			 alpha and beta values at the start of the search() iteration
 *			 store() was called from.
 * Output:   None.
 * Purpose:  Stores the position in the transposition table so that it can
 *           be looked up in the future.
 */

void boardStruct::store(int depthSearched, move bestMove,
			int value, int alpha, int beta)
{ 
  duword offset;


assert (depthSearched <= 128);
assert (depthSearched >= 0);

assert (value <= INFINITY);
assert (value >= -INFINITY);

assert (alpha <= INFINITY);
assert (alpha >= -INFINITY);

assert (beta <= INFINITY);
assert (beta >= -INFINITY);

assert (hashMoveCircle >= 0);
assert  (hashMoveCircle <= 7);


  offset = (duword) (hashValue & lookupMask);  

  
  if (hashMoveCircle != lookupTable[onMove][offset].moveNr) 

  {
	  stats_hashFillingUp++; 
  }
	  
  // we overwrite the hash from a different or same position with the same
  // key if 
  // a) it was an old position or 
  // b) it was now searched deeper or
  // c) we got an exact score, and the position we overwrite doesnt

  // c) is a bit dubious, but only 1% are exact scores, and with a big 
  // hash table we should be ok
  
  if ((hashMoveCircle != lookupTable[onMove][offset].moveNr) 
	  || (depthSearched > lookupTable[onMove][offset].depth) 
	  || ((lookupTable[onMove][offset].type != EXACT) 
	  && (((value < beta) && (value > alpha)) || (value >= MATE) || (value <= -MATE) )))
  
  {
    lookupTable[onMove][offset].hash = hashValue >> 16;

#ifdef DEBUG_HASH 
	lookupTable[onMove][offset].hashT = hashValueT >> 16;
#endif

    if((value < beta) && (value > alpha))
	{
      lookupTable[onMove][offset].type = EXACT;
     
	  lookupTable[onMove][offset].value = sword (value);
	  lookupTable[onMove][offset].depth = byte (depthSearched);
	  lookupTable[onMove][offset].moveNr = byte (hashMoveCircle);
    }
    else if(value >= beta) 
	{      
	  if(value >= MATE) 
	  {
		  lookupTable[onMove][offset].type = EXACT;
		  depthSearched = MAX_SEARCH_DEPTH * ONE_PLY; 
	  }
	  else lookupTable[onMove][offset].type = FAIL_HIGH;

	  lookupTable[onMove][offset].value = sword (value);
	  lookupTable[onMove][offset].depth = byte (depthSearched);
	  lookupTable[onMove][offset].moveNr = byte (hashMoveCircle);
    } 
	else if(value <= alpha) 
	{            
	  if(value <= -MATE) 
	  {  
		  lookupTable[onMove][offset].type = EXACT;
		  depthSearched = MAX_SEARCH_DEPTH * ONE_PLY; 
	  }
	  else lookupTable[onMove][offset].type = FAIL_LOW;

	  lookupTable[onMove][offset].value = sword (value);
	  lookupTable[onMove][offset].depth = byte (depthSearched);
	  lookupTable[onMove][offset].moveNr = byte (hashMoveCircle);
    } 

	if(!bestMove.isBad()) 
	{
      lookupTable[onMove][offset].hashMove = bestMove;
    } 
	else 
	{
      lookupTable[onMove][offset].hashMove.makeBad();
    }	
  }
	

  return;
}

/* Function: lookup
 * Input:    None.
 * Output:   The transpositionEntry that corresponds to this position (if any)
 * Purpose:  Used to find if the position was searched before and we "remember"
 *           the result.
 */

transpositionEntry *boardStruct::lookup()
{


  duword offset;
	
  offset = (duword) (hashValue & lookupMask);

  if ((lookupTable[onMove][offset].hash == hashValue >> 16)) { 

	  
#ifdef DEBUG_HASH
	  
	  if (lookupTable[onMove][offset].hashT != hashValueT >> 16)
	  { 
		  debug_allcoll++; 	
	  }

#endif 
	  

	  if (lookupTable[onMove][offset].moveNr != hashMoveCircle) 
	  {
      lookupTable[onMove][offset].moveNr = hashMoveCircle;	
	  }


	  assert (lookupTable[onMove][offset].depth >= 0 );
	  assert (lookupTable[onMove][offset].depth <= 128); 


    return &lookupTable[onMove][offset];
  }
  else return NULL;
}

/* Function: checkLearnTable
 * Input:    None.
 * Output:   a learn value used in findfirstmove() and findmove()
 * Purpose:  Used to find if we remember a lost or won game with that position
 */

int boardStruct::checkLearnTable()
{

  if ((!learning) || (currentRules == BUGHOUSE)) return 0; 

  duword offset;
	
  offset = (duword) (hashValue & learnMask);

  if ((learnTable[onMove][offset].hash == hashValue >> 16)) 
  { 
	
assert (learnTable[onMove][offset].value <= 1000);
assert (learnTable[onMove][offset].value >= -1000); 
  

  return (learnTable[onMove][offset].value / 10 );

  }
  else return 0;
}

/* Function: saveLearnTable
 * Input:    How many points we won (or lost) from this game. 
 * Output:   None.
 * Purpose:  Stores for all the moves in the game a position in the Learn Table. 
 */

void boardStruct::saveLearnTable(int pointsWon)
{ 
  duword offset;
  unsigned int n; 

#ifdef DEBUG_LEARN  
  int highestValue = 0; 
  int numberValue = 0; 
  char buf[MAX_STRING]; 
#endif


/* If we won but the opening resulted in a position Sunsetter didn't like at all, 
   or vv. don't store big values. It might have been a win on time or the like. */

if ( ((firstBigValue > 0) && (pointsWon < 0)) ||
	 ((firstBigValue < 0) && (pointsWon > 0)) )
{	
#ifdef DEBUG_LEARN
	output ("Learn: No convincing win or loss, only light entries.\n");
#endif
	pointsWon /= 3; 

}

/* Now make the position where we got the first big evaluation positiv */ 
if (firstBigValue < 0 ) firstBigValue *= -1; 

/* If our opp lost on time or resigned before we got a big 
   evaluation, store the whole game */
if (!firstBigValue) firstBigValue = getMoveNum(); 

#ifdef DEBUG_LEARN
sprintf (buf, "Learn: We are storing till move %d. : ",firstBigValue);
output (buf);  
#endif

while (!unplayMove())

{

  if (getMoveNum() > firstBigValue) continue; 

  if (getDeepBugColor() != getColorOnMove() )
  {		  

	offset = (duword) (hashValue & learnMask);  

	learnTable[onMove][offset].hash = hashValue >> 16;
	learnTable[onMove][offset].value = (sword) (learnTable[onMove][offset].value + pointsWon) ;

#ifdef DEBUG_LEARN
	sprintf (buf, "%d ,",pointsWon);
	output (buf); 
#endif

	/* Decrease a bit for every move we go back nearer 
	   to the starting position. We want it to try other moves deeper 
	   in the game first, before only playing few opening moves.*/ 
	
	pointsWon -= (pointsWon/(firstBigValue / 2)); 


assert (learnTable[onMove][offset].value <= INFINITY);
assert (learnTable[onMove][offset].value >= -INFINITY);

  }
}	

#ifdef DEBUG_LEARN
output ("<done>\n");  
#endif

/* Make the values age slowly over time, when they become > 100 */

for(n = 0; n <= learnMask; n++) 
{
    
	if (learnTable[WHITE][n].value || learnTable[BLACK][n].value)
	{
	
	learnTable[WHITE][n].value = (sword)( learnTable[WHITE][n].value - (learnTable[WHITE][n].value / 100)); 
	learnTable[BLACK][n].value = (sword)( learnTable[BLACK][n].value - (learnTable[BLACK][n].value / 100));

#ifdef DEBUG_LEARN	
	if (learnTable[WHITE][n].value) numberValue++; 
	if (learnTable[BLACK][n].value) numberValue++; 

	if (abs(learnTable[WHITE][n].value) > abs(highestValue)) highestValue = learnTable[WHITE][n].value; 
	if (abs(learnTable[BLACK][n].value) > abs(highestValue)) highestValue = learnTable[BLACK][n].value; 
#endif

	}
}
#ifdef DEBUG_LEARN
	sprintf (buf,"Learn: Learned positions: %d \n",numberValue); output (buf); 
	sprintf (buf,"Learn: Highest value    : %d \n\n",highestValue); output (buf); 
#endif
  
  return;
}
