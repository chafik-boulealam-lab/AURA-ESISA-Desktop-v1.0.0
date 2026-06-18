Feuille de Route et Prompts - Projet Aura-CLI


👥 Répartition des Rôles

PM (Moi) : Gère le GitHub, le README, l'architecture, la documentation et l'assemblage (Merge).

Dev 1 (Système) : Gère le terminal, l'affichage (main.c), le Makefile, et la base de données (SQLite).

Dev 2 (Réseau/IA) : Gère la connexion avec l'API (libcurl) et le parsing (cJSON).


📅 Planning Hebdomadaire

Semaine 1 : Squelette, Makefile, Menu CLI interactif, Test API sur Postman, Docs.

Semaine 2 : Intégration libcurl et cJSON (Faire parler l'IA en C).

Semaine 3 : Intégration SQLite (Sauvegarde des scores) et gestion des erreurs (crash, réseau).

Semaine 4 : Tests finaux, Vidéo de démonstration, Nettoyage du code, Présentation.


🤖 Bibliothèque de Prompts (Code Vibing)

Voici les prompts à utiliser avec l'IA pour générer le code :
Pour le Makefile : "Écris un Makefile propre. Sources dans src/, headers dans include/, obj/, bin/. Ajoute -lcurl et -lsqlite3."

Pour le Menu : "Crée main.c avec un menu interactif CLI en couleurs (ANSI). Boucle infinie, switch/case."

Pour l'API : "Écris une fonction C avec libcurl qui fait un POST vers l'API. Lis la clé depuis getenv()."

Pour cJSON : "Écris une fonction qui parse ce JSON [exemple] avec cJSON et extrait le 'content'."

Pour SQLite : "Écris des fonctions pour init_db(), insert_score() et afficher_leaderboard() avec sqlite3."