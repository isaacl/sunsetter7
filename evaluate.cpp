/* ***************************************************************************
 *                                Sunsetter                                  *
 *				 (c) Ben Dean-Kawamura, Georg v. Zimmermann                  *
 *   For license terms, see the file COPYING that came with this program.    *
 *                                                                           *
 *  Name: evaluation.cc                                                      *
 *  Purpose: Has the functions to do a static evaluation of the position     *
 *                                                                           *
 *  Comments: The main function this contains is eval().  It outputs a       *
 * numerical value corresponding to how well eval() thinks the color to move *
 * is doing.  The value is in centipawns (100 points equals one pawn).  If   *
 * the side to move is better it will be positive, if the side to move is    *
 * worse it will be  negative.                                               *                                          *
 *************************************************************************** */


#include <stdlib.h>
#include "brain.h"
#include "board.h"
#include "evaluate.h"



#define OFF_MOVE (otherColor(onMove))

int DevelopmentTable[COLORS][PIECES][SQUARES];


/* Function: boardControlEval
 * Input:    None.
 * Output:   int
 * Purpose:  Returns the BoardControl part of the evaluation
 */

int boardStruct::boardControlEval()
{

	int control = 0;
	square sq; 

	for(sq = C1; sq <= G8; sq++) 
	
	{
		control +=  ((attacks[WHITE][sq] - attacks[BLACK][sq])); 
	}


return (control * BC_FACTOR);

}

/* Function: kingSafetyEval
 * Input:    the color to compute King Safety for.
 * Output:   int
 * Purpose:  Returns the King Safety part of the evaluation
 */

int boardStruct::kingSafetyEval(color c)
{

	int escapeSquares = 0;
	int takeSquares = 0; 

	int sqcontrol;
	square sq; 
	bitboard nearKing = nearSquares[kingSquare[c]][c];  


	while (nearKing.hasBits())
		{					
			
			sq = firstSquare(nearKing.data);
			nearKing.unsetSquare(sq);  

			sqcontrol = attacks[otherColor(c)][sq] - attacks[c][sq]; 
		
			// Squares we attack more then opp near opps king (excluding his king)
			// or that are empty and as often defended as attacked  

			if ((position[sq] == NONE) && (sqcontrol > -1) ) takeSquares += ( sqcontrol + 2) + takeSquares / 4;
			else if (sqcontrol > -1) takeSquares += ( sqcontrol + 1) + takeSquares / 4;
			else if ((sqcontrol == -1) && (position[sq] == NONE) && (DevelopmentTable[c][KING][sq] + 10 < DevelopmentTable[c][KING][kingSquare[c]]))
				takeSquares += ( sqcontrol + 2) + takeSquares / 4;
			
			// Bonus for escape squares near our own king 
			// squares that are defended by us and not attacked by opp	
			
			if ((!attacks[otherColor(c)][sq]) && (attacks[c][sq] > 1) && (position[sq] == NONE)) escapeSquares+= attacks[c][sq]; 
			// 2 for a normal one, 3 for a better protected one, 4 will be seldom
			
		}


	return ((max (0, takeSquares - escapeSquares)) * NK_FACTOR);
	
	// positive return value is bad for the side we are doing KS for

}

#ifdef GAMETREE


/* Function:	oneSquareBoardControl
 * Input:		for which square to report, what sort of report
 * Output:		int
 * Purpose:		Returns the BoardControl part of the evaluation
 *				for a single square for the html gametree
 *				
 */

int boardStruct::oneSquareBoardControl(square sq, int report)
{

  int tmp = 0;
  int sqcontrol; 
  square c;

  bitboard nearKing; 

	if (report == 1)
	{				
	
		for (c = WHITE; c <= BLACK; c = (color) (c + 1)) 
		{
	
		sqcontrol = attacks[otherColor(c)][sq] - attacks[c][sq]; 

		nearKing = nearSquares[kingSquare[c]][c];  
	
		if (nearKing.squareIsSet(sq))
		{			
			tmp = 3;
			
			// Squares we attack more then opp near opps king (excluding his king)
			// or that are empty and as often defended as attacked  

			if ((position[sq] == NONE) && (sqcontrol > -1) ) tmp = 2;
			else if (sqcontrol > -1) tmp = 2;
			else if ((sqcontrol == -1) && (position[sq] == NONE) && (DevelopmentTable[c][KING][sq] + 10 < DevelopmentTable[c][KING][kingSquare[c]]))
				tmp = 2;
			
			// Bonus for escape squares near our own king 
			// squares that are defended by us and not attacked by opp	
			
			if ((!attacks[otherColor(c)][sq]) && (attacks[c][sq] > 1) && (position[sq] == NONE)) tmp = 1; 
			// 2 for a normal one, 3 for a better protected one, 4 will be seldom
			
		}
		}
	

		return tmp;
	}

	else
	{
		if ((sq >= C1) && (sq <= G8))
			return (attacks[WHITE][sq] - attacks[BLACK][sq]); 
		else return 99;
	}

}


/* Function: getPieceDevOnSquare
 * Input:    a color, a square, a piece.
 * Output:   what this piece gets us on this square 
 * Purpose:  used in printHTML
 */

int boardStruct::getPieceDevOnSquare(color c, square sq, piece p)

{

	return DevelopmentTable[c][p][sq]; 

}


#endif  // GAMETREE


/* Function: eval
 * Input:    None.
 * Output:   The value of the position.
 * Purpose:  Returns a static evaluation of the position
 */

int boardStruct::eval()

{

  if (onMove == WHITE) // it is whites turn NOW. 
  {
    return( material   +  development + boardControlEval() - kingSafetyEval(WHITE) * getMaterialInHand(BLACK)+ kingSafetyEval(BLACK) * getMaterialInHand(WHITE) + bughouseSitForEval());
  }
  else // blacks turn
  { 
    return( -( material + development  + boardControlEval() - kingSafetyEval(WHITE) * getMaterialInHand(BLACK)+ kingSafetyEval(BLACK) * getMaterialInHand(WHITE)+ bughouseSitForEval()));
  }

 
}

/* Function:	bughouseMateEval
 * Input:		None.
 * Output:		integer value.
 * Purpose:		used for mate value in search in bughouse. Make it prefer mates where it 
 *				does not have to wait for a piece. 
 */

int boardStruct::bughouseMateEval()

{ 
  if ((currentRules == CRAZYHOUSE) || (partsitting) || (parttoldgo)) return 0; 
  
  int bieval = 0;
  
  if (getPieceInHand(OFF_MOVE, ROOK) < 1 ) bieval+=pValue[ROOK];
  if (getPieceInHand(OFF_MOVE, KNIGHT) < 1 ) bieval+=pValue[KNIGHT];
  if (getPieceInHand(OFF_MOVE, PAWN) < 1 ) bieval+=pValue[PAWN];
  

  return (bieval);
}


/* Function: bughouseSitForEval
 * Input: None
 * Output: A Value, the Malus for Pieces used we would have to wait for in bughouse
 */
 
int boardStruct::bughouseSitForEval()

{ 

  if ((currentRules == CRAZYHOUSE) || (partsitting) || (parttoldgo)) return 0;
	
  int bpeval = 0;   
  
  if (getPieceInHand(BLACK, ROOK) < 1 ) bpeval+=pValue[ROOK];
  if (getPieceInHand(WHITE, ROOK) < 1 ) bpeval-=pValue[ROOK];
	
  if (getPieceInHand(BLACK, KNIGHT) < 1 ) bpeval+=pValue[KNIGHT];
  if (getPieceInHand(WHITE, KNIGHT) < 1 ) bpeval-=pValue[KNIGHT];

  if (getPieceInHand(BLACK, PAWN) < 1 ) bpeval+=pValue[PAWN];
  if (getPieceInHand(WHITE, PAWN) < 1 ) bpeval-=pValue[PAWN];

  return (bpeval);
}



/* Function: getMaterialInHand
 * Input: Color
 * Output: a material Value of the pieces one side is holding
 * Purpose: used by kingSafetyEval 
 */

int boardStruct::getMaterialInHand(color c)

{
int mInHand = 0;
piece p;

for (p=FIRST_PIECE; p <= LAST_PIECE; p = (piece)(p + 1) ) 
{
	mInHand+= (pValue[p]* hand[c][p]);
}

mInHand /= 80;
mInHand += 2; 

return min(12, mInHand );

}

/* Function: highestAttacked
 * Input:    a square
 * Output:   the highest piece value we attacked with the 
 *			 last move that is not defended or defended less
 * Purpose:  used in search for razoring
 */

// note name is entirely wrong, as is the above description.
// seems to test whether the recently moved piece now attacks something
// that was not attacked before, or attacks that something more times
// than it has defenders.
int boardStruct::highestAttacked(square moveTo)
{
	square sq;
	
	if (takeBackHistory[moveNum-1].captured != NONE) return 1;
	// captured something
		
	if ((attacks[onMove][moveTo] > attacks[OFF_MOVE][moveTo])
		|| ((position[moveTo] == QUEEN) && (attacks[onMove][moveTo]) )) return 0;
	// piece is probably en prise there 
	
	bitboard bb = ( moveAttackedSomething[moveNum] & occupied[onMove] );
	while (bb.hasBits())
	{
		sq = firstSquare(bb.data);
		bb.unsetSquare(sq); 

		if ((!attacks[onMove][sq]) 
			&& (attacks[OFF_MOVE][sq] == 1) 
			&& (pValue[position[sq]] > bestCaptureGain[moveNum-1] + 20))
			return 1;
		// we just attacked an undefended piece the first time

		if ((pValue[position[sq]] > pValue[position[moveTo]] + bestCaptureGain[moveNum-1] + 20))
			return 1;
		// we just attacked a piece more than it was defended

	}

	return 0; 
		
}

/* Function:	escapingAttack(square movedFrom, square moveTo)
 * Input:		a square
 * Output:		boolean as integer
 * Purpose:		razoring in search : if this piece got attacked in the last 
 *				move moving away will not get razored
 */

int boardStruct::escapingAttack(square movedFrom, square moveTo)
{
	bitboard bb;
	square sq;
	
	if ( (moveAttackedSomething[moveNum-1].squareIsSet(movedFrom) )
		&& ((attacks[onMove][movedFrom]-attacks[OFF_MOVE][movedFrom]) 
		>= (attacks[onMove][moveTo]-attacks[OFF_MOVE][moveTo]))) 
		return 1; 
	// escaping attack ? 

	// this will also test moves when the opp captured a piece
	// that defendet mine, to move mine away !
	

	bb = ( moveAttackedSomething[moveNum-1] & moveAttackedSomething[moveNum] & occupied[OFF_MOVE] );
	if (bb.hasBits() 
		&& (!attacks[onMove][moveTo]))
	{
		while (bb.hasBits())
		{
		sq = firstSquare(bb.data);
		bb.unsetSquare(sq); 
		
		if ( (attacks[OFF_MOVE][sq] == 1) 
			&& (attacks[onMove][sq] == 1) 
			&& (pValue[position[sq]] + 20 < pValue[moveHistory[moveNum-1].moved()]))		
			return 1;
		
		}
	}
	// defending a piece ?

	
	return 0; 
		
}


/* Function: captureExtensionCondition
 * Input:    None
 * Output:   int
 * Purpose:  Tells me whether I should extend based on last 2 moves.
 */


int boardStruct::captureExtensionCondition() 
{


	if (	(takeBackHistory[moveNum-1].captured != NONE) && 
			(moveHistory[moveNum-2].to() == moveHistory[moveNum-1].to())  &&
			( (! attacks[OFF_MOVE][moveHistory[moveNum-1].to()]) ||
			(checkHistory[moveNum-1] && takeBackHistory[moveNum-2].captured != NONE) ) ) 
			 return 1;


	return 0;

}



/* Function: standpatCondition
 * Input:    None
 * Output:   int
 * Purpose:  Tells me whether I should allow to stand pat based on the last 2 moves.
 */


int boardStruct::standpatCondition() 
{


	if ((moveHistory[moveNum-1].to() != moveHistory[moveNum-2].to()) || 
		(pValue[takeBackHistory[moveNum-2].captured] > pValue[takeBackHistory[moveNum-1].captured] - 50 ))
			return 1; 

	return 0;

}


/* Function: initializeEval
 * Input:    None
 * Output:   None
 * Purpose:  Sets up the tables that eval() uses.
 */


void initializeEval() 
{

int sq;

for(sq = 0; sq < SQUARES; sq++) 
{

    DevelopmentTable[BLACK][KING][(sq / 8) * ONE_RANK + (sq % 8) * ONE_FILE] =
      (kingDevelopmentTable[sq]*DE_FACTOR);
		
		
	DevelopmentTable[BLACK][PAWN][(sq / 8) * ONE_RANK + (sq % 8) * ONE_FILE] =
     (pawnDevelopmentTable[sq]* DE_FACTOR);


    DevelopmentTable[BLACK][BISHOP][(sq / 8) * ONE_RANK + (sq % 8) * ONE_FILE] =
      (bishopDevelopmentTable[sq]* DE_FACTOR);


    DevelopmentTable[BLACK][ROOK][(sq / 8) * ONE_RANK + (sq % 8) * ONE_FILE] =
      (rookDevelopmentTable[sq]* DE_FACTOR);


    DevelopmentTable[BLACK][KNIGHT][(sq / 8) * ONE_RANK + (sq % 8) * ONE_FILE] =
     (knightDevelopmentTable[sq]* DE_FACTOR);


    DevelopmentTable[BLACK][QUEEN][(sq / 8) * ONE_RANK + (sq % 8) * ONE_FILE] =
      (queenDevelopmentTable[sq]* DE_FACTOR);


	/* Same for white */

	DevelopmentTable[WHITE][KING][(7- sq / 8) * ONE_RANK + (sq % 8) * ONE_FILE]=
      (kingDevelopmentTable[sq]*DE_FACTOR);

	DevelopmentTable[WHITE][PAWN][(7- sq / 8) * ONE_RANK + (sq % 8) * ONE_FILE]=
      (pawnDevelopmentTable[sq]* DE_FACTOR);

	DevelopmentTable[WHITE][BISHOP][(7- sq / 8) * ONE_RANK + (sq % 8) * ONE_FILE]=
      (bishopDevelopmentTable[sq]* DE_FACTOR);

	DevelopmentTable[WHITE][ROOK][(7- sq / 8) * ONE_RANK + (sq % 8) * ONE_FILE]=
      (rookDevelopmentTable[sq]* DE_FACTOR);

	DevelopmentTable[WHITE][KNIGHT][(7- sq / 8) * ONE_RANK + (sq % 8) * ONE_FILE]=
     (knightDevelopmentTable[sq]* DE_FACTOR);

	DevelopmentTable[WHITE][QUEEN][(7- sq / 8) * ONE_RANK + (sq % 8) * ONE_FILE] =
      (queenDevelopmentTable[sq]* DE_FACTOR);

 
}

}
