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

         Client c = { csock };
         strncpy(c.name, buffer, BUF_SIZE - 1);
         clients[nClients] = c;
         nClients++;

         // Welcome message
         write_client(csock, "Welcome to the Oware game server!\n");
         write_client(csock, "You ask for the list of players available by typing '/list'.\n");
         write_client(csock, "You can challenge a player by typing '/challenge <player>'.\n");
         //write_client(csock, "You list all the current games by typing '/games'.\n");
         //write_client(csock, "You can spectate a game by typing '/spectate <game>'.\n");                           
         //write_client(csock, "You can write a bio by typing '/bio <bio>'.\n");
         //write_client(csock, "You can see the bio of a player by typing '/bio <player>'.\n");

         /* notify other clients */
         snprintf(buffer, BUF_SIZE - 1, "%s joined the server !", c.name);
         buffer[BUF_SIZE - 1] = 0;
         for (i = 0; i < nClients; i++)
         {
            if (clients[i].sock != csock)
            {
               write_client(clients[i].sock, buffer);
            }
         }
      }
      /* a client is talking */
      else
      {
         int clientInGame = 0;
         int i = 0;
         for(i = 0; i < nClients; i++)
         {
            /* a client is talking */
            if(FD_ISSET(clients[i].sock, &rdfs))
            {
               Client client = clients[i];
               printf("Client %s is talking\n", client.name);
               int c = read_client(clients[i].sock, buffer);
               /* client disconnected */
               if(c == 0)
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &nClients);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, client, nClients, buffer, 1);
               }
               else
               {
                  /* si le client qui parle est en jeu, on gere ses coups, sinon on fait les autres commandes */
                  for (int j = 0; j < nGames; j++)
                  {
                     if (!games[j].finished && ( games[j].player1.sock == client.sock || games[j].player2.sock == client.sock ))
                     {
                        clientInGame = 1;
                        printf("clientInGame: %d\n", clientInGame);
                     }

                     printf("turn : %d, turn % 2: %d\n", games[j].turn, games[j].turn % 2);
                     printf("client.name: %s\n", client.name); 
                     printf("playerToPlay: %s\n", (games[j].turn % 2) == 0 ? games[j].player1.name : games[j].player2.name);
                     printf("client.sock: %d\n", client.sock); 
                     printf("(games[j].turn % 2) == 0 ? games[j].player1.sock : games[j].player2.sock : %d\n", (games[j].turn % 2) == 0 ? games[j].player1.sock : games[j].player2.sock);

                     if (!games[j].finished && client.sock == ((games[j].turn % 2) == 0 ? games[j].player1.sock : games[j].player2.sock))
                     {
                        //debug

                        printf("client.sock: %d\n", client.sock); 
                        printf("(games[j].turn % 2) == 0 ? games[j].player1.sock : games[j].player2.sock : %d\n", (games[j].turn % 2) == 0 ? games[j].player1.sock : games[j].player2.sock);

                        printf("playerToPlay: %s\n", (games[j].turn % 2) == 0 ? games[j].player1.name : games[j].player2.name);
                        printf("player1: %s\n", games[j].player1.name);
                        printf("player2: %s\n", games[j].player2.name);

                        int choice = atoi(buffer);
                        // if (choice < 0 || choice >= N_PITS / 2 || games[j].board[choice + games[j].turn % 2 == 0 ? 0 : N_PITS / 2] == 0)
                        // {
                        //    write_client(client.sock, "Invalid choice, please choose a non-empty pit between 0 and 5: ");
                        //    break;
                        // }
                        printf("choice: %d\n", choice);
                        printf("choice: %d\n", choice + ((games[j].turn % 2) * (N_PITS / 2)));  
                        games[j].moves[games[j].turn] = choice + ((games[j].turn % 2) * (N_PITS / 2));

                        makeMove(
                           games[j].board, 
                           games[j].moves[games[j].turn],
                           games[j].turn % 2,
                           games[j].total_seeds_collected
                        );

                        if (checkGameEnd(games[j].board, games[j].total_seeds_collected))
                        {
                           games[j].finished = 1;
                           int winner = getWinner(games[j].total_seeds_collected);
                           if (winner == 0)
                           {
                              write_client(games[j].player1.sock, "You won!\n");
                              write_client(games[j].player2.sock, "You lost!\n");
                           }
                           else if (winner == 1)
                           {
                              write_client(games[j].player1.sock, "You lost!\n");
                              write_client(games[j].player2.sock, "You won!\n");
                           }
                           else
                           {
                              write_client(games[j].player1.sock, "Draw!\n");
                              write_client(games[j].player2.sock, "Draw!\n");
                           }
                           break;
                        }

                        displayBoardToPlayer(games[j]);

                        games[j].turn++;

                        printf("Au tour de %s\n", games[j].turn % 2 == 0 ? games[j].player1.name : games[j].player2.name);

                        write_client(games[j].turn % 2 == 0 ? games[j].player1.sock : games[j].player2.sock, "Your opponent played!\n");
                     
                        displayBoardToPlayer(games[j]);

                        write_client(games[j].turn % 2 == 0 ? games[j].player1.sock : games[j].player2.sock, "Choose a pit to play (0-5): ");
                        break;
                     }
                     else
                     {
                        write_client(client.sock, "It's not your turn!\n");
                     }
                  }

                  if (clientInGame) // Si le client est en jeu, on ne traite pas les autres commandes
                  {
                     break;
                  }

                  /* list all players */
                  if (strcmp(buffer, "/list") == 0) 
                  {
                     write_client(client.sock, "List of players:\n");
                     for (int j = 0; j < nClients; j++)
                     {
                        write_client(client.sock, clients[j].name);
                        write_client(client.sock, "\n");
                     }
                  }
                  /* challenge a player */
                  else if (strncmp(buffer, "/challenge", 9) == 0)
                  {
                     char *player = buffer + 11;
                     for (int j = 0; j < nClients; j++)
                     {
                        if (strcmp(clients[j].name, player) == 0)
                        {
                           write_client(client.sock, "You challenged ");
                           write_client(client.sock, player);
                           write_client(client.sock, "\n");
                           write_client(clients[j].sock, "You have been challenged by ");
                           write_client(clients[j].sock, client.name);
                           write_client(clients[j].sock, "\n");
                           write_client(clients[j].sock, "Do you accept? (yes/no)\n");
                           
                           if (read_client(clients[j].sock, buffer) == 0)
                           {
                              closesocket(clients[j].sock);
                              remove_client(clients, j, &nClients);
                              strncpy(buffer, player, BUF_SIZE - 1);
                              strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                              send_message_to_all_clients(clients, client, nClients, buffer, 1);
                           }
                           else {

                              if (strcmp(buffer, "yes") == 0 || strcmp(buffer, "y") == 0)
                              {

                                 Game game;
                                 game.turn = 0;
                                 game.finished = 0;
                                 game.board = (int *)malloc(N_PITS * sizeof(int));
                                 game.total_seeds_collected = (int *)malloc(N_PLAYERS * sizeof(int));
                                 game.moves = (int *)malloc(MAX_MOVES * sizeof(int));

                                 initBoard(game.board, game.total_seeds_collected);
                                 
                                 // Randomly choose who starts 0 or 1
                                 int player = (rand() + 1) % 2;

                                 printf("player: %d\n", player);

                                 game.player1 = (player == 0) ? client : clients[j];
                                 game.player2 = (player == 1) ? client : clients[j];

                                 displayBoardToPlayer(game);

                                 write_client(client.sock, "Game started!\n");
                                 write_client(clients[j].sock, "Game started!\n");

                                 write_client(game.player1.sock, "You start!\n");
                                 write_client(game.player2.sock, "Your opponent starts!\n");
                                 
                                 write_client(game.player1.sock, "Choose a pit to play (0-5): ");

                                 games[nGames] = game;
                                 nGames++;
                                 
                          
                              //   int draw = 0;

                              //    while (!checkGameEnd(board, total_seeds_collected) && !draw)
                              //    {         
                              //       for (int i = 0; i < N_PLAYERS; i++)
                              //       {
                              //          choice = askForPlayerChoice(board, players[player]);
                              //          printf("draw : %d\n", draw);
                              //          while (choice == -1) 
                              //          {
                              //             write_client(players[player].sock, "You proposed a draw!\n");
                              //             write_client(players[(player + 1) % N_PLAYERS].sock, "Your opponent proposed a draw!\n");
                              //             write_client(players[(player + 1) % N_PLAYERS].sock, "Do you accept? (yes/no)\n");
                              //             read_client(players[(player + 1) % N_PLAYERS].sock, buffer);
                              //             if (strcmp(buffer, "yes") == 0 || strcmp(buffer, "y") == 0)
                              //             {
                              //                draw = 1;
                              //                break;
                              //             }
                              //             else
                              //             {
                              //                write_client(players[player].sock, "Draw declined!\n");
                              //                write_client(players[(player + 1) % N_PLAYERS].sock, "Draw declined!\n");
                              //                choice = askForPlayerChoice(board, players[player]);  
                              //             }
                              //          }
                              //          if (draw)
                              //          {
                              //             break; // il break le for
                              //          }
                              //          makeMove(board, choice + player * N_PITS / 2, player, total_seeds_collected);
                              //          displayBoardToPlayers(board, total_seeds_collected, players);
                              //          player = (player + 1) % N_PLAYERS;
                              //       }
                              //    }
                              //    int winner = getWinner(total_seeds_collected);
                              //    if (winner == 0)
                              //    {
                              //       write_client(client.sock, "You won!\n");
                              //       write_client(clients[j].sock, "You lost!\n");
                              //    }
                              //    else if (winner == 1)
                              //    {
                              //       write_client(client.sock, "You lost!\n");
                              //       write_client(clients[j].sock, "You won!\n");
                              //    }
                              //    else
                              //    {
                              //       write_client(client.sock, "Draw!\n");
                              //       write_client(clients[j].sock, "Draw!\n");
                              //    }
                              }
                              else
                              {
                                 write_client(client.sock, "Game declined!\n");
                                 write_client(clients[j].sock, "Game declined!\n");
                              }
                              break;
                           }
                        }
                     }
                  }
                  /* list all finished games */
                  else if (strcmp(buffer, "/fgames") == 0)
                  {
                     write_client(client.sock, "List of finished games:\n");
                     for (int j = 0; j < nGames; j++)
                     {
                        if (games[j].finished)
                        {
                           write_client(client.sock, j);
                           write_client(client.sock, " : ");
                           write_client(client.sock, games[j].player1.name);
                           write_client(client.sock, " vs ");
                           write_client(client.sock, games[j].player2.name);
                           write_client(client.sock, "\n");
                        }
                     }
                  }
                  /* rewatch a game */
                  else if (strncmp(buffer, "/rewatch", 8) == 0)
                  {
                     char *gameIndex = buffer + 8;
                     Game game = games[atoi(gameIndex)];
                     if (game.finished)
                     {
                        write_client(client.sock, "You are rewatching ");
                        write_client(client.sock, game.player1.name);
                        write_client(client.sock, " vs ");
                        write_client(client.sock, game.player2.name);
                        write_client(client.sock, "\n");
                        displayBoardToPlayer(game);
                     }
                  } 
                  /* list all unfinished games */
                  else if (strcmp(buffer, "/games") == 0)
                  {
                     write_client(client.sock, "List of games:\n");
                     for (int j = 0; j < nGames; j++)
                     {
                        if (!games[j].finished)
                        {
                           write_client(client.sock, j);
                           write_client(client.sock, " : ");
                           write_client(client.sock, games[j].player1.name);
                           write_client(client.sock, " vs ");
                           write_client(client.sock, games[j].player2.name);
                           write_client(client.sock, "\n");
                        }
                     }
                  }
                  /* spectate a game */
                  else if (strncmp(buffer, "/spectate", 9) == 0)
                  {
                     char *gameIndex = buffer + 9;
                     Game game = games[atoi(gameIndex)];
                     if (!game.finished)
                     {
                        write_client(client.sock, "You are spectating ");
                        write_client(client.sock, game.player1.name);
                        write_client(client.sock, " vs ");
                        write_client(client.sock, game.player2.name);
                        write_client(client.sock, "\n");
                        displayBoardToPlayer(game);
                     }
                  }
                  /* write a bio */
                  else if (strncmp(buffer, "/bio", 4) == 0)
                  {
                     char *bio = buffer + 4;
                     strncpy(client.bio, bio, BUF_SIZE - 1);
                     write_client(client.sock, "Bio updated!\n");
                  }
                  /* see the bio of a player */
                  else if (strncmp(buffer, "/seebio", 7) == 0)
                  {
                     char *player = buffer + 7;
                     for (int j = 0; j < nClients; j++)
                     {
                        if (strcmp(clients[j].name, player) == 0)
                        {
                           write_client(client.sock, "Bio of ");
                           write_client(client.sock, player);
                           write_client(client.sock, " :\n");
                           write_client(client.sock, clients[j].bio);
                           write_client(client.sock, "\n");
                           break;
                        }
                     }
                  }
                  /* exit */
                  else if (strcmp(buffer, "/exit") == 0)
                  {
                     write_client(client.sock, "Goodbye!\n");
                     closesocket(client.sock);
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

static void send_message_to_all_clients(Client *clients, Client sender, int nClients, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for(i = 0; i < nClients; i++)
   {
      /* we don't send message to the sender */
      if(sender.sock != clients[i].sock)
      {
         if(from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
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

void displayBoardToPlayer(Game game)
{
   char display[BUF_SIZE * 4]; // Un grand buffer pour accumuler l'affichage
   int offset = 0; // Position d'écriture dans le buffer

   // Si on affiche pour le joueur 1, on affiche le board normalement

   if (game.turn % 2 == 0)
   {
      for (int i = N_PITS - 1; i >= N_PITS / 2; i--)
      {
         offset += sprintf(display + offset, "%d ", game.board[i]);
      }
      offset += sprintf(display + offset, "\n");
      for (int i = 0; i < N_PITS / 2; i++)
      {
         offset += sprintf(display + offset, "%d ", game.board[i]);
      }
      offset += sprintf(display + offset, "\n");

      //Affichage des coordonnées des cases
      for (int i = 0; i >= N_PITS - 1; i++)
      {
         offset += sprintf(display + offset, "%d ", i);
      }
      offset += sprintf(display + offset, "\n");

      // Affichage des graines collectées
      offset += sprintf(display + offset, "Seeds collected by you : %d\n", game.total_seeds_collected[0]);
      offset += sprintf(display + offset, "Seeds collected by %s : %d\n", game.player2.name, game.total_seeds_collected[1]);
   }
   // Si on affiche pour le joueur 2, on affiche le board à l'envers
   else
   {

      for (int i = N_PITS / 2 - 1; i >= 0; i--)
      {
         offset += sprintf(display + offset, "%d ", game.board[i]);
      }
      offset += sprintf(display + offset, "\n");
      for (int i = N_PITS / 2; i < N_PITS; i++)
      {
         offset += sprintf(display + offset, "%d ", game.board[i]);
      }
      offset += sprintf(display + offset, "\n");

      //Affichage des coordonnées des cases
      for (int i = 0; i >= N_PITS / 2; i++)
      {
         offset += sprintf(display + offset, "%d ", i);
      }
      offset += sprintf(display + offset, "\n");

      // Affichage des graines collectées

      offset += sprintf(display + offset, "Seeds collected by %s : %d\n", game.player1.name, game.total_seeds_collected[0]);
      offset += sprintf(display + offset, "Seeds collected by you : %d\n", game.total_seeds_collected[1]);
   }
   
   // Envoi du buffer au client
   write_client(game.turn % 2 == 0 ? game.player1.sock : game.player2.sock, display);
}

int main()
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
