#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

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

   printf("Server started on port %d\n", PORT);

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
         client.friends = (SOCKET *)malloc(MAX_FRIENDS * sizeof(SOCKET));
         for (int i = 0; i < MAX_FRIENDS; i++)
         {
            client.friends[i] = INVALID_SOCKET;
         }
         
         // Si le client qui se connecte à un nom qui existe déjà ou si son nom est vide, on lui demande de se connecter avec un autre nom
         
         int nameExistsOrNull = 0;

         do 
         {
            nameExistsOrNull = 0;
            for (int i = 0; i < nClients; i++)
            {
               if (strcmp(clients[i].name, buffer) == 0)
               {
                  nameExistsOrNull = 1;
                  write_client(csock, COLOR_RED);
                  write_client(csock, COLOR_BOLD);
                  write_client(csock, "This name is already taken, please choose another one: ");
                  write_client(csock, COLOR_RESET);
                  read_client(csock, buffer);
                  break;
               }
               else if (strlen(buffer) == 0)
               {
                  nameExistsOrNull = 1;
                  write_client(csock, COLOR_RED);
                  write_client(csock, COLOR_BOLD);
                  write_client(csock, "Name cannot be empty, please choose another one: ");
                  write_client(csock, COLOR_RESET);
                  read_client(csock, buffer);
                  break;
               }
            }
         } while (nameExistsOrNull);       

         strncpy(client.bio, "This player has no bio", BUF_SIZE - 1);
         strncpy(client.name, buffer, BUF_SIZE - 1);

         clients[nClients] = client;
         nClients++;

         // Welcome message with colours

         char owareTitle[BUF_SIZE];
         int offset = 0;

         offset += sprintf(owareTitle + offset, COLOR_YELLOW);
         offset += sprintf(owareTitle + offset, COLOR_BOLD);
         offset += sprintf(owareTitle + offset,"  ,ad8888ba,   I8,        8        ,8I    db         88888888ba   88888888888 \n"); 
         offset += sprintf(owareTitle + offset," d8\"'    `\"8b  `8b       d8b       d8'   d88b        88      \"8b  88          \n");   
         offset += sprintf(owareTitle + offset,"d8'        `8b  \"8,     ,8\"8,     ,8\"   d8'`8b       88      ,8P  88          \n");  
         offset += sprintf(owareTitle + offset,"88          88   Y8     8P Y8     8P   d8'  `8b      88aaaaaa8P'  88aaaaa     \n"); 
         offset += sprintf(owareTitle + offset,"88          88   `8b   d8' `8b   d8'  d8YaaaaY8b     88\"\"\"\"88'    88\"\"\"\"\"     \n"); 
         offset += sprintf(owareTitle + offset,"Y8,        ,8P    `8a a8'   `8a a8'  d8\"\"\"\"\"\"\"\"8b    88    `8b    88          \n");
         offset += sprintf(owareTitle + offset," Y8a.    .a8P      `8a8'     `8a8'  d8'        `8b   88     `8b   88          \n"); 
         offset += sprintf(owareTitle + offset,"  `\"Y8888Y\"'        `8'       `8'  d8'          `8b  88      `8b  88888888888 \n");
         offset += sprintf(owareTitle + offset, COLOR_RESET);

         write_client(clients[i].sock, owareTitle);

         char welcome[BUF_SIZE];
         offset = 0;

         // Welcome message with colors and all of the commands

         offset += sprintf(welcome + offset, COLOR_BOLD);
         offset += sprintf(welcome + offset, "Welcome to Oware, ");
         offset += sprintf(welcome + offset, COLOR_CYAN);
         offset += sprintf(welcome + offset, client.name);
         offset += sprintf(welcome + offset, "\n\n");
         offset += sprintf(welcome + offset, COLOR_RESET);

         write_client(clients[i].sock, welcome);

         char commands[BUF_SIZE];
         offset = 0;

         offset += sprintf(commands + offset, COLOR_BOLD);
         offset += sprintf(commands + offset, "Commands:\n");

         offset += sprintf(commands + offset, COLOR_CYAN);
         offset += sprintf(commands + offset, "/list : List all players\n");
         offset += sprintf(commands + offset, "/challenge <player> : Challenge a player\n");
         offset += sprintf(commands + offset, "/pchallenge <player> : Challenge a player for a private game\n");
         offset += sprintf(commands + offset, "/fgames : List all finished games\n");
         offset += sprintf(commands + offset, "/replay <game index> : Replay a game\n");
         offset += sprintf(commands + offset, "/games : List all unfinished games\n");
         offset += sprintf(commands + offset, "/spectate <game index> : Spectate a game\n");
         offset += sprintf(commands + offset, "/bio <bio> : Write a bio\n");
         offset += sprintf(commands + offset, "/seebio <player> : See the bio of a player\n");
         offset += sprintf(commands + offset, "/friend <player> : Add a friend\n");
         offset += sprintf(commands + offset, "/unfriend <player> : Remove a friend\n");
         offset += sprintf(commands + offset, "/listfriends : List all your friends\n");
         offset += sprintf(commands + offset, "/whisper <player> <message> : Whisper to a player\n");
         offset += sprintf(commands + offset, "/exit : Exit the server\n");
         offset += sprintf(commands + offset, "/help : Display the help\n");
         offset += sprintf(commands + offset, COLOR_RESET);

         write_client(clients[i].sock, commands);
         offset = 0;

         char message[BUF_SIZE];

         /* notify other clients */
         for (int i = 0; i < nClients; i++)
         {
            if (clients[i].sock != csock && nGames == 0)
            {
               offset += sprintf(message + offset, COLOR_YELLOW); 
               offset += sprintf(message + offset, COLOR_BOLD);
               offset += sprintf(message + offset, client.name);
               offset += sprintf(message + offset, " has joined the server!\n");
               offset += sprintf(message + offset, COLOR_RESET);

               write_client(clients[i].sock, message);
               message[0] = '\0';
               offset = 0;
            }
            else
            {
            for (int j = 0; j < nGames; j++)
               {
                  if (!clients[i].playing && !clients[i].spectating && !clients[i].reviewing && clients[i].sock != csock)
                  {
                     offset += sprintf(message + offset, COLOR_YELLOW);
                     offset += sprintf(message + offset, COLOR_BOLD);
                     offset += sprintf(message + offset, client.name);
                     offset += sprintf(message + offset, " has joined the server!\n");
                     offset += sprintf(message + offset, COLOR_RESET);

                     write_client(clients[i].sock, message);
                     message[0] = '\0';
                     offset = 0;
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

               int c = read_client(clients[i].sock, buffer);
               /* client disconnected */
               if(c == 0)
               {
                  // On vérifie si le client était en train de jouer ou de regarder une partie, si c'est le cas, on arrête la partie et on informe les joueurs

                  for (int j = 0; j < nGames; j++)
                  {
                     if (games[j].player1->sock == client->sock || games[j].player2->sock == client->sock)
                     {
                        // On cherche le joueur correspondant au client qui s'est déconnecté
                        Client* player;
                        Client* opponent;

                        if (games[j].player1->sock == client->sock)
                        {
                           player = games[j].player1;
                           opponent = games[j].player2;
                        }
                        else
                        {
                           player = games[j].player2;
                           opponent = games[j].player1;
                        }

                        games[j].finished = 1;
                        opponent->playing = 0;

                        displayBoardToPlayer(&games[j]);
                        displayBoardToSpectator(&games[j]);
                        
                        char message[BUF_SIZE];
                        sprintf(message, COLOR_BOLD);
                        sprintf(message, "Game finished! %s disconnected!\n", player->name);

                        write_client(opponent->sock, COLOR_RED);
                        write_client(opponent->sock, "You opponent disconnected!\nThe game is finished!\n");
                        write_client(opponent->sock, COLOR_RESET);

                        for (int k = 0; k < games[j].nSpectators; k++)
                        {
                           write_client(games[j].spectators[k]->sock, message);
                           write_client(games[j].spectators[k]->sock, COLOR_BOLD);
                           write_client(games[j].spectators[k]->sock, "You can leave the game by typing 'q'\n");
                           write_client(games[j].spectators[k]->sock, COLOR_RESET);
                        }
                        break;
                     }
                  }

                  remove_client(clients, i, &nClients);

                  char disconnectMessage[BUF_SIZE];
                  int offset = 0;
                  offset += sprintf(disconnectMessage + offset, COLOR_YELLOW);
                  offset += sprintf(disconnectMessage + offset, COLOR_BOLD);
                  offset += sprintf(disconnectMessage + offset, client->name);
                  offset += sprintf(disconnectMessage + offset, " has left the server!\n");
                  offset += sprintf(disconnectMessage + offset, COLOR_RESET);
                  
                  send_message_to_all_clients(clients, client, nClients, disconnectMessage, 1);
               }
               else
               {
                  /* Protocole qui gère le client en fonction de ce qu'il a écrit et de ce qu'il fait */

                  // On vérifie si le client a reçu un défi
                  if (client->receiveChallenge > 0)
                  {
                     if (strcmp(buffer, "yes") == 0 || strcmp(buffer, "y") == 0)
                     {
                        client->playing = 1;
                        
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
                        game.private = client->receiveChallenge == 2 ? 1 : 0;

                        client->receiveChallenge = 0;
                        challenger->sendChallenge = 0;

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

                        // On affiche le plateau au joueur et aux spectateurs
                        displayBoardToPlayer(&game);
                        displayBoardToSpectator(&game);

                        // On informe les joueurs
                        write_client(game.player1->sock, COLOR_BOLD);
                        write_client(game.player1->sock, "Game started!\n");
                        write_client(game.player1->sock, COLOR_RESET);
                        write_client(game.player2->sock, COLOR_BOLD);
                        write_client(game.player2->sock, "Game started!\n");
                        write_client(game.player2->sock, COLOR_RESET);

                        // On informe le joueur qui commence
                        write_client(game.player1->sock, COLOR_BOLD);
                        write_client(game.player1->sock, "You start!\n");
                        write_client(game.player1->sock, COLOR_RESET);
                        write_client(game.player2->sock, COLOR_BOLD);
                        write_client(game.player2->sock, "Your opponent starts!\n");
                        write_client(game.player2->sock, COLOR_RESET);

                        // On demande au joueur de jouer
                        write_client(game.player1->sock, COLOR_BOLD);
                        write_client(game.player1->sock, "Choose a pit to play (0-5): ");
                        write_client(game.player1->sock, COLOR_RESET);
                     }
                     else
                     {
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

                        challenger->sendChallenge = 0;
                        client->receiveChallenge = 0;

                        write_client(client->challenger, COLOR_BOLD);
                        write_client(client->challenger, "Your challenge was declined!\n");
                        write_client(client->challenger, COLOR_RESET);
                        client->challenger = INVALID_SOCKET;
                        write_client(client->sock, COLOR_BOLD);
                        write_client(client->sock, "You declined the game!\n");
                        write_client(client->sock, COLOR_RESET);
                     }
                  }

                  // On cherche si le client est dans une partie
                  else if (client->playing)
                  {
                     // On cherche la partie dans laquelle le client est
                     
                     Game* clientGame;

                     for (int j = 0; j < nGames; j++)
                     {
                        if ((games[j].player1->sock == client->sock || games[j].player2->sock == client->sock) && !games[j].finished)
                        {
                           clientGame = &(games[j]);
                           break;
                        }
                     }

                     // Si le client envoie un message avec "/msg <message>", on envoie le message à tous les spectateurs et aux joueurs

                     if (strncmp(buffer, "/chat", 5) == 0)
                     {
                        char *message = buffer + 6;
                        char chatMessage[BUF_SIZE];
                        int offset = 0;
                        for (int j = 0; j < clientGame->nSpectators; j++)
                        {
                           offset += sprintf(chatMessage + offset, COLOR_CYAN);
                           offset += sprintf(chatMessage + offset, client->name);
                           offset += sprintf(chatMessage + offset, COLOR_RESET);
                           offset += sprintf(chatMessage + offset, COLOR_BOLD);
                           offset += sprintf(chatMessage + offset, ": ");
                           offset += sprintf(chatMessage + offset, COLOR_RESET);
                           offset += sprintf(chatMessage + offset, message);
                           offset += sprintf(chatMessage + offset, "\n");

                           write_client(clientGame->spectators[j]->sock, chatMessage);
                           
                           chatMessage[0] = '\0';
                           offset = 0;
                        }
                        
                        offset += sprintf(chatMessage + offset, COLOR_CYAN);
                        offset += sprintf(chatMessage + offset, client->name);
                        offset += sprintf(chatMessage + offset, COLOR_RESET);
                        offset += sprintf(chatMessage + offset, COLOR_BOLD);
                        offset += sprintf(chatMessage + offset, ": ");
                        offset += sprintf(chatMessage + offset, COLOR_RESET);
                        offset += sprintf(chatMessage + offset, message);
                        offset += sprintf(chatMessage + offset, "\n");

                        write_client(clientGame->player1->sock, chatMessage);
                        write_client(clientGame->player2->sock, chatMessage);
                     }

                     // Sinon si le flag draw est à 1, on vérifie si le joueur a écrit "yes" ou "no"
                     // Il faut bien que ce soit le joueur ayant reçu la proposition de match nul qui réponde

                     else if (clientGame->draw == 1)
                     {
                        if ((clientGame->turn % 2 == 0 && clientGame->player2->sock == client->sock) || (clientGame->turn % 2 == 1 && clientGame->player1->sock == client->sock))
                        {
                           if (strcmp(buffer, "yes") == 0 || strcmp(buffer, "y") == 0)
                           {
                              clientGame->finished = 1;
                              clientGame->player1->playing = 0;
                              clientGame->player2->playing = 0;
                              clientGame->draw = 0;

                              displayBoardToPlayer(clientGame);
                              displayBoardToSpectator(clientGame);

                              char message[BUF_SIZE];
                              write_client(clientGame->player1->sock, COLOR_BOLD);
                              write_client(clientGame->player2->sock, COLOR_BOLD);

                              sprintf(message, "Game finished! Draw!\n");
                              write_client(clientGame->player1->sock, message);
                              write_client(clientGame->player2->sock, message);

                              write_client(clientGame->player1->sock, COLOR_RESET);
                              write_client(clientGame->player2->sock, COLOR_RESET);

                              // On informe les spectateurs

                              for (int j = 0; j < clientGame->nSpectators; j++)
                              {
                                 write_client(clientGame->spectators[j]->sock, message);
                                 write_client(clientGame->spectators[j]->sock, COLOR_BOLD);
                                 write_client(clientGame->spectators[j]->sock, "You can leave the game by typing 'q'\n");
                                 write_client(clientGame->spectators[j]->sock, COLOR_RESET);
                              }

                              break;
                           }
                           else if (strcmp(buffer, "no") == 0 || strcmp(buffer, "n") == 0)
                           {
                              clientGame->draw = 0;
                              write_client(clientGame->turn % 2 == 0 ? clientGame->player2->sock : clientGame->player1->sock, COLOR_BOLD);
                              write_client(clientGame->turn % 2 == 0 ? clientGame->player1->sock : clientGame->player2->sock, COLOR_BOLD);
                              write_client(clientGame->turn % 2 == 0 ? clientGame->player1->sock : clientGame->player2->sock, "Your opponent declined the draw!\n");
                              write_client(clientGame->turn % 2 == 0 ? clientGame->player2->sock : clientGame->player1->sock, "You declined the draw!\n");

                              // On demande au joueur de jouer

                              write_client(clientGame->turn % 2 == 0 ? clientGame->player1->sock : clientGame->player2->sock, "Choose a pit to play (0-5): ");

                              write_client(clientGame->turn % 2 == 0 ? clientGame->player1->sock : clientGame->player2->sock, COLOR_RESET);
                              write_client(clientGame->turn % 2 == 0 ? clientGame->player2->sock : clientGame->player1->sock, COLOR_RESET);
                              break;
                           }
                           else
                           {
                              write_client(client->sock, COLOR_RED);
                              write_client(client->sock, COLOR_BOLD);
                              write_client(client->sock, "Invalid choice, please choose 'yes' or 'no': ");
                              write_client(client->sock, COLOR_RESET);
                              break;

                           }
                        }
                        else
                        {
                           write_client(client->sock, COLOR_RED);
                           write_client(client->sock, "Wait for your opponent's response!\n");
                           write_client(client->sock, COLOR_RESET);
                        }
                     }

                     // Sinon on vérifie si c'est le tour du client

                     else if ((clientGame->turn % 2 == 0 && clientGame->player1->sock == client->sock) || (clientGame->turn % 2 == 1 && clientGame->player2->sock == client->sock))
                     {
                        // On vérifie si le client a écrit un nombre entre 0 et 5

                        int choice = atoi(buffer);

                        //// On affiche ce que le client a écrit
                        //printf("Player %s played in pit %d\n", client->name, choice);
                        //printf(clientGame->board[choice + (clientGame->turn % 2 == 0 ? 0 : N_PITS / 2)] == 0 ? "Pit is empty\n" : "Pit is not empty\n");

                        // Si le joueur a écrit "draw", on propose un match nul

                        if (strcmp(buffer, "draw") == 0)
                        {
                           clientGame->draw = 1;
                           write_client(clientGame->turn % 2 == 0 ? clientGame->player2->sock : clientGame->player1->sock, COLOR_BOLD);
                           write_client(clientGame->turn % 2 == 0 ? clientGame->player1->sock : clientGame->player2->sock, COLOR_BOLD);
                           write_client(clientGame->turn % 2 == 0 ? clientGame->player2->sock : clientGame->player1->sock, "Your opponent proposed a draw! Do you accept? (yes/no): ");
                           write_client(clientGame->turn % 2 == 0 ? clientGame->player1->sock : clientGame->player2->sock, "You proposed a draw! Waiting for your opponent's response...\n");
                           write_client(clientGame->turn % 2 == 0 ? clientGame->player1->sock : clientGame->player2->sock, COLOR_RESET);
                           write_client(clientGame->turn % 2 == 0 ? clientGame->player2->sock : clientGame->player1->sock, COLOR_RESET);
                           break;
                        }
                           
                        if (choice < 0 || choice >= N_PITS / 2 || clientGame->board[choice + (clientGame->turn % 2 == 0 ? 0 : N_PITS / 2)] == 0)
                        {
                           write_client(client->sock, COLOR_RED);
                           write_client(client->sock, "Invalid choice, please choose a non-empty pit between 0 and 5: ");
                           write_client(client->sock, COLOR_RESET);
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
                              sprintf(message, COLOR_BOLD);
                              sprintf(message, "Game finished! %s won!\n", clientGame->player1->name);
                              write_client(clientGame->player1->sock, COLOR_GREEN);
                              write_client(clientGame->player1->sock, "You won!\n");
                              write_client(clientGame->player2->sock, COLOR_MAGENTA);
                              write_client(clientGame->player2->sock, "You lost!\n");
                              write_client(clientGame->player1->sock, COLOR_RESET);
                              write_client(clientGame->player2->sock, COLOR_RESET);
                              sprintf(message, COLOR_RESET);
                           }
                           else if (winner == 1)
                           {
                              sprintf(message, COLOR_BOLD);
                              sprintf(message, "Game finished! %s won!\n", clientGame->player2->name);
                              write_client(clientGame->player2->sock, COLOR_GREEN);
                              write_client(clientGame->player2->sock, "You won!\n");
                              write_client(clientGame->player1->sock, COLOR_MAGENTA);
                              write_client(clientGame->player1->sock, "You lost!\n");
                              write_client(clientGame->player2->sock, COLOR_RESET);
                              write_client(clientGame->player1->sock, COLOR_RESET);
                              sprintf(message, COLOR_RESET);
                           }
                           else
                           {
                              sprintf(message, COLOR_BOLD);
                              sprintf(message, "Game finished! Draw!\n");
                              write_client(clientGame->player1->sock, COLOR_YELLOW);
                              write_client(clientGame->player1->sock, "Draw!\n");
                              write_client(clientGame->player2->sock, COLOR_YELLOW);
                              write_client(clientGame->player2->sock, "Draw!\n");
                              write_client(clientGame->player1->sock, COLOR_RESET);
                              write_client(clientGame->player2->sock, COLOR_RESET);
                              sprintf(message, COLOR_RESET);
                           }
                           // On informe les spectateurs
                           for (int j = 0; j < clientGame->nSpectators; j++)
                           {
                              write_client(clientGame->spectators[j]->sock, COLOR_BOLD);

                              write_client(clientGame->spectators[j]->sock, message);
                              write_client(clientGame->spectators[j]->sock, "You can leave the game by typing 'q'\n");

                              write_client(clientGame->spectators[j]->sock, COLOR_RESET);
                           }
                           displayBoardToPlayer(clientGame);
                           displayBoardToSpectator(clientGame);
                           break;
                        }

                        // On affiche le plateau au joueur 

                        displayBoardToPlayer(clientGame);
                        

                        // On passe au tour suivant

                        clientGame->turn++;

                        // On informe le joueur adverse

                        write_client(clientGame->turn % 2 == 0 ? clientGame->player1->sock : clientGame->player2->sock, COLOR_BOLD);

                        write_client(clientGame->turn % 2 == 0 ? clientGame->player1->sock : clientGame->player2->sock, "Your opponent played in pit ");
                        char oppponentMove[BUF_SIZE];
                        sprintf(oppponentMove, "%d", clientGame->moves[clientGame->turn - 1] + (clientGame->turn % 2 == 0 ? 0 : N_PITS / 2));
                        write_client(clientGame->turn % 2 == 0 ? clientGame->player1->sock : clientGame->player2->sock, oppponentMove);
                        write_client(clientGame->turn % 2 == 0 ? clientGame->player1->sock : clientGame->player2->sock, "\n");

                        write_client(clientGame->turn % 2 == 0 ? clientGame->player1->sock : clientGame->player2->sock, COLOR_RESET);

                        // On affiche le plateau au joueur adverse et aux spectateurs

                        displayBoardToPlayer(clientGame);
                        displayBoardToSpectator(clientGame);

                        // On demande au joueur de jouer
                        write_client(clientGame->turn % 2 == 0 ? clientGame->player1->sock : clientGame->player2->sock, COLOR_BOLD);

                        write_client(clientGame->turn % 2 == 0 ? clientGame->player1->sock : clientGame->player2->sock, "Choose a pit to play (0-5): ");

                        write_client(clientGame->turn % 2 == 0 ? clientGame->player1->sock : clientGame->player2->sock, COLOR_RESET);

                        break;
                     }
                     else
                     // Si ce n'est pas le tour du client
                     {
                        write_client(client->sock, COLOR_RED);
                        write_client(client->sock, "It's not your turn!\n");
                        write_client(client->sock, COLOR_RESET);
                     }
                  }
                  // Si le client est en train de spectate une partie
                  else if (client->spectating)
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
                        write_client(client->sock, COLOR_BOLD);
                        write_client(client->sock, "You stopped spectating the game!\n");
                        write_client(client->sock, COLOR_RESET);
                        break;
                     }

                     // Si il a fait /chat <message>, on envoie le message à tous les spectateurs et aux joueurs
                     if (strncmp(buffer, "/chat", 5) == 0)
                     {
                        char *message = buffer + 6;
                        char chatMessage[BUF_SIZE];
                        int offset = 0;
                        for (int j = 0; j < game->nSpectators; j++)
                        {
                           offset += sprintf(chatMessage + offset, COLOR_CYAN);
                           offset += sprintf(chatMessage + offset, client->name);
                           offset += sprintf(chatMessage + offset, COLOR_RESET);
                           offset += sprintf(chatMessage + offset, COLOR_BOLD);
                           offset += sprintf(chatMessage + offset, ": ");
                           offset += sprintf(chatMessage + offset, COLOR_RESET);
                           offset += sprintf(chatMessage + offset, message);
                           offset += sprintf(chatMessage + offset, "\n");

                           write_client(game->spectators[j]->sock, chatMessage);
                           
                           chatMessage[0] = '\0';
                           offset = 0;
                        }
                        chatMessage[0] = '\0';
                        offset = 0;
                        offset += sprintf(chatMessage + offset, COLOR_CYAN);
                        offset += sprintf(chatMessage + offset, client->name);
                        offset += sprintf(chatMessage + offset, COLOR_RESET);
                        offset += sprintf(chatMessage + offset, COLOR_BOLD);
                        offset += sprintf(chatMessage + offset, ": ");
                        offset += sprintf(chatMessage + offset, COLOR_RESET);
                        offset += sprintf(chatMessage + offset, message);
                        offset += sprintf(chatMessage + offset, "\n");

                        write_client(game->player1->sock, chatMessage);
                        write_client(game->player2->sock, chatMessage);
                     }

                     // Sinon, on ignore le message
                  }
                  // Si le client est en train de revoir une partie
                  else if (client->reviewing)
                  {
                     // Si il a appuyé sur 'q', on arrête de regarder la partie
                     
                     if (strcmp(buffer, "q") == 0)
                     {
                        client->reviewing = 0;
                        write_client(client->sock, COLOR_BOLD);
                        write_client(client->sock, "You stopped reviewing the game!\n");
                        write_client(client->sock, COLOR_RESET);
                        break;
                     }
                  }
   
                  /* list all players */
                  else if (strcmp(buffer, "/list") == 0) 
                  {
                     write_client(client->sock, COLOR_BOLD);
                     write_client(client->sock, "\nList of players:\n");
                     write_client(client->sock, COLOR_RESET);

                     char players[BUF_SIZE];
                     int offset = 0;

                     for (int j = 0; j < nClients; j++)
                     {
                        if (clients[j].sock == client->sock)
                        {
                           offset += sprintf(players + offset, COLOR_GREEN);
                           offset += sprintf(players + offset, COLOR_BOLD);
                           offset += sprintf(players + offset, client->name); 
                           offset += sprintf(players + offset, "\n");
                           offset += sprintf(players + offset, COLOR_RESET);
                           
                        }
                        else if (clients[j].sock != INVALID_SOCKET)
                        {
                           offset += sprintf(players + offset, COLOR_CYAN);
                           offset += sprintf(players + offset, clients[j].name);
                           offset += sprintf(players + offset, "\n");
                           offset += sprintf(players + offset, COLOR_RESET);
                        }
                     }

                     write_client(client->sock, players);
                  }
                  
                  /* challenge a player */
                  else if (strncmp(buffer, "/challenge", 10) == 0)
                  {
                     // On vérifie que le client n'envoit pas juste "/challenge"
                     if (strlen(buffer) < 12)
                     {
                        write_client(client->sock, COLOR_RED);
                        write_client(client->sock, "Invalid player name!\n");
                        write_client(client->sock, "Syntax: /challenge <player>\n");
                        write_client(client->sock, COLOR_RESET);
                        break;
                     }
                     char *player = buffer + 11;

                     // Le joueur ne peut pas se défier lui-même
                     if (strcmp(player, client->name) == 0)
                     {
                        write_client(client->sock, COLOR_RED);
                        write_client(client->sock, "You can't challenge yourself!\n");
                        write_client(client->sock, COLOR_RESET);
                        break;
                     }

                     for (int j = 0; j < nClients; j++)
                     {
                        if (strcmp(clients[j].name, player) == 0)
                        {
                           if (!clients[j].playing && !clients[j].spectating && !clients[j].reviewing && clients[j].receiveChallenge == 0 && clients[j].sendChallenge == 0)
                           {
                              char message[BUF_SIZE];
                              int offset = 0;

                              offset += sprintf(message + offset, COLOR_BOLD);
                              offset += sprintf(message + offset, "\nYou challenged ");
                              offset += sprintf(message + offset, COLOR_RESET);
                              offset += sprintf(message + offset, COLOR_CYAN);
                              offset += sprintf(message + offset, player);
                              offset += sprintf(message + offset, COLOR_RESET);

                              write_client(client->sock, message);

                              clients[j].receiveChallenge = 1;
                              client->sendChallenge = 1;
                              clients[j].challenger = client->sock;

                              message[0] = '\0';
                              offset = 0;
                              
                              offset += sprintf(message + offset, COLOR_BOLD);
                              offset += sprintf(message + offset, "\nYou have been challenged by ");
                              offset += sprintf(message + offset, COLOR_RESET);
                              offset += sprintf(message + offset, COLOR_CYAN);
                              offset += sprintf(message + offset, client->name);
                              offset += sprintf(message + offset, COLOR_RESET);
                              offset += sprintf(message + offset, COLOR_BOLD);
                              offset += sprintf(message + offset, "\n");
                              offset += sprintf(message + offset, "Do you accept? (yes/no)\n");
                              offset += sprintf(message + offset, COLOR_RESET);

                              write_client(clients[j].sock, message);                           
                           }
                           else
                           {
                              write_client(client->sock, COLOR_RED);
                              write_client(client->sock, "This player is not available for a challenge!\n");
                              write_client(client->sock, COLOR_RESET);
                           }
                           break;
                        }  
                        else if (j == nClients - 1)
                        {
                           write_client(client->sock, COLOR_RED);
                           write_client(client->sock, "This player does not exist!\n");
                           write_client(client->sock, COLOR_RESET);
                        }                                                               
                     }
                  }

                  /* challenge a player for a private game */
                  else if (strncmp(buffer, "/pchallenge", 11) == 0)
                  {
                     // On vérifie que le client n'envoit pas juste "/pchallenge"
                     if (strlen(buffer) < 13)
                     {
                        write_client(client->sock, COLOR_RED);
                        write_client(client->sock, "Invalid player name!\n");
                        write_client(client->sock, "Syntax: /pchallenge <player>\n");
                        write_client(client->sock, COLOR_RESET);
                        break;
                     }
                     char *player = buffer + 12;

                     // Le joueur ne peut pas se défier lui-même
                     if (strcmp(player, client->name) == 0)
                     {
                        write_client(client->sock, COLOR_RED);
                        write_client(client->sock, "You can't challenge yourself!\n");
                        write_client(client->sock, COLOR_RESET);
                        break;
                     }

                     for (int j = 0; j < nClients; j++)
                     {
                        if (strcmp(clients[j].name, player) == 0)
                        {
                           if (!clients[j].playing && !clients[j].spectating && !clients[j].reviewing && clients[j].receiveChallenge == 0)
                           {
                              char message[BUF_SIZE];
                              int offset = 0;

                              offset += sprintf(message + offset, COLOR_BOLD);
                              offset += sprintf(message + offset, "\nYou challenged ");
                              offset += sprintf(message + offset, COLOR_RESET);
                              offset += sprintf(message + offset, COLOR_CYAN);
                              offset += sprintf(message + offset, player);
                              offset += sprintf(message + offset, COLOR_RESET);
                              offset += sprintf(message + offset, COLOR_BOLD);
                              offset += sprintf(message + offset, " for a private game\n");
                              offset += sprintf(message + offset, COLOR_RESET);

                              write_client(client->sock, message);

                              clients[j].receiveChallenge = 2;
                              clients[j].challenger = client->sock;

                              message[0] = '\0';
                              offset = 0;

                              offset += sprintf(message + offset, COLOR_BOLD);
                              offset += sprintf(message + offset, "\nYou have been challenged by ");
                              offset += sprintf(message + offset, COLOR_RESET);
                              offset += sprintf(message + offset, COLOR_CYAN);
                              offset += sprintf(message + offset, client->name);
                              offset += sprintf(message + offset, COLOR_RESET);
                              offset += sprintf(message + offset, COLOR_BOLD);
                              offset += sprintf(message + offset, " for a private game\n");
                              offset += sprintf(message + offset, "Do you accept? (yes/no)\n");
                              offset += sprintf(message + offset, COLOR_RESET);

                              write_client(clients[j].sock, message);                         
                           }
                           else
                           {
                              write_client(client->sock, COLOR_RED);
                              write_client(client->sock, "This player is not available for a challenge!\n");
                              write_client(client->sock, COLOR_RESET);
                           }
                           break;
                        } 
                        else if (j == nClients - 1)
                        {
                           write_client(client->sock, COLOR_RED);
                           write_client(client->sock, "This player does not exist!\n");
                           write_client(client->sock, COLOR_RESET);
                        }
                     }
                     break;                   
                  }
                  
                  /* list all finished games */
                  else if (strcmp(buffer, "/fgames") == 0)
                  {
                     write_client(client->sock, COLOR_BOLD);
                     write_client(client->sock, "\nList of finished games:\n");
                     write_client(client->sock, COLOR_RESET);

                     char gamesList[BUF_SIZE];
                     int offset = 0;

                     for (int j = 0; j < nGames; j++)
                     {
                        if (games[j].finished)
                        {                        
                           offset += sprintf(gamesList + offset, COLOR_BOLD);
                           offset += sprintf(gamesList + offset, "%d", j);
                           offset += sprintf(gamesList + offset, " : ");
                           offset += sprintf(gamesList + offset, COLOR_RESET);
                           offset += sprintf(gamesList + offset, COLOR_CYAN);
                           offset += sprintf(gamesList + offset, games[j].player1->name);
                           offset += sprintf(gamesList + offset, COLOR_RESET);
                           offset += sprintf(gamesList + offset, COLOR_BOLD);
                           offset += sprintf(gamesList + offset, " VS ");
                           offset += sprintf(gamesList + offset, COLOR_RESET);
                           offset += sprintf(gamesList + offset, COLOR_CYAN);
                           offset += sprintf(gamesList + offset, games[j].player2->name);
                           offset += sprintf(gamesList + offset, COLOR_RESET);
                           offset += sprintf(gamesList + offset, COLOR_BOLD);
                           offset += sprintf(gamesList + offset, " (");
                           offset += sprintf(gamesList + offset, "%d", games[j].total_seeds_collected[0]);
                           offset += sprintf(gamesList + offset, " - ");
                           offset += sprintf(gamesList + offset, "%d", games[j].total_seeds_collected[1]);
                           offset += sprintf(gamesList + offset, ")");
                           offset += sprintf(gamesList + offset, "\n");
                           offset += sprintf(gamesList + offset, COLOR_RESET);
                        }
                     }
                     if (offset == 0)
                     {
                        offset += sprintf(gamesList + offset, COLOR_RED);
                        offset += sprintf(gamesList + offset, "No finished games available!\n");
                        offset += sprintf(gamesList + offset, COLOR_RESET);
                     }

                     write_client(client->sock, gamesList);
                  }
                  
                  /* rewatch a game */
                  else if (strncmp(buffer, "/replay", 7) == 0)
                  {
                     // On vérifie que le client n'envoit pas juste "/replay"
                     if (strlen(buffer) < 9)
                     {
                        write_client(client->sock, COLOR_RED);
                        write_client(client->sock, "Invalid game index!\n");
                        write_client(client->sock, "Syntax: /replay <game index>\n");
                        write_client(client->sock, COLOR_RESET);
                        break;
                     }

                     char *gameIndex = buffer + 8;

                     // On vérifie si l'index est valide
                     if (atoi(gameIndex) < 0 || atoi(gameIndex) >= nGames)
                     {
                        write_client(client->sock, COLOR_RED);
                        write_client(client->sock, "Invalid game index!\n");
                        write_client(client->sock, COLOR_RESET);
                        break;
                     }

                     Game *game = &games[atoi(gameIndex)];

                     char message[BUF_SIZE];
                     int offset = 0;

                     if (game->finished)
                     {
                        offset += sprintf(message + offset, COLOR_BOLD);
                        offset += sprintf(message + offset, "\nYou are reviewing ");
                        offset += sprintf(message + offset, COLOR_RESET);
                        offset += sprintf(message + offset, COLOR_CYAN);
                        offset += sprintf(message + offset, game->player1->name);
                        offset += sprintf(message + offset, COLOR_RESET);
                        offset += sprintf(message + offset, COLOR_BOLD);
                        offset += sprintf(message + offset, " VS ");
                        offset += sprintf(message + offset, COLOR_RESET);
                        offset += sprintf(message + offset, COLOR_CYAN);
                        offset += sprintf(message + offset, game->player2->name);
                        offset += sprintf(message + offset, COLOR_RESET);
                        offset += sprintf(message + offset, "\n");
                        offset += sprintf(message + offset, COLOR_BOLD);
                        offset += sprintf(message + offset, "Type 'q' to stop reviewing\n");
                        offset += sprintf(message + offset, COLOR_RESET);

                        write_client(client->sock, message);

                        replay(client, game);
                     }
                     else
                     {
                        write_client(client->sock, COLOR_RED);
                        write_client(client->sock, "This game is not finished!\n");
                        write_client(client->sock, COLOR_RESET);
                     }
                  } 

                  /* list all unfinished games */
                  else if (strcmp(buffer, "/games") == 0)
                  {
                     char gamesList[BUF_SIZE];
                     int offset = 0;

                     offset += sprintf(gamesList + offset, COLOR_BOLD);
                     offset += sprintf(gamesList + offset, "\nList of games:\n");
                     offset += sprintf(gamesList + offset, COLOR_RESET);

                     if (nGames == 0)
                     {
                        offset += sprintf(gamesList + offset, COLOR_RED);
                        offset += sprintf(gamesList + offset, "No games available!\n");
                        offset += sprintf(gamesList + offset, COLOR_RESET);
                        write_client(client->sock, gamesList);
                        break;
                     }

                     offset += sprintf(gamesList + offset, COLOR_BOLD);
                     offset += sprintf(gamesList + offset, COLOR_GREEN);
                     offset += sprintf(gamesList + offset, "Public games:\n");
                     offset += sprintf(gamesList + offset, COLOR_RESET);

                     for (int j = 0; j < nGames; j++)
                     {
                        if (!games[j].finished && !games[j].private)
                        {
                           offset += sprintf(gamesList + offset, COLOR_BOLD);
                           offset += sprintf(gamesList + offset, "%d", j);
                           offset += sprintf(gamesList + offset, " : ");
                           offset += sprintf(gamesList + offset, COLOR_RESET);
                           offset += sprintf(gamesList + offset, COLOR_CYAN);
                           offset += sprintf(gamesList + offset, games[j].player1->name);
                           offset += sprintf(gamesList + offset, COLOR_RESET);
                           offset += sprintf(gamesList + offset, COLOR_BOLD);
                           offset += sprintf(gamesList + offset, " VS ");
                           offset += sprintf(gamesList + offset, COLOR_RESET);
                           offset += sprintf(gamesList + offset, COLOR_CYAN);
                           offset += sprintf(gamesList + offset, games[j].player2->name);
                           offset += sprintf(gamesList + offset, COLOR_RESET);
                           offset += sprintf(gamesList + offset, COLOR_BOLD);
                           if (games[j].nSpectators == MAX_SPECTATORS)
                           {
                              offset += sprintf(gamesList + offset, COLOR_RED);
                           }
                           offset += sprintf(gamesList + offset, " (");
                           offset += sprintf(gamesList + offset, "%d", games[j].nSpectators);
                           offset += sprintf(gamesList + offset, ")");
                           offset += sprintf(gamesList + offset, "\n");
                           offset += sprintf(gamesList + offset, COLOR_RESET);
                        }
                     }
                     offset += sprintf(gamesList + offset, COLOR_BOLD);
                     offset += sprintf(gamesList + offset, COLOR_MAGENTA);
                     offset += sprintf(gamesList + offset, "Private games:\n");
                     offset += sprintf(gamesList + offset, COLOR_RESET);
                     for (int j = 0; j < nGames; j++)
                     {
                        if (!games[j].finished && games[j].private)
                        {
                           offset += sprintf(gamesList + offset, COLOR_BOLD);
                           offset += sprintf(gamesList + offset, "%d", j);
                           offset += sprintf(gamesList + offset, " : ");
                           offset += sprintf(gamesList + offset, COLOR_RESET);
                           offset += sprintf(gamesList + offset, COLOR_CYAN);
                           offset += sprintf(gamesList + offset, games[j].player1->name);
                           offset += sprintf(gamesList + offset, COLOR_RESET);
                           offset += sprintf(gamesList + offset, COLOR_BOLD);
                           offset += sprintf(gamesList + offset, " VS ");
                           offset += sprintf(gamesList + offset, COLOR_RESET);
                           offset += sprintf(gamesList + offset, COLOR_CYAN);
                           offset += sprintf(gamesList + offset, games[j].player2->name);
                           offset += sprintf(gamesList + offset, COLOR_RESET);
                           offset += sprintf(gamesList + offset, COLOR_BOLD);
                           if (games[j].nSpectators == MAX_SPECTATORS)
                           {
                              offset += sprintf(gamesList + offset, COLOR_RED);
                           }
                           offset += sprintf(gamesList + offset, " (");
                           offset += sprintf(gamesList + offset, "%d", games[j].nSpectators);
                           offset += sprintf(gamesList + offset, ")");
                           offset += sprintf(gamesList + offset, "\n");
                           offset += sprintf(gamesList + offset, COLOR_RESET);   
                        }
                     }

                     write_client(client->sock, gamesList);
                  }
                  
                  /* spectate a game */
                  else if (strncmp(buffer, "/spectate", 9) == 0)
                  {
                     // On vérifie que le client n'a pas juste envoyé /spectate
                     if (strlen(buffer) < 11)
                     {
                        write_client(client->sock, COLOR_RED);
                        write_client(client->sock, "Invalid command!\n");
                        write_client(client->sock, "Syntax: /spectate <game index>\n");
                        write_client(client->sock, COLOR_RESET);
                        break;
                     }
                     // Le client peut regarder une partie en cours si elle n'est pas finie et si elle n'est pas privée
                     // Si elle est privée, il peut regarder que si un des joueurs est son ami

                     char *gameIndex = buffer + 10;

                     // On vérifie si l'index est valide
                     if (atoi(gameIndex) < 0 || atoi(gameIndex) >= nGames)
                     {
                        write_client(client->sock, COLOR_RED);
                        write_client(client->sock, "Invalid game index!\n");
                        write_client(client->sock, COLOR_RESET);
                        break;
                     }

                     Game *game = &games[atoi(gameIndex)];
                     char message[BUF_SIZE];
                     message[0] = '\0';
                     int offset = 0;
                     if (!game->finished && !game->private && game->nSpectators < MAX_SPECTATORS)
                     {
                        offset += sprintf(message + offset, COLOR_BOLD);
                        offset += sprintf(message + offset, "\nYou are spectating ");
                        offset += sprintf(message + offset, COLOR_RESET);
                        offset += sprintf(message + offset, COLOR_CYAN);
                        offset += sprintf(message + offset, game->player1->name);
                        offset += sprintf(message + offset, COLOR_RESET);
                        offset += sprintf(message + offset, COLOR_BOLD);
                        offset += sprintf(message + offset, " VS ");
                        offset += sprintf(message + offset, COLOR_RESET);
                        offset += sprintf(message + offset, COLOR_CYAN);
                        offset += sprintf(message + offset, game->player2->name);
                        offset += sprintf(message + offset, COLOR_RESET);
                        offset += sprintf(message + offset, "\n");
                        offset += sprintf(message + offset, COLOR_BOLD);
                        offset += sprintf(message + offset, "Type 'q' to stop spectating\n");
                        offset += sprintf(message + offset, "Type '/chat <message>' to chat with the players and the spectators\n");
                        offset += sprintf(message + offset, COLOR_RESET);


                        
                        game->spectators[game->nSpectators] = client;
                        game->nSpectators++;
                        client->spectating = 1;
                     }
                     else if (!game->finished && game->private)
                     {
                        // On parcours les amis des deux joueurs pour voir si le client est ami avec un des deux

                        int isFriend = 0;
                        for (int j = 0; j < MAX_FRIENDS; j++)
                        {
                           if (game->player1->friends[j] == client->sock || game->player2->friends[j] == client->sock)
                           {
                              isFriend = 1;
                              break;
                           }
                        }

                        if (isFriend)
                        {
                           offset += sprintf(message + offset, COLOR_BOLD);
                           offset += sprintf(message + offset, "\nYou are spectating ");
                           offset += sprintf(message + offset, COLOR_RESET);
                           offset += sprintf(message + offset, COLOR_CYAN);
                           offset += sprintf(message + offset, game->player1->name);
                           offset += sprintf(message + offset, COLOR_RESET);
                           offset += sprintf(message + offset, COLOR_BOLD);
                           offset += sprintf(message + offset, " VS ");
                           offset += sprintf(message + offset, COLOR_RESET);
                           offset += sprintf(message + offset, COLOR_CYAN);
                           offset += sprintf(message + offset, game->player2->name);
                           offset += sprintf(message + offset, COLOR_RESET);
                           offset += sprintf(message + offset, "\n");
                           offset += sprintf(message + offset, COLOR_BOLD);
                           offset += sprintf(message + offset, "Type 'q' to stop spectating\n");
                           offset += sprintf(message + offset, "Type '/chat <message>' to chat with the players and the spectators\n");
                           offset += sprintf(message + offset, COLOR_RESET);

                           game->spectators[game->nSpectators] = client;
                           game->nSpectators++;
                           client->spectating = 1;
                        }
                        else if (!client->spectating)
                        {
                           write_client(client->sock, COLOR_RED);
                           write_client(client->sock, "This game is private and none of the players are friends with you!\n");
                           write_client(client->sock, COLOR_RESET);
                        }
                     }
                     else
                     {
                        write_client(client->sock, COLOR_RED);
                        write_client(client->sock, "This game is finished or has too many spectators!\n");
                        write_client(client->sock, COLOR_RESET);
                     }    

                     write_client(client->sock, message);     
                  }

                  /* write a bio */
                  else if (strncmp(buffer, "/bio", 4) == 0)
                  {
                     char *bio = buffer + 5;
                     strncpy(client->bio, bio, BUF_SIZE - 1);
                     write_client(client->sock, COLOR_BOLD);
                     write_client(client->sock, "\nBio updated!\n");
                     write_client(client->sock, COLOR_RESET);
                  }

                  /* see the bio of a player */
                  else if (strncmp(buffer, "/seebio", 7) == 0)
                  {
                     // On vérifie que le client n'a pas juste envoyé /seebio
                     if (strlen(buffer) < 9)
                     {
                        write_client(client->sock, COLOR_RED);
                        write_client(client->sock, "Invalid command!\n");
                        write_client(client->sock, "Syntax: /seebio <player>\n");
                        write_client(client->sock, COLOR_RESET);
                        break;
                     }
                     char *player = buffer + 8;
                     for (int j = 0; j < nClients; j++)
                     {
                        if (strcmp(clients[j].name, player) == 0)
                        {
                           char bio[BUF_SIZE];  
                           int offset = 0;
                           offset += sprintf(bio + offset, COLOR_BOLD);
                           offset += sprintf(bio + offset, "\nBio of ");
                           offset += sprintf(bio + offset, COLOR_RESET);
                           offset += sprintf(bio + offset, COLOR_CYAN);
                           offset += sprintf(bio + offset, player);
                           offset += sprintf(bio + offset, COLOR_RESET);
                           offset += sprintf(bio + offset, COLOR_BOLD);
                           offset += sprintf(bio + offset, " :\n");
                           offset += sprintf(bio + offset, COLOR_RESET);
                           offset += sprintf(bio + offset, clients[j].bio);
                           offset += sprintf(bio + offset, "\n");
                           write_client(client->sock, bio);
                           break;
                        }
                     }
                  }
                  // Ajouter un ami
                  else if (strncmp(buffer, "/friend", 7) == 0)
                  {
                     // On vérifie que le client n'a pas juste envoyé /friend
                     if (strlen(buffer) < 9)
                     {
                        write_client(client->sock, COLOR_RED);
                        write_client(client->sock, "Invalid command!\n");
                        write_client(client->sock, "Syntax: /friend <player>\n");
                        write_client(client->sock, COLOR_RESET);
                        break;
                     }

                     char *player = buffer + 8;

                     // Un client ne peut pas s'ajouter lui-même en ami
                     if (strcmp(player, client->name) == 0)
                     {
                        write_client(client->sock, COLOR_RED);
                        write_client(client->sock, "You can't add yourself as a friend!\n");
                        write_client(client->sock, COLOR_RESET);
                        break;
                     }

                     char message[BUF_SIZE];
                     int offset = 0;
                     int found = 0;

                     for (int j = 0; j < nClients; j++)
                     {
                        if (strcmp(clients[j].name, player) == 0)
                        {
                           client->friends[j] = clients[j].sock;
                           offset += sprintf(message + offset, COLOR_BOLD);
                           offset += sprintf(message + offset, "\nYou added ");
                           offset += sprintf(message + offset, COLOR_RESET);
                           offset += sprintf(message + offset, COLOR_CYAN);
                           offset += sprintf(message + offset, player);
                           offset += sprintf(message + offset, COLOR_RESET);
                           offset += sprintf(message + offset, COLOR_BOLD);
                           offset += sprintf(message + offset, " to your friends!\n");
                           offset += sprintf(message + offset, COLOR_RESET);
                           write_client(client->sock, message);

                           found = 1;
                           break;
                        }
                     }
                     if (!found)
                     {
                        write_client(client->sock, COLOR_RED);
                        write_client(client->sock, "This player does not exist!\n");
                        write_client(client->sock, COLOR_RESET);
                     }
                  }

                  // Retirer un ami
                  else if (strncmp(buffer, "/unfriend", 9) == 0)
                  {
                     // On vérifie que le client n'a pas juste envoyé /unfriend
                     if (strlen(buffer) < 11)
                     {
                        write_client(client->sock, COLOR_RED);
                        write_client(client->sock, "Invalid command!\n");
                        write_client(client->sock, "Syntax: /unfriend <player>\n");
                        write_client(client->sock, COLOR_RESET);
                        break;
                     }
                     char *player = buffer + 10;

                     // Un client ne peut pas se retirer lui-même de sa liste d'amis
                     if (strcmp(player, client->name) == 0)
                     {
                        write_client(client->sock, COLOR_RED);
                        write_client(client->sock, "You can't remove yourself from your friends!\n");
                        write_client(client->sock, COLOR_RESET);
                        break;
                     }
                     
                     char message[BUF_SIZE];
                     int offset = 0;
                     int found = 0;

                     // Pour retirer un ami, il faut que le joueur soit dans la liste d'amis
                     for (int j = 0; j < nClients; j++)
                     {
                        if (strcmp(clients[j].name, player) == 0)
                        {
                           for (int k = 0; k < MAX_FRIENDS; k++)
                           {
                              if (client->friends[k] == clients[j].sock)
                              {
                                 client->friends[k] = INVALID_SOCKET;
                                 offset += sprintf(message + offset, COLOR_BOLD);
                                 offset += sprintf(message + offset, "\nYou removed ");
                                 offset += sprintf(message + offset, COLOR_RESET);
                                 offset += sprintf(message + offset, COLOR_CYAN);
                                 offset += sprintf(message + offset, player);
                                 offset += sprintf(message + offset, COLOR_RESET);
                                 offset += sprintf(message + offset, COLOR_BOLD);
                                 offset += sprintf(message + offset, " from your friends!\n");
                                 offset += sprintf(message + offset, COLOR_RESET);
                                 write_client(client->sock, message);

                                 found = 2;
                                 break;
                              }
                              found = 1;
                           }
                           break;
                        }
                     }
                     if (!found)
                     {
                        write_client(client->sock, COLOR_RED);
                        write_client(client->sock, "This player does not exist!\n");
                        write_client(client->sock, COLOR_RESET);
                     }
                     else if (found == 1)
                     {
                        write_client(client->sock, COLOR_RED);
                        write_client(client->sock, "This player is not in your friends list!\n");
                        write_client(client->sock, COLOR_RESET);
                     }
                  }

                  // Voir sa liste d'ami
                  else if (strcmp(buffer, "/listfriends") == 0)
                  {
                     write_client(client->sock, COLOR_BOLD);
                     write_client(client->sock, "\nList of friends:\n");
                     write_client(client->sock, COLOR_RESET);

                     char friendsList[BUF_SIZE];
                     int offset = 0;

                     for (int j = 0; j < MAX_FRIENDS; j++)
                     {
                        if (client->friends[j] != INVALID_SOCKET)
                        {
                           offset += sprintf(friendsList + offset, COLOR_CYAN);
                           offset += sprintf(friendsList + offset, clients[j].name);
                           offset += sprintf(friendsList + offset, "\n");
                           offset += sprintf(friendsList + offset, COLOR_RESET);
                        }
                     }
                     if (offset == 0)
                     {
                        offset += sprintf(friendsList + offset, COLOR_RED);
                        offset += sprintf(friendsList + offset, "You have no friends!\n");
                        offset += sprintf(friendsList + offset, COLOR_RESET);
                     }

                     write_client(client->sock, friendsList);
                  }

                  // Chuchoter un message a qqn
                  else if (strncmp(buffer, "/whisper", 8) == 0)
                  {
                     // On vérifie que le client n'a pas juste envoyé /whisper
                     if (strlen(buffer) < 10)
                     {
                        write_client(client->sock, COLOR_RED);
                        write_client(client->sock, "Invalid command!\n");
                        write_client(client->sock, "Syntax: /whisper <player> <message>\n");
                        write_client(client->sock, COLOR_RESET);
                        break;
                     }  
                     char *player = buffer + 9;
                     char *message = strchr(player, ' ') + 1;
                     *strchr(player, ' ') = '\0';
                     char whisperMessage[BUF_SIZE];
                     int offset = 0;
                     int found = 0;

                     // Un client ne peut pas se chuchoter à lui-même
                     if (strcmp(player, client->name) == 0)
                     {
                        write_client(client->sock, COLOR_RED);
                        write_client(client->sock, "You can't whisper to yourself!\n");
                        write_client(client->sock, COLOR_RESET);
                        break;
                     }

                     for (int j = 0; j < nClients; j++)
                     {
                        if (strcmp(clients[j].name, player) == 0)
                        {
                           offset += sprintf(whisperMessage + offset, COLOR_BOLD);
                           offset += sprintf(whisperMessage + offset, COLOR_CYAN);
                           offset += sprintf(whisperMessage + offset, client->name);
                           offset += sprintf(whisperMessage + offset, COLOR_RESET);
                           offset += sprintf(whisperMessage + offset, COLOR_BOLD);
                           offset += sprintf(whisperMessage + offset, " whispered: ");
                           offset += sprintf(whisperMessage + offset, COLOR_RESET);
                           offset += sprintf(whisperMessage + offset, message);
                           offset += sprintf(whisperMessage + offset, "\n");

                           write_client(clients[j].sock, whisperMessage);

                           whisperMessage[0] = '\0';
                           offset = 0;

                           offset += sprintf(whisperMessage + offset, COLOR_BOLD);
                           offset += sprintf(whisperMessage + offset, "To ");
                           offset += sprintf(whisperMessage + offset, COLOR_RESET);
                           offset += sprintf(whisperMessage + offset, COLOR_CYAN);
                           offset += sprintf(whisperMessage + offset, player);
                           offset += sprintf(whisperMessage + offset, COLOR_RESET);
                           offset += sprintf(whisperMessage + offset, ": ");
                           offset += sprintf(whisperMessage + offset, message);
                           offset += sprintf(whisperMessage + offset, "\n");

                           write_client(client->sock, whisperMessage);

                           found = 1;
                           break;
                        }
                     }
                     if (!found)
                     {
                        write_client(client->sock, COLOR_RED);
                        write_client(client->sock, "This player does not exist!\n");
                        write_client(client->sock, COLOR_RESET);
                     }
                  }

                  /* help : affiche les commandes */
                  else if (strcmp(buffer, "/help") == 0)
                  {
                     char help[BUF_SIZE];
                     int offset = 0;

                     offset += sprintf(help + offset, COLOR_BOLD);
                     offset += sprintf(help + offset, "Commands:\n");

                     offset += sprintf(help + offset, COLOR_CYAN);

                     offset += sprintf(help + offset, "/list : List all players\n");
                     offset += sprintf(help + offset, "/challenge <player> : Challenge a player\n");
                     offset += sprintf(help + offset, "/pchallenge <player> : Challenge a player for a private game\n");
                     offset += sprintf(help + offset, "/fgames : List all finished games\n");
                     offset += sprintf(help + offset, "/replay <game index> : Replay a game\n");
                     offset += sprintf(help + offset, "/games : List all unfinished games\n");
                     offset += sprintf(help + offset, "/spectate <game index> : Spectate a game\n");
                     offset += sprintf(help + offset, "/bio <bio> : Write a bio\n");
                     offset += sprintf(help + offset, "/seebio <player> : See the bio of a player\n");
                     offset += sprintf(help + offset, "/friend <player> : Add a friend\n");
                     offset += sprintf(help + offset, "/unfriend <player> : Remove a friend\n");
                     offset += sprintf(help + offset, "/listfriends : List all friends\n");
                     offset += sprintf(help + offset, "/whisper <player> <message> : Whisper a message to a player\n");
                     offset += sprintf(help + offset, "/exit : Exit the server\n");
                     offset += sprintf(help + offset, "/help : Display this help\n");

                     offset += sprintf(help + offset, COLOR_RESET);

                     write_client(client->sock, help);
                  }

                  /* exit */
                  else if (strcmp(buffer, "/exit") == 0)
                  {
                     // On informe les autres joueurs
                     char exitMessage[BUF_SIZE];
                     int offset = 0;
                     offset += sprintf(exitMessage + offset, COLOR_BOLD);
                     offset += sprintf(exitMessage + offset, COLOR_YELLOW);
                     offset += sprintf(exitMessage + offset, client->name);
                     offset += sprintf(exitMessage + offset, " has left the server!\n");
                     offset += sprintf(exitMessage + offset, COLOR_RESET);
                     send_message_to_all_clients(clients, client, nClients, exitMessage, 1);

                     write_client(client->sock, COLOR_BOLD);
                     write_client(client->sock, COLOR_YELLOW);
                     write_client(client->sock, "Goodbye!\n");
                     write_client(client->sock, COLOR_RESET);
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
   clear_games(games, nGames);
   end_connection(sock);
}

static void clear_clients(Client *clients, int nClients)
{
   int i = 0;
   for(i = 0; i < nClients; i++)
   {
      closesocket(clients[i].sock);
   }
   free(clients);
}

static void clear_games(Game *games, int nGames)
{
   int i = 0;
   for(i = 0; i < nGames; i++)
   {
      closesocket(games[i].player1->sock);
      closesocket(games[i].player2->sock);
      free(games[i].spectators);
      free(games[i].board);
      free(games[i].total_seeds_collected);
      free(games[i].player1);
      free(games[i].player2);
   }
   free(games);
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
   char message[BUF_SIZE];
   int offset = 0;

   for (int i = 0; i < nClients; i++)
   {
      if (clients[i].sock != sender->sock && !clients[i].playing && !clients[i].spectating && !clients[i].reviewing && clients[i].receiveChallenge == 0)
      {
         if (from_server)
         {
            offset += sprintf(message + offset, COLOR_YELLOW);
            offset += sprintf(message + offset, buffer);
            offset += sprintf(message + offset, COLOR_RESET);
         }
         else
         {
            offset += sprintf(message + offset, COLOR_BOLD);
            offset += sprintf(message + offset, COLOR_CYAN);
            offset += sprintf(message + offset, sender->name);
            offset += sprintf(message + offset, COLOR_RESET);
            offset += sprintf(message + offset, COLOR_BOLD);
            offset += sprintf(message + offset, ": ");
            offset += sprintf(message + offset, COLOR_RESET);
            offset += sprintf(message + offset, buffer);
         }
         write_client(clients[i].sock, message);
         message[0] = '\0';
         offset = 0;
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

   offset += sprintf(display + offset, COLOR_BOLD);

   offset += sprintf(display + offset, "\n\n\n");
   offset += sprintf(display + offset, "Turn %d\n", game->turn);
   offset += sprintf(display + offset, "\n");

   if (game->turn % 2 == 0)
   {
      //Affichage des coordonnées des cases du haut
      offset += sprintf(display + offset, COLOR_YELLOW);

      for (int i = N_PITS - 1; i >= N_PITS / 2; i--)
      {
         if (i >= 10)
         {
            offset += sprintf(display + offset, "  %d  ", i);
         }
         else
         {
            offset += sprintf(display + offset, "  %d   ", i);
         }
      }

      offset += sprintf(display + offset, "\n");

      // Affichage du board

      offset += sprintf(display + offset, COLOR_CYAN);
   
      for (int i = N_PITS - 1; i >= N_PITS / 2; i--)
      {
         if (game->moves[game->turn - 1] == i)
         {
            offset += sprintf(display + offset, COLOR_MAGENTA);
         }
         offset += sprintf(display + offset, "[ %d ] ", game->board[i]);
         offset += sprintf(display + offset, COLOR_CYAN);
      }
      offset += sprintf(display + offset, "\n");
      for (int i = 0; i < N_PITS / 2; i++)
      {
         offset += sprintf(display + offset, "[ %d ] ", game->board[i]);
      }
      offset += sprintf(display + offset, "\n");

      offset += sprintf(display + offset, COLOR_YELLOW);

      // Affichage des coordonnées des cases du bas
      for (int i = 0; i < N_PITS / 2; i++)
      {
         offset += sprintf(display + offset, "  %d   ", i);
      }
      offset += sprintf(display + offset, "\n\n");

      offset += sprintf(display + offset, COLOR_RESET);
      offset += sprintf(display + offset, COLOR_BOLD);
      // Affichage des graines collectées
      offset += sprintf(display + offset, "Seeds collected by you : %d\n", game->total_seeds_collected[0]);
      offset += sprintf(display + offset, "Seeds collected by %s : %d\n", game->player2->name, game->total_seeds_collected[1]);
   }
   // Si on affiche pour le joueur 2, on affiche le board à l'envers
   else
   {

      //Affichage des coordonnées des cases du haut

      offset += sprintf(display + offset, COLOR_YELLOW);

      for (int i = N_PITS -1 ; i >= N_PITS / 2; i--)
      {
         if (i >= 10)
         {
            offset += sprintf(display + offset, "  %d  ", i);
         }
         else
         {
            offset += sprintf(display + offset, "  %d   ", i);
         }
      }

      offset += sprintf(display + offset, "\n");

      // Affichage du board

      offset += sprintf(display + offset, COLOR_CYAN);

      for (int i = N_PITS / 2 - 1; i >= 0; i--)
      {
         if (game->moves[game->turn - 1] == i)
         {
            offset += sprintf(display + offset, COLOR_MAGENTA);
         }
         offset += sprintf(display + offset, "[ %d ] ", game->board[i]);
         offset += sprintf(display + offset, COLOR_CYAN);
      }
      offset += sprintf(display + offset, "\n");
      for (int i = N_PITS / 2; i < N_PITS; i++)
      {
         offset += sprintf(display + offset, "[ %d ] ", game->board[i]);
      }
      offset += sprintf(display + offset, "\n");
      
      //Affichage des coordonnées des cases

      offset += sprintf(display + offset, COLOR_YELLOW);

      for (int i = 0; i < N_PITS / 2; i++)
      {
         offset += sprintf(display + offset, "  %d   ", i);
      }
      offset += sprintf(display + offset, "\n\n");

      // Affichage des graines collectées

      offset += sprintf(display + offset, COLOR_RESET);
      offset += sprintf(display + offset, COLOR_BOLD);

      offset += sprintf(display + offset, "Seeds collected by %s : %d\n", game->player1->name, game->total_seeds_collected[0]);
      offset += sprintf(display + offset, "Seeds collected by you : %d\n", game->total_seeds_collected[1]);
   }
   offset += sprintf(display + offset, COLOR_RESET);

   offset += sprintf(display + offset, "\n\n");
   // Envoi du buffer au client
   write_client(game->turn % 2 == 0 ? game->player1->sock : game->player2->sock, display);
}

static void displayBoardToSpectator(Game *game)
{
   char display[BUF_SIZE * 4]; // Un grand buffer pour accumuler l'affichage
   int offset = 0; // Position d'écriture dans le buffer

   // Affichage de qui a jouer et quoi

   offset += sprintf(display + offset, "\n\n\n");
   offset += sprintf(display + offset, COLOR_BOLD);   

   offset += sprintf(display + offset, "Turn %d\n\n", game->turn);
   if (game->turn > 0)
   {
      offset += sprintf(display + offset, "%s played in pit %d\n\n", game->turn % 2 == 0 ? game->player2->name : game->player1->name, game->moves[game->turn - 1]);
   }

   //Affichage des coordonnées des cases du haut

   offset += sprintf(display + offset, COLOR_YELLOW);

   for (int i = N_PITS - 1; i >= N_PITS / 2; i--)
   {
      if (i >= 10)
      {
         offset += sprintf(display + offset, "  %d  ", i);
      }
      else
      {
         offset += sprintf(display + offset, "  %d   ", i);
      }
   }
   offset += sprintf(display + offset, "\n");

   // Affichage du board

   offset += sprintf(display + offset, COLOR_CYAN);

   for (int i = N_PITS - 1; i >= N_PITS / 2; i--)
   {
      if (game->moves[game->turn - 1] == i)
      {
         offset += sprintf(display + offset, COLOR_MAGENTA);
      }
      offset += sprintf(display + offset, "[ %d ] ", game->board[i]);
      offset += sprintf(display + offset, COLOR_CYAN);
   }
   offset += sprintf(display + offset, "\n");
   for (int i = 0; i < N_PITS / 2; i++)
   {
      if (game->moves[game->turn - 1] == i)
      {
         offset += sprintf(display + offset, COLOR_MAGENTA);
      }
      offset += sprintf(display + offset, "[ %d ] ", game->board[i]);
      offset += sprintf(display + offset, COLOR_CYAN);
   }
   offset += sprintf(display + offset, "\n");

   //Affichage des coordonnées des cases

   offset += sprintf(display + offset, COLOR_YELLOW);

   for (int i = 0; i < N_PITS / 2; i++)
   {
      offset += sprintf(display + offset, "  %d  ", i);
   }
   offset += sprintf(display + offset, "\n\n");

   // Affichage des graines collectées

   offset += sprintf(display + offset, COLOR_RESET);
   offset += sprintf(display + offset, COLOR_BOLD);

   offset += sprintf(display + offset, "Seeds collected by %s : %d\n", game->player1->name, game->total_seeds_collected[0]);
   offset += sprintf(display + offset, "Seeds collected by %s : %d\n", game->player2->name, game->total_seeds_collected[1]);

   offset += sprintf(display + offset, COLOR_RESET);

   offset += sprintf(display + offset, "\n\n");

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

      offset += sprintf(display + offset, COLOR_BOLD);
      offset += sprintf(display + offset, "Turn %d\n", i + 1);
      if (i > 0)
      {
         offset += sprintf(display + offset, "%s played in pit %d\n", i % 2 == 0 ? game->player2->name : game->player1->name, game->moves[i - 1]);
      }

      offset += sprintf(display + offset, "\n");

      // Affichage des coordonnées des cases du haut

      offset += sprintf(display + offset, COLOR_YELLOW);

      for (int j = N_PITS - 1; j >= N_PITS / 2; j--)
      {
         if (j >= 10)
         {
            offset += sprintf(display + offset, "  %d  ", j);
         }
         else
         {
            offset += sprintf(display + offset, "  %d   ", j);
         }
      }
      offset += sprintf(display + offset, "\n");

      // Affichage du board

      offset += sprintf(display + offset, COLOR_CYAN);

      for (int j = N_PITS - 1; j >= N_PITS / 2; j--)
      {
         if (game->moves[i] == j)
         {
            offset += sprintf(display + offset, COLOR_MAGENTA);
         }
         offset += sprintf(display + offset, "[ %d ] ", board[j]);
         offset += sprintf(display + offset, COLOR_CYAN);
      }
      offset += sprintf(display + offset, "\n");
      for (int j = 0; j < N_PITS / 2; j++)
      {
         if (game->moves[i] == j)
         {
            offset += sprintf(display + offset, COLOR_MAGENTA);
         }
         offset += sprintf(display + offset, "[ %d ] ", board[j]);
         offset += sprintf(display + offset, COLOR_CYAN);
      }
      offset += sprintf(display + offset, "\n");

      //Affichage des coordonnées des cases

      offset += sprintf(display + offset, COLOR_YELLOW);

      for (int j = 0; j < N_PITS / 2; j++)
      {
         offset += sprintf(display + offset, "  %d   ", j);
      }
      offset += sprintf(display + offset, "\n\n");
      
      // Affichage des graines collectées

      offset += sprintf(display + offset, "Seeds collected by %s : %d\n", game->player1->name, total_seeds_collected[0]);
      offset += sprintf(display + offset, "Seeds collected by %s : %d\n", game->player2->name, total_seeds_collected[1]);

      offset += sprintf(display + offset, COLOR_RESET);
      // Envoi du buffer au client

      write_client(client->sock, display);

      // On attend 1 seconde entre chaque affichage
      sleep(1);

   }

   // On indique au client que le replay est terminé

   write_client(client->sock, COLOR_BOLD);
   write_client(client->sock, "Replay finished!\n");
   write_client(client->sock, COLOR_RESET);
}

int main()
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
