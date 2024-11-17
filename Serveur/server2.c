#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server2.h"
#include "client2.h"
#include "oware.h"

static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if(err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

static void app(void)
{
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE];
   /* the index for the array */
   int nClients = 0;
   int nGames = 0;
   int max = sock;
   /* an array for all clients */
   Client* clients = (Client *)malloc(MAX_CLIENTS * sizeof(Client));
   /* an array for all games */
   Game* games = (Game *)malloc(MAX_GAMES * sizeof(Game));

   fd_set rdfs;

   while(1)
   {
      int i = 0;
      FD_ZERO(&rdfs);
      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);
      /* add the connection socket */
      FD_SET(sock, &rdfs);
      /* add socket of each client */
      for(i = 0; i < nClients; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }
      if(select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }
      /* something from standard input : i.e keyboard */
      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* stop process when type on keyboard */
         break;
      }
      /* something from connection socket */
      else if(FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = { 0 };
         socklen_t sinsize = sizeof csin;    // remplacé size_t par socklen_t
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if(csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }
         /* after connecting the client sends its name */
         if(read_client(csock, buffer) == -1)
         {
            /* disconnected */
            continue;
         }
         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;
         FD_SET(csock, &rdfs);

         /* add the client in the clients array */ 
         Client client;
         client.sock = csock;
         client.reviewing = 0;
         client.spectating = 0;
         client.playing = 0;
         client.receiveChallenge = 0;
         client.challenger = INVALID_SOCKET;
         strncpy(client.bio, "This player has no bio", BUF_SIZE - 1);
         strncpy(client.name, buffer, BUF_SIZE - 1);

         clients[nClients] = client;
         nClients++;

         // Welcome message
         write_client(csock, "Welcome to the Oware game server!\n");
         write_client(csock, "You ask for the list of players available by typing '/list'.\n");
         write_client(csock, "You can challenge a player by typing '/challenge <player>'.\n");
         //write_client(csock, "You list all the current games by typing '/games'.\n");
         //write_client(csock, "You can spectate a game by typing '/spectate <gameIndex>'.\n");    
         //write_client(csock, "You can list all the finished games by typing '/fgames'.\n");
         //write_client(csock, "You can rewatch a game by typing '/rewatch <fgameIndex>'.\n");                       
         //write_client(csock, "You can write a bio by typing '/bio <bio>'.\n");
         //write_client(csock, "You can see the bio of a player by typing '/seebio <player>'.\n");
         write_client(csock, "You can exit the server by typing '/exit'.\n");

         /* notify other clients */
         for (int i = 0; i < nClients; i++)
         {
            if (clients[i].sock != csock && nGames == 0)
            {
               write_client(clients[i].sock, client.name);
               write_client(clients[i].sock, " has joined the server!\n");
            }
            else
            {
            for (int j = 0; j < nGames; j++)
               {
                  if (!clients[i].playing && !clients[i].spectating && !clients[i].reviewing && clients[i].sock != csock)
                  {
                     write_client(clients[i].sock, client.name);
                     write_client(clients[i].sock, " has joined the server!\n");
                  }        
               }
            }
         }
      }
      /* a client is talking */
      else
      {
         for(int i = 0; i < nClients; i++)
         {
            /* a client is talking */         
            if(FD_ISSET(clients[i].sock, &rdfs))
            {
               Client* client = &clients[i];

               //// On affiche toutes les informations du client
               printf("========================================\n");
               printf("Client\n");
               printf("Name: %s\n", client->name);
               printf("Playing: %d\n", client->playing);
               printf("Spectating: %d\n", client->spectating);
               printf("Reviewing: %d\n", client->reviewing);
               printf("Receive challenge: %d\n", client->receiveChallenge);
               printf("Challenger: %d\n", client->challenger);
               printf("========================================\n");

               int c = read_client(clients[i].sock, buffer);
               /* client disconnected */
               if(c == 0)
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &nClients);
                  strncpy(buffer, client->name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, client, nClients, buffer, 1);
               }
               else
               {
                  /* Protocole qui gère le client en fonction de ce qu'il a écrit et de ce qu'il fait */

                  // On vérifie si le client a reçu un défi
                  if (client->receiveChallenge)
                  {
                     if (strcmp(buffer, "yes") == 0 || strcmp(buffer, "y") == 0)
                     {
                        client->playing = 1;
                        client->receiveChallenge = 0;
                        
                        // On trouve le client qui a défié
                        Client *challenger;

                        for (int j = 0; j < nClients; j++)
                        {
                           if (clients[j].sock == client->challenger)
                           {
                              challenger = &clients[j];
                              break;
                           }
                        }

                        challenger->playing = 1;

                        // On crée une partie
                        Game game;
                        game.turn = 0;
                        game.finished = 0;
                        game.nSpectators = 0;
                        game.board = (int *)malloc(N_PITS * sizeof(int));
                        game.total_seeds_collected = (int *)malloc(N_PLAYERS * sizeof(int));
                        game.moves = (int *)malloc(MAX_MOVES * sizeof(int));
                        game.spectators = (Client **)malloc(MAX_SPECTATORS * sizeof(Client *));

                        // On initialise le plateau
                        initBoard(game.board, game.total_seeds_collected);

                        // On choisit aléatoirement qui commence
                        int player = (rand() + 1) % 2;

                        // On affecte les joueurs
                        if (player == 0)
                        {
                           game.player1 = challenger;
                           game.player2 = client;
                        }
                        else
                        {
                           game.player1 = client;
                           game.player2 = challenger;
                        }

                        // On ajoute la partie à la liste des parties
                        games[nGames] = game;
                        nGames++;

                        printf("Game started between %s and %s\n", game.player1->name, game.player2->name);
                        printf("Player 1: %s\n", game.player1->name);
                        printf("Player 2: %s\n", game.player2->name);

                        // On affiche le plateau au joueur et aux spectateurs
                        displayBoardToPlayer(&game);
                        displayBoardToSpectator(&game);

                        // On informe les joueurs
                        write_client(game.player1->sock, "Game started!\n");
                        write_client(game.player2->sock, "Game started!\n");

                        // On informe le joueur qui commence
                        write_client(game.player1->sock, "You start!\n");
                        write_client(game.player2->sock, "Your opponent starts!\n");

                        // On demande au joueur de jouer
                        write_client(game.player1->sock, "Choose a pit to play (0-5): ");
                     }
                     else
                     {
                        client->receiveChallenge = 0;
                        write_client(client->challenger, "Your challenge was declined!\n");
                        client->challenger = INVALID_SOCKET;
                        write_client(client->sock, "You declined the game!\n");
                     }
                  }

                  // On cherche si le client est dans une partie
                  else if (client->playing)
                  {
                     // On cherche la partie dans laquelle le client est
                     
                     Game* clientGame;

                     for (int j = 0; j < nGames; j++)
                     {
                        if (games[j].player1->sock == client->sock || games[j].player2->sock == client->sock)
                        {
                           clientGame = &(games[j]);
                           break;
                        }
                     }

                     //// On affiche toutes les informations de la partie
                     printf("========================================\n");
                     printf("Game\n");
                     printf("Player 1: %s\n", clientGame->player1->name);
                     printf("Player 2: %s\n", clientGame->player2->name);
                     printf("Turn: %d\n", clientGame->turn);
                     printf("Finished: %d\n", clientGame->finished);
                     printf("Spectators: %d\n", clientGame->nSpectators);
                     for (int j = 0; j < clientGame->nSpectators; j++)
                     {
                        printf("Spectator %d: %s\n", j, clientGame->spectators[j]->name);
                     }
                     printf("Player to play: %s\n", clientGame->turn % 2 == 0 ? clientGame->player1->name : clientGame->player2->name);
                     for (int j = 0; j < N_PITS; j++)
                     {
                        printf("%d ", clientGame->board[j]);
                     }
                     printf("===========-------------================\n");

                     // On vérifie si c'est le tour du client

                     if ((clientGame->turn % 2 == 0 && clientGame->player1->sock == client->sock) || (clientGame->turn % 2 == 1 && clientGame->player2->sock == client->sock))
                     {
                        // On vérifie si le client a écrit un nombre entre 0 et 5

                        int choice = atoi(buffer);

                        //// On affiche ce que le client a écrit
                        printf("Player %s played in pit %d\n", client->name, choice);
                        printf(clientGame->board[choice + (clientGame->turn % 2 == 0 ? 0 : N_PITS / 2)] == 0 ? "Pit is empty\n" : "Pit is not empty\n");

                        if (choice < 0 || choice >= N_PITS / 2 || clientGame->board[choice + (clientGame->turn % 2 == 0 ? 0 : N_PITS / 2)] == 0)
                        {
                           write_client(client->sock, "Invalid choice, please choose a non-empty pit between 0 and 5: ");
                           break;   // On sort de la boucle for
                        }  

                        // On enregistre le choix du joueur

                        clientGame->moves[clientGame->turn] = choice + (clientGame->turn % 2 == 0 ? 0 : N_PITS / 2);

                        // On joue le coup

                        makeMove(
                           clientGame->board,
                           clientGame->moves[clientGame->turn],
                           clientGame->turn % 2,
                           clientGame->total_seeds_collected
                        );

                        // On vérifie si la partie est terminée

                        if (checkGameEnd(clientGame->board, clientGame->total_seeds_collected))
                        {
                           clientGame->finished = 1;
                           clientGame->player1->playing = 0;
                           clientGame->player2->playing = 0;
                           int winner = getWinner(clientGame->total_seeds_collected);
                           char message[BUF_SIZE];
                           if (winner == 0)
                           {
                              sprintf(message, "Game finished! %s won!\n", clientGame->player1->name);
                           }
                           else if (winner == 1)
                           {
                              sprintf(message, "Game finished! %s won!\n", clientGame->player2->name);
                           }
                           else
                           {
                              sprintf(message, "Game finished! Draw!\n");
                           }
                           write_client(clientGame->player1->sock, message);
                           write_client(clientGame->player2->sock, message);
                           displayBoardToPlayer(clientGame);
                           displayBoardToSpectator(clientGame);

                           //// On affiche toutes les informations de la partie
                           printf("=========----------------===============\n");
                           printf("Game\n");
                           printf("Player 1: %s\n", clientGame->player1->name);
                           printf("Player 2: %s\n", clientGame->player2->name);
                           printf("Turn: %d\n", clientGame->turn);
                           printf("Finished: %d\n", clientGame->finished);
                           printf("Spectators: %d\n", clientGame->nSpectators);
                           for (int j = 0; j < clientGame->nSpectators; j++)
                           {
                              printf("Spectator %d: %s\n", j, clientGame->spectators[j]->name);
                           }
                           printf("Player to play: %s\n", clientGame->turn % 2 == 0 ? clientGame->player1->name : clientGame->player2->name);
                           for (int j = 0; j < N_PITS; j++)
                           {
                              printf("%d ", clientGame->board[j]);
                           }
                           printf("========================================\n");

                           break;
                        }

                        // On affiche le plateau au joueur et aux spectateurs

                        displayBoardToPlayer(clientGame);
                        displayBoardToSpectator(clientGame);

                        // On passe au tour suivant

                        clientGame->turn++;

                        // On informe le joueur adverse

                        write_client(clientGame->turn % 2 == 0 ? clientGame->player1->sock : clientGame->player2->sock, "Your opponent played in pit ");
                        char oppponentMove[BUF_SIZE];
                        sprintf(oppponentMove, "%d", clientGame->moves[clientGame->turn - 1]);
                        write_client(clientGame->turn % 2 == 0 ? clientGame->player1->sock : clientGame->player2->sock, oppponentMove);
                        write_client(clientGame->turn % 2 == 0 ? clientGame->player1->sock : clientGame->player2->sock, "\n");

                        // On affiche le plateau au joueur adverse

                        displayBoardToPlayer(clientGame);
                        displayBoardToSpectator(clientGame);

                        // On demande au joueur de jouer

                        write_client(clientGame->turn % 2 == 0 ? clientGame->player1->sock : clientGame->player2->sock, "Choose a pit to play (0-5): ");

                        break;
                     }
                     else
                     // Si ce n'est pas le tour du client
                     {
                        write_client(client->sock, "It's not your turn!\n");
                     }
                  }

                  else  if (client->spectating)
                  {
                     // On cherche la partie que le client regarde
                     Game* game;
                     for (int j = 0; j < nGames; j++)
                     {
                        if (games[j].spectators[0]->sock == client->sock)
                        {
                           game = &games[j];
                           break;
                        }
                     }
                     // Si il a appuyé sur 'q', on arrête de regarder la partie
                     if (strcmp(buffer, "q") == 0)
                     {
                        client->spectating = 0;
                        game->nSpectators--;
                        for (int j = 0; j < game->nSpectators; j++)
                        {
                           if (game->spectators[j]->sock == client->sock)
                           {
                              memmove(game->spectators + j, game->spectators + j + 1, (game->nSpectators - j - 1) * sizeof(Client *));
                              break;
                           }
                        }
                        write_client(client->sock, "You stopped spectating the game!\n");
                        break;
                     }

                     // Si il a fait /chat <message>, on envoie le message à tous les spectateurs et aux joueurs
                     if (strncmp(buffer, "/chat", 5) == 0)
                     {
                        char *message = buffer + 6;
                        for (int j = 0; j < game->nSpectators; j++)
                        {
                           write_client(game->spectators[j]->sock, client->name);
                           write_client(game->spectators[j]->sock, ": ");
                           write_client(game->spectators[j]->sock, message);
                        }
                        write_client(game->player1->sock, client->name);
                        write_client(game->player1->sock, ": ");
                        write_client(game->player1->sock, message);
                        write_client(game->player2->sock, client->name);
                        write_client(game->player2->sock, ": ");
                        write_client(game->player2->sock, message);
                     }

                     // Sinon, on ignore le message
                  }

                  else if (client->reviewing)
                  {
                     // Si il a appuyé sur 'q', on arrête de regarder la partie
                     
                     if (strcmp(buffer, "q") == 0)
                     {
                        client->reviewing = 0;
                        write_client(client->sock, "You stopped reviewing the game!\n");
                        break;
                     }
                  }
   
                  /* list all players */
                  else if (strcmp(buffer, "/list") == 0) 
                  {
                     write_client(client->sock, "List of players:\n");
                     for (int j = 0; j < nClients; j++)
                     {
                        write_client(client->sock, clients[j].name);
                        write_client(client->sock, "\n");
                     }
                  }
                  
                  /* challenge a player */
                  else if (strncmp(buffer, "/challenge", 10) == 0)
                  {
                     char *player = buffer + 11;

                     for (int j = 0; j < nClients; j++)
                     {
                        if (strcmp(clients[j].name, player) == 0)
                        {
                           if (!clients[j].playing && !clients[j].spectating && !clients[j].reviewing && !clients[j].receiveChallenge)
                           {
                              write_client(client->sock, "You challenged ");
                              write_client(client->sock, player);
                              write_client(client->sock, "\n");

                              clients[j].receiveChallenge = 1;
                              clients[j].challenger = client->sock;

                              write_client(clients[j].sock, "You have been challenged by ");
                              write_client(clients[j].sock, client->name);
                              write_client(clients[j].sock, "\n");
                              write_client(clients[j].sock, "Do you accept? (yes/no)\n");                             
                           }
                           else
                           {
                              write_client(client->sock, "This player is not available for a challenge!\n");
                           }
                           break;
                        }                      
                     }
                  }
                  
                  /* list all finished games */
                  else if (strcmp(buffer, "/fgames") == 0)
                  {
                     write_client(client->sock, "List of finished games:\n");
                     for (int j = 0; j < nGames; j++)
                     {
                        if (games[j].finished)
                        {
                           char index[BUF_SIZE];
                           sprintf(index, "%d", j);
                           write_client(client->sock, index);
                           write_client(client->sock, " : ");
                           write_client(client->sock, games[j].player1->name);
                           write_client(client->sock, " vs ");
                           write_client(client->sock, games[j].player2->name);
                           write_client(client->sock, "\n");
                        }
                     }
                  }
                  
                  /* rewatch a game */
                  else if (strncmp(buffer, "/replay", 7) == 0)
                  {
                     char *gameIndex = buffer + 8;
                     Game *game = &games[atoi(gameIndex)];
                     if (game->finished)
                     {
                        write_client(client->sock, "You are replaying ");
                        write_client(client->sock, game->player1->name);
                        write_client(client->sock, " VS ");
                        write_client(client->sock, game->player2->name);
                        write_client(client->sock, "\n");
                        write_client(client->sock, "Type 'q' to stop replay\n");
                        replay(client, game);
                     }
                  } 

                  /* list all unfinished games */
                  else if (strcmp(buffer, "/games") == 0)
                  {
                     write_client(client->sock, "List of games:\n");
                     for (int j = 0; j < nGames; j++)
                     {
                        if (!games[j].finished)
                        {
                           char index[BUF_SIZE];
                           sprintf(index, "%d", j);
                           write_client(client->sock, index);
                           write_client(client->sock, " : ");
                           write_client(client->sock, games[j].player1->name);
                           write_client(client->sock, " vs ");
                           write_client(client->sock, games[j].player2->name);
                           write_client(client->sock, "\n");
                        }
                     }
                  }
                  
                  /* spectate a game */
                  else if (strncmp(buffer, "/spectate", 9) == 0)
                  {
                     char *gameIndex = buffer + 10;
                     Game *game = &games[atoi(gameIndex)];
                     if (!game->finished)
                     {
                        write_client(client->sock, "You are spectating ");
                        write_client(client->sock, game->player1->name);
                        write_client(client->sock, " VS ");
                        write_client(client->sock, game->player2->name);
                        write_client(client->sock, "\n");
                        write_client(client->sock, "Type 'q' to stop spectating\n");
                        game->spectators[game->nSpectators] = client;
                        game->nSpectators++;
                        client->spectating = 1;
                     }
                  }

                  /* write a bio */
                  else if (strncmp(buffer, "/bio", 4) == 0)
                  {
                     char *bio = buffer + 5;
                     strncpy(client->bio, bio, BUF_SIZE - 1);
                     write_client(client->sock, "Bio updated!\n");
                  }

                  /* see the bio of a player */
                  else if (strncmp(buffer, "/seebio", 7) == 0)
                  {
                     char *player = buffer + 8;
                     for (int j = 0; j < nClients; j++)
                     {
                        if (strcmp(clients[j].name, player) == 0)
                        {
                           printf("Bio of %s : %s\n", player, clients[j].bio);
                           write_client(client->sock, "Bio of ");
                           write_client(client->sock, player);
                           write_client(client->sock, " :\n");
                           write_client(client->sock, clients[j].bio);
                           write_client(client->sock, "\n");
                           break;
                        }
                     }
                  }

                  /* exit */
                  else if (strcmp(buffer, "/exit") == 0)
                  {
                     write_client(client->sock, "Goodbye!\n");
                     closesocket(client->sock);
                     remove_client(clients, i, &nClients);
                  }
                  else
                  {
                     /* message for all */
                     send_message_to_all_clients(clients, client, nClients, buffer, 0);
                  }               
               }
               break;
            }
         }
      }
   }

   clear_clients(clients, nClients);
   end_connection(sock);
}

static void clear_clients(Client *clients, int nClients)
{
   int i = 0;
   for(i = 0; i < nClients; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void remove_client(Client *clients, int to_remove, int *nClients)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*nClients - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*nClients)--;
}

static void send_message_to_all_clients(Client *clients, Client *sender, int nClients, const char *buffer, char from_server)
{
   for (int i = 0; i < nClients; i++)
   {
      if (clients[i].sock != sender->sock && !clients[i].playing && !clients[i].spectating && !clients[i].reviewing && !clients[i].receiveChallenge)
      {
         write_client(clients[i].sock, from_server ? "Server: " : "");
         write_client(clients[i].sock, sender->name);
         write_client(clients[i].sock, ": ");
         write_client(clients[i].sock, buffer);
      }
   }
}

static int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if(bind(sock,(SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR) 
   {
      perror("bind()");   
      exit(errno);
   }

   if(listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

static void displayBoardToPlayer(Game *game)
{
   char display[BUF_SIZE * 4]; // Un grand buffer pour accumuler l'affichage
   int offset = 0; // Position d'écriture dans le buffer

   // Si on affiche pour le joueur 1, on affiche le board normalement

   if (game->turn % 2 == 0)
   {
      for (int i = N_PITS - 1; i >= N_PITS / 2; i--)
      {
         offset += sprintf(display + offset, "%d ", game->board[i]);
      }
      offset += sprintf(display + offset, "\n");
      for (int i = 0; i < N_PITS / 2; i++)
      {
         offset += sprintf(display + offset, "%d ", game->board[i]);
      }
      offset += sprintf(display + offset, "\n");

      //Affichage des coordonnées des cases
      for (int i = 0; i >= N_PITS - 1; i++)
      {
         offset += sprintf(display + offset, "%d ", i);
      }
      offset += sprintf(display + offset, "\n");

      // Affichage des graines collectées
      offset += sprintf(display + offset, "Seeds collected by you : %d\n", game->total_seeds_collected[0]);
      offset += sprintf(display + offset, "Seeds collected by %s : %d\n", game->player2->name, game->total_seeds_collected[1]);
   }
   // Si on affiche pour le joueur 2, on affiche le board à l'envers
   else
   {

      for (int i = N_PITS / 2 - 1; i >= 0; i--)
      {
         offset += sprintf(display + offset, "%d ", game->board[i]);
      }
      offset += sprintf(display + offset, "\n");
      for (int i = N_PITS / 2; i < N_PITS; i++)
      {
         offset += sprintf(display + offset, "%d ", game->board[i]);
      }
      offset += sprintf(display + offset, "\n");

      //Affichage des coordonnées des cases
      for (int i = 0; i >= N_PITS / 2; i++)
      {
         offset += sprintf(display + offset, "%d ", i);
      }
      offset += sprintf(display + offset, "\n");

      // Affichage des graines collectées

      offset += sprintf(display + offset, "Seeds collected by %s : %d\n", game->player1->name, game->total_seeds_collected[0]);
      offset += sprintf(display + offset, "Seeds collected by you : %d\n", game->total_seeds_collected[1]);
   }
   
   // Envoi du buffer au client
   write_client(game->turn % 2 == 0 ? game->player1->sock : game->player2->sock, display);
}

static void displayBoardToSpectator(Game *game)
{
   char display[BUF_SIZE * 4]; // Un grand buffer pour accumuler l'affichage
   int offset = 0; // Position d'écriture dans le buffer

   // Affichage de qui a jouer et quoi

   offset += sprintf(display + offset, "Last move: ");
   if (game->turn > 0)
   {
      offset += sprintf(display + offset, "%s played in pit %d\n", game->turn % 2 == 0 ? game->player2->name : game->player1->name, game->moves[game->turn - 1] + game->turn % 2 == 0 ? 0 : N_PITS / 2);
   }

   // Affichage du board

   for (int i = N_PITS - 1; i >= N_PITS / 2; i--)
   {
      offset += sprintf(display + offset, "%d ", game->board[i]);
   }
   offset += sprintf(display + offset, "\n");
   for (int i = 0; i < N_PITS / 2; i++)
   {
      offset += sprintf(display + offset, "%d ", game->board[i]);
   }
   offset += sprintf(display + offset, "\n");

   //Affichage des coordonnées des cases
   for (int i = 0; i >= N_PITS - 1; i++)
   {
      offset += sprintf(display + offset, "%d ", i);
   }
   offset += sprintf(display + offset, "\n");

   // Affichage des graines collectées
   offset += sprintf(display + offset, "Seeds collected by %s : %d\n", game->player1->name, game->total_seeds_collected[0]);
   offset += sprintf(display + offset, "Seeds collected by %s : %d\n", game->player2->name, game->total_seeds_collected[1]);

   // Envoi du buffer aux spectateurs
   for (int i = 0; i < game->nSpectators; i++)
   {
      write_client(game->spectators[i]->sock, display);
   }
}

static void replay(Client *client, Game *game)
{
   // Affiche les moves joué dans la partie
   for (int i = 0; i < game->turn; i++)
   {
      // On crée un plateau pour le replay
      int board[N_PITS];
      int total_seeds_collected[N_PLAYERS];
      
      // On initialise le plateau
      initBoard(board, total_seeds_collected);

      // On joue les moves
      for (int j = 0; j <= i; j++)
      {
         makeMove(
            board,
            game->moves[j],
            j % 2,
            total_seeds_collected
         );
      }

      // On affiche le plateau
      char display[BUF_SIZE * 4]; // Un grand buffer pour accumuler l'affichage
      int offset = 0; // Position d'écriture dans le buffer

      // Affichage de qui a jouer et quoi

      offset += sprintf(display + offset, "Last move: ");
      if (i > 0)
      {
         offset += sprintf(display + offset, "%s played in pit %d\n", i % 2 == 0 ? game->player2->name : game->player1->name, game->moves[i - 1] + i % 2 == 0 ? 0 : N_PITS / 2);
      }

      // Affichage du board

      for (int j = N_PITS - 1; j >= N_PITS / 2; j--)
      {
         offset += sprintf(display + offset, "%d ", board[j]);
      }
      offset += sprintf(display + offset, "\n");
      for (int j = 0; j < N_PITS / 2; j++)
      {
         offset += sprintf(display + offset, "%d ", board[j]);
      }
      offset += sprintf(display + offset, "\n");

      //Affichage des coordonnées des cases
      for (int j = 0; j >= N_PITS - 1; j++)
      {
         offset += sprintf(display + offset, "%d ", j);
      }
      offset += sprintf(display + offset, "\n");
      
      // Affichage des graines collectées

      offset += sprintf(display + offset, "Seeds collected by %s : %d\n", game->player1->name, total_seeds_collected[0]);
      offset += sprintf(display + offset, "Seeds collected by %s : %d\n", game->player2->name, total_seeds_collected[1]);

      // Envoi du buffer au client

      write_client(client->sock, display);

   }
}

int main()
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
