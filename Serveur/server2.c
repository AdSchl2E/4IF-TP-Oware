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
   int actual = 0;
   int max = sock;
   /* an array for all clients */
   Client clients[MAX_CLIENTS];

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
      for(i = 0; i < actual; i++)
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
         clients[actual] = c;
         actual++;

         // Welcome message
         write_client(csock, "Welcome to the Oware game server!\n");
         write_client(csock, "You ask for the list of players available by typing 'list'.\n");
         write_client(csock, "You can challenge a player by typing 'challenge <player>'.\n");
         //write_client(csock, "You list all the current games by typing 'games'.\n");
         //write_client(csock, "You can spectate a game by typing 'spectate <game>'.\n");                           
         //write_client(csock, "You can write a bio by typing 'bio <bio>'.\n");
         //write_client(csock, "You can see the bio of a player by typing 'bio <player>'.\n");

         /* notify other clients */
         snprintf(buffer, BUF_SIZE - 1, "%s joined the server !", c.name);
         buffer[BUF_SIZE - 1] = 0;
         send_message_to_all_clients(clients, c, actual, buffer, 1);
      }
      /* a client is talking */
      else
      {
         int i = 0;
         for(i = 0; i < actual; i++)
         {
            /* a client is talking */
            if(FD_ISSET(clients[i].sock, &rdfs))
            {
               Client client = clients[i];
               int c = read_client(clients[i].sock, buffer);
               /* client disconnected */
               if(c == 0)
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, client, actual, buffer, 1);
               }
               else
               {
                  /* list all players */
                  printf("Received: %s from %s\n", buffer, client.name);
                  if (strcmp(buffer, "list") == 0) 
                  {
                     write_client(client.sock, "List of players:\n");
                     for (int j = 0; j < actual; j++)
                     {
                        write_client(client.sock, clients[j].name);
                        write_client(client.sock, "\n");
                     }
                  }
                  /* challenge a player */
                  else if (strncmp(buffer, "challenge", 9) == 0)
                  {
                     char *player = buffer + 10;
                     for (int j = 0; j < actual; j++)
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
                              remove_client(clients, j, &actual);
                              strncpy(buffer, player, BUF_SIZE - 1);
                              strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                              send_message_to_all_clients(clients, client, actual, buffer, 1);
                           }
                           else if (strcmp(buffer, "yes") == 0)
                           {
                              Client* players[2] = {&client, &clients[j]};
                              write_client(client.sock, "Game started!\n");
                              write_client(clients[j].sock, "Game started!\n");
                              int board[N_PITS];
                              int total_seeds_collected[N_PLAYERS];
                              initBoard(board, total_seeds_collected);
                              displayBoardToPlayers(board, total_seeds_collected, players);

                              // Randomly choose who starts
                              int player = rand() % 2;
                              write_client(players[player]->sock, "You start!\n");
                              write_client(players[(player + 1) % N_PLAYERS]->sock, "Your opponent starts!\n");
                              
                              int choice = 0;
                              int end = 0;

                              while (!end)
                              {
                                 for (int i = 0; i < N_PLAYERS; i++)
                                 {
                                    choice = askForPlayerChoice(board, players[player]);
                                    int seeds_collected = makeMove(board, choice + player * N_PITS / 2, player, total_seeds_collected);
                                    displayBoardToPlayers(board, total_seeds_collected, players);
                                    player = (player + 1) % N_PLAYERS;
                                 }
                                 end = checkGameEnd(board, total_seeds_collected);
                              }
                              int winner = getWinner(total_seeds_collected);
                              if (winner == 0)
                              {
                                 write_client(client.sock, "You won!\n");
                                 write_client(clients[j].sock, "You lost!\n");
                              }
                              else
                              {
                                 write_client(client.sock, "You lost!\n");
                                 write_client(clients[j].sock, "You won!\n");
                              }
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
                  /* list all games */
                  else if (strcmp(buffer, "games") == 0)
                  {
                     write_client(client.sock, "List of games:\n");
                     for (int j = 0; j < actual; j++)
                     {
                        write_client(client.sock, clients[j].name);
                        write_client(client.sock, "\n");
                     }
                  }
                  /* spectate a game */
                  else if (strncmp(buffer, "spectate", 8) == 0)
                  {
                     char *game = buffer + 9;
                     for (int j = 0; j < actual; j++)
                     {
                        if (strcmp(clients[j].name, game) == 0)
                        {
                           write_client(client.sock, "You are spectating ");
                           write_client(client.sock, game);
                           write_client(client.sock, "\n");
                           write_client(clients[j].sock, "You are being spectated by ");
                           write_client(clients[j].sock, client.name);
                           write_client(clients[j].sock, "\n");
                           break;
                        }
                     }
                  }
                  /* write a bio */
                  else if (strncmp(buffer, "bio", 3) == 0)
                  {
                     char *bio = buffer + 4;
                     for (int j = 0; j < actual; j++)
                     {
                        if (strcmp(clients[j].name, bio) == 0)
                        {
                           write_client(client.sock, "Bio of ");
                           write_client(client.sock, bio);
                           write_client(client.sock, "\n");
                           break;
                        }
                     }
                  }
                  else if (strcmp(buffer, "exit") == 0)
                  {
                     write_client(client.sock, "Goodbye!\n");
                     closesocket(client.sock);
                     remove_client(clients, i, &actual);
                  }
                  else
                  {
                     /* message for all */
                     send_message_to_all_clients(clients, client, actual, buffer, 0);
                  }               
               }
               break;
            }
         }
      }
   }

   clear_clients(clients, actual);
   end_connection(sock);
}

static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for(i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for(i = 0; i < actual; i++)
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

void displayBoardToPlayers(int board[], int total_seeds_collected[], Client *players[2])
{
   char display[BUF_SIZE * 4]; // Un grand buffer pour accumuler l'affichage
   int offset = 0; // Position d'écriture dans le buffer

   // Pour le joueur 1
   offset += snprintf(display + offset, sizeof(display) - offset, "\n   ");

   // Affichage de la rangée supérieure
   for (int i = N_PITS - 1; i >= N_PITS / 2; i--)
   {
      offset += snprintf(display + offset, sizeof(display) - offset, COLOR_CYAN "|%2d|" COLOR_RESET, board[i]);
   }
   offset += snprintf(display + offset, sizeof(display) - offset, "\n   ");

   // Affichage de la rangée inférieure
   for (int i = 0; i < N_PITS / 2; i++)
   {
      offset += snprintf(display + offset, sizeof(display) - offset, COLOR_CYAN "|%2d|" COLOR_RESET, board[i]);
   }
   offset += snprintf(display + offset, sizeof(display) - offset, "\n   ");

   // Coordonnées en bas pour chaque case
   for (int i = 0; i < N_PITS / 2; i++)
   {
      offset += snprintf(display + offset, sizeof(display) - offset, COLOR_YELLOW " %d  " COLOR_RESET, i);
   }
   offset += snprintf(display + offset, sizeof(display) - offset, "\n");

   offset += snprintf(display + offset, sizeof(display) - offset, "Total seeds collected by you: " COLOR_GREEN "%d\n" COLOR_RESET, total_seeds_collected[0]);
   offset += snprintf(display + offset, sizeof(display) - offset, "Total seeds collected by %s: " COLOR_GREEN "%d\n" COLOR_RESET, players[1]->name, total_seeds_collected[1]);

   // Envoyer l'affichage complet pour le joueur 1
   write_client(players[0]->sock, display);

   // Réinitialiser le buffer pour le joueur 2
   offset = 0;
   offset += snprintf(display + offset, sizeof(display) - offset, "\n   ");

   // Affichage de la rangée supérieure
   for (int i = N_PITS / 2 - 1; i >= 0; i--)
   {
      offset += snprintf(display + offset, sizeof(display) - offset, COLOR_CYAN "|%2d|" COLOR_RESET, board[i]);
   }
   offset += snprintf(display + offset, sizeof(display) - offset, "\n   ");

   // Affichage de la rangée inférieure
   for (int i = N_PITS / 2; i < N_PITS; i++)
   {
      offset += snprintf(display + offset, sizeof(display) - offset, COLOR_CYAN "|%2d|" COLOR_RESET, board[i]);
   }
   offset += snprintf(display + offset, sizeof(display) - offset, "\n   ");

   // Coordonnées en bas pour chaque case
   for (int i = 0; i < N_PITS / 2; i++)
   {
      offset += snprintf(display + offset, sizeof(display) - offset, COLOR_YELLOW " %d  " COLOR_RESET, i);
   }
   offset += snprintf(display + offset, sizeof(display) - offset, "\n");

   offset += snprintf(display + offset, sizeof(display) - offset, "Total seeds collected by you: " COLOR_GREEN "%d\n" COLOR_RESET, total_seeds_collected[1]);
   offset += snprintf(display + offset, sizeof(display) - offset, "Total seeds collected by %s: " COLOR_GREEN "%d\n" COLOR_RESET, players[0]->name, total_seeds_collected[0]);

   // Envoyer l'affichage complet pour le joueur 2
   write_client(players[1]->sock, display);
}

static int askForPlayerChoice(int board[], Client *player)
{
   char buffer[BUF_SIZE];
   
   write_client(player->sock, "Choose a pit to play (0-5): ");
   read_client(player->sock, buffer);

   while (atoi(buffer) < 0 || atoi(buffer) >= N_PITS / 2)
   {
      write_client(player->sock, "Invalid choice, please choose a non-empty pit between 0 and 5: ");
      read_client(player->sock, buffer);
   }

   return atoi(buffer);
}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
