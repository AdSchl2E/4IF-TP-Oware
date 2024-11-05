#ifndef OWARE_H
#define OWARE_H

#define N_PITS 12
#define N_SEEDS 4
#define N_PLAYERS 2
#define WINNING_SEEDS 25
#define NUM_SEEDS_DRAW 24

void init(int tab[], int graines_gagnees[]);
void displayBoard(int tab[], int graines_gagnees[]);
int makeMove(int tab[], int choix, int joueur);
int checkGameEnd(int tab[], int graines_gagnees[]);
int getWinner(int graines_gagnees[]);
int playerChoice(int tab[], int joueur);
int computerChoice(int tab[]);
int randomChoice(int tab[]);

#endif /* guard */