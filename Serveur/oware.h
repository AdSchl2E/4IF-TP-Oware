#ifndef OWARE_H
#define OWARE_H

#define N_PITS 12
#define N_SEEDS 4
#define N_PLAYERS 2
#define WINNING_SEEDS 25
#define NUM_SEEDS_DRAW 24
#define MAX_MOVES 100

void initBoard(int board[], int total_seeds_collected[]);
void displayBoard(int board[], int seeds_collected[]);
void makeMove(int board[], int choice, int player, int total_seeds_collected[]);
int checkGameEnd(int board[], int total_seeds_collected[]);
int getWinner(int total_seeds_collected[]);
int playerChoice(int board[], int player);
int computerChoice(int board[]);
int randomChoice(int board[]);

#endif /* guard */