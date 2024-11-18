# 4IF-TP-Oware
Tuto comment jouer et cest qoi et blablabka


On fait la Grand Slam variation
: Such a move is legal, but no capture results. International competitions often follow this rule.


A faire : 


- Ajouter un mode privé: un joueur peut limiter la liste des observateurs à une liste d’amis. Implanter cette fonctionalité. (/friend pseudo) (/challenge pseudo -p) (/listfriends) (/unfriend pseudo) (/whisper pseudo message) (ajouter int private dans struct game)
- refaire l'affiche de la déconnexion
- Probleme avec l'affichage de qui à jouer quoi
- Gérer les cas ou on met rien apres les commandes (/challenge) (/friend) (/unfriend) (/whisper) etc
- Libre à vore imagination: classement des joueurs : article wikipedia sur le classement elo, organisation de tournois, adapter pour un autre jeu etc. (aled)



A tester avec plus de clients :

- Si vous avez une version qui fonctionne pour une partie, Vérifiez que cela fonctionne pour plusieurs parties en jeu.
(A tester avec vraiment beaucoup de clients)
- Challenge croisé : A peut défier B, B peut défier C, C peut défier A. Vérifiez que cela fonctionne.

A améliorer / paufiner :

- Ajouter la fonctionnalité de sauvegarde de partie jouée pour pouvoir la regarder par la suite.
- Affichage console

Done : 

- Implanter le jeu d’Awalé : représentation interne, comment jouer un coup, compter les points,  imprimer l’état du plateau, proposer un draw
- Concevez une application client/serveur. Chaque client s’inscrit avec un pseudo.
- Quand une partie est créée entre A et B, le serveur décide de qui commence par tirage au sort. Le serveur vérifie la légalité des coups joués (en utilisant le code créé pour 0).
- Chaque client peut demander au serveur la liste des pseudos en ligne.
- Un client A peut défier un autre client B. B peut accepter ou refuser.
- Faire en sorte qu'on ne puisse pas jouer sur un 0
- Vous pouvez ajouter comme fonctionalité un listing des parties en cours
- Donnez la possiblité à un joueur d’écrire une bio : disons 10 lignes en ASCII, pour se présenter. On peut demander au serveur de nous afficher la bio d’un pseudo particulier.
- Implanter une option de chat : les joueurs en plus d’envoyer des coups à peuvent échanger des messages pour discuter (dans et en dehors d’une partie). (/msg message)
- Faut ecrire aux specs qui a gagné et qui a perdu / draw et corriger affichage coup précedent et quand c''est fini faut proposer au spec de parrtir
- Un mode “observateur” pour lequel le serveur envoie le plateau et le score à C qui observe la partie entre A et B.




J'ai eu une segfault en faisant un chat avec un spectateur, je sais pas si c'est un cas isolé ou si c'est un bug à corriger