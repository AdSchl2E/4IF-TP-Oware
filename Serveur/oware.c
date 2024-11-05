#include <stdio.h>
#include <stdlib.h>

#include "oware.h"


// Fonction principale
int main() {
    int board[N_PITS];
    int total_seeds_collected[N_PLAYERS];
    int player = 0;
    int choice = 0;
    int winner = 0;

    init(board, total_seeds_collected);
    displayBoard(board, total_seeds_collected); // Send message to all clients

    while (!checkGameEnd(board, total_seeds_collected)) {
        player = player % N_PLAYERS;
        if (player == 0) {
            choice = playerChoice(board, player); // Recieve message from client 1
        } else {
            choice = playerChoice(board, player); // Recieve message from client 2
        }
        int seeds_collected = makeMove(board, choice, player, total_seeds_collected);
        printf("Le joueur %d a joué la case %d et a ramassé %d graines\n", player + 1, choice, seeds_collected); // Send message to all clients
        displayBoard(board, total_seeds_collected); // Send message to all clients
        player++;
    }

    winner = getWinner(total_seeds_collected);
    if (winner == -1) {
        printf("Match nul\n"); // Send message to all clients
    } else {
        printf("Le joueur %d a gagné\n", winner + 1); // Send message to all clients
    }

    return 0;
}

// Initialisation du tableau
void init(int board[], int total_seeds_collected[]) {
    for (int i = 0; i < N_PITS; i++) {
        board[i] = N_SEEDS;
    }
    for (int i = 0; i < N_PLAYERS; i++) {
        total_seeds_collected[i] = 0;
    }
}

// Affichage du tableau
void displayBoard(int board[], int seeds_collected[]) {
    for (int i = N_PITS - 1; i >= N_PITS / 2; i--) {
        printf("%d ", board[i]);
    }
    printf("\n");
    for (int i = 0; i < N_PITS / 2; i++) {
        printf("%d ", board[i]);
    }
    printf("\n");

    printf("Graines capturées par le joueur 1: %d\n", seeds_collected[0]);
    printf("Graines capturées par le joueur 2: %d\n", seeds_collected[1]);
    printf("\n");
}

// Jouer un coup
int makeMove(int board[], int choice, int player, int total_seeds_collected[]) {
    int n_seeds = board[choice];
    int seeds_collected = 0;
    int opponent = 1 - player;
    int seeds_on_board[N_PLAYERS];
    seeds_on_board[0] = 0;
    seeds_on_board[1] = 0;

    // Distribution des graines
    board[choice] = 0;
    while (n_seeds > 0) {
        choice = (choice + 1) % N_PITS;
        if (board[choice] < 12){
            board[choice]++;
            n_seeds--;
        }
    }

    // Grand Slam (coup valide mais ne pas capturer toutes les graines du joueur adverse en cas de grand slam)
    for (int i = 0; i < N_PITS/2; i++) {
        seeds_on_board[0] += board[i];
        seeds_on_board[1] += board[i + N_PITS/2];
    }
    if (seeds_on_board[0] == 0 || seeds_on_board[1] == 0) {
        printf("Grand Slam\n");
        return 0;
    }

    // Vérifier pour capture (en remontant les cases, si elles appartiennent à l’adversaire)
    while ((board[choice] == 2 || board[choice] == 3) && (choice / (N_PITS / 2)) == opponent) {
        seeds_collected += board[choice];
        board[choice] = 0;
        choice = (choice - 1 + N_PITS) % N_PITS;
    }

    total_seeds_collected[player] += seeds_collected;
    return seeds_collected;
}

// Fin de jeu
int checkGameEnd(int board[], int total_seeds_collected[]) {
    // Vérifier si un joueur a gagné en atteignant 25 graines
    if (total_seeds_collected[0] >= WINNING_SEEDS || total_seeds_collected[1] >= WINNING_SEEDS) {
        return 1;
    }

    // Vérifier pour un match nul à 24 graines
    if (total_seeds_collected[0] == NUM_SEEDS_DRAW && total_seeds_collected[1] == NUM_SEEDS_DRAW) {
        return 1;
    }

    // Vérifier s'il reste des graines pour jouer
    int total_seeds = 0;
    for (int i = 0; i < N_PITS; i++) {
        total_seeds += board[i];
    }
    return total_seeds == 0;
}

// Gagnant
int getWinner(int total_seeds_collected[]) {
    if (total_seeds_collected[0] >= WINNING_SEEDS) {
        return 0;
    } else if (total_seeds_collected[1] >= WINNING_SEEDS) {
        return 1;
    } else if (total_seeds_collected[0] == NUM_SEEDS_DRAW && total_seeds_collected[1] == NUM_SEEDS_DRAW) {
        return -1;
    }
    return -1; // Par défaut, match nul si la fin est atteinte sans gagnant clair
}

// Choix du joueur
int playerChoice(int board[], int player) {
    int choice = 0;

    if (player == 0) {
        printf("Joueur %d, choisissez une case (non vide entre 0 et 5 inclus)\n", player); 
        scanf("%d", &choice);
        while (choice < 0 || choice >= N_PITS / 2 || board[choice] == 0) {
            printf("Choix invalide, veuillez choisir une case non vide entre 0 et 5 inclus\n");
            scanf("%d", &choice);
        }
    } else {
        printf("Joueur %d, choisissez une case (non vide entre 6 et 11 inclus)\n", player);
        scanf("%d", &choice);
        while (choice < N_PITS / 2 || choice >= N_PITS || board[choice] == 0) {
            printf("Choix invalide, veuillez choisir une case non vide entre 6 et 11 inclus\n");
           scanf("%d", &choice);
        }
    }
    return choice;
}

// Choix de l'ordinateur
int computerChoice(int board[]) {
    return randomChoice(board);
}

// Choix aléatoire
int randomChoice(int board[]) {
    int choice;
    do {
        choice = rand() % (N_PITS / 2) + N_PITS / 2;
    } while (board[choice] == 0);
    return choice;
}

