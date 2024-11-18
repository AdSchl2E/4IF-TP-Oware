#ifndef CLIENT_H
#define CLIENT_H

#include "server2.h"

typedef struct
{
   SOCKET sock;
   char name[BUF_SIZE];
   char bio[BUF_SIZE];
   int playing;
   int spectating;
   int reviewing;
   int receiveChallenge;
   SOCKET challenger;
   SOCKET* friends;
} Client;

#endif /* guard */
