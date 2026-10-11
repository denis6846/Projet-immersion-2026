# Klosnowo

Implémentation en C du jeu de plateau **Klosnowo**, un jeu de stratégie à deux
joueurs inspiré des jeux de type Tafl (Tablut / Hnefatafl), développé dans le
cadre de l'immersion entreprise 2026 de l'ENSISA (1<sup>re</sup> année
ingénieur).

> Règles originales du jeu © Cyril Meyer, sous licence
> [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/deed.fr).

## Sommaire

- [Contexte](#contexte)
- [Le jeu](#le-jeu)
  - [Règles résumées](#règles-résumées)
- [Équipe](#équipe)
- [Stack technique](#stack-technique)
- [Structure du projet](#structure-du-projet)
- [Dépôt SVN](#dépôt-svn)
- [Compilation](#compilation)
- [Utilisation](#utilisation)
- [Documentation (Doxygen)](#documentation-doxygen)
- [Tests](#tests)
- [Licence](#licence)

---

## Contexte

Ce projet est réalisé dans le cadre de la semaine d'**immersion entreprise**
de première année à l'**ENSISA** (École Nationale Supérieure d'Ingénieurs Sud
Alsace), en groupe. Il consiste à concevoir et développer, en C, un jeu de
plateau jouable :

- en **local** (deux joueurs sur la même machine) ;
- en **réseau** (client / serveur, via sockets TCP).

Le développement se fait en environnement natif sur Ubuntu (pas via le
Vagrantfile fourni par l'encadrement). Le projet couvre l'ensemble d'une
chaîne de développement logicielle en C : gestion de version (SVN),
interface graphique (GTK4), et documentation générée automatiquement
(Doxygen).

## Le jeu

**Klosnowo** se joue sur un plateau rectangulaire de **11 × 7 cases**. Chaque
joueur dispose d'un **roi** (à défendre) et de **9 soldats**. Chaque camp
possède une case "cité" dans son coin du plateau.

### Règles résumées

- **Déplacements** : les unités (roi et soldats) se déplacent en ligne
  horizontale ou verticale, d'autant de cases que souhaité, sans pouvoir
  sauter par-dessus une pièce ni se placer sur une case occupée.
- **Conditions de victoire** :
  - **Conquête** : le roi atteint la cité adverse ;
  - **Exécution** : le roi adverse est capturé ;
  - **Contrôle** (après 64 tours, 32 par joueur) : le joueur ayant le score
    le plus élevé l'emporte (égalité possible).
- **Score de contrôle** : +1 par case contrôlée, +1 par soldat encore en vie
  (le roi ne compte pas). Le contrôle d'une case change lorsqu'une troupe s'y
  déplace ; la case quittée reste contrôlée par le même camp.
- **Captures** : seule une pièce qui vient de se déplacer peut capturer (pas
  de capture passive). Deux mécanismes de capture :
  - **Linca** : capture "sandwich" (la pièce adverse est prise en étau entre
    deux pièces ennemies, ou une pièce ennemie et une barricade) ;
  - **Seultou** : capture "en poussant" (une pièce est repoussée contre un
    obstacle si rien ne la soutient de l'autre côté).
- **Barricade** : avant le premier coup, chaque joueur choisit en secret une
  case de son côté du plateau (diagonale et zone adjacente à son camp
  exclues) qui deviendra un obstacle neutre : non capturable, infranchissable,
  et protégeant les pièces contre certaines captures. Le choix du joueur bleu
  est révélé avant que le joueur rouge ne fasse le sien.

> Le détail complet des règles (illustrations incluses) se trouve dans le
> document `Sujet-Immersion-2026.pdf` fourni par l'encadrement.

## Équipe

Projet réalisé en groupe dans le cadre de l'immersion ENSISA 2026.

- BRODIN Mathias
- EL AISSAOUI Mohammed
- ALLOUCHI Yassine
- MEHDI Yasmine
- RAROG Christophe
- TEMEL Denis

**Encadrement** : Cyril Meyer, Michel Hassenforder

## Stack technique

| Domaine              | Outil / Techno                          |
|-----------------------|------------------------------------------|
| Langage               | C                                        |
| Interface graphique   | GTK 4                                    |
| Réseau                | Sockets BSD, **TCP** (`SOCK_STREAM`)     |
| Documentation         | Doxygen (+ Graphviz pour les graphes)    |
| Gestion de version     | Subversion (SVN)                         |
| Tests unitaires       | `assert()` (C standard), pas de framework externe |
| Environnement de dev  | Ubuntu, en natif (pas de VM Vagrant)     |

## Structure du projet

Arborescence réelle du projet (confirmée via la génération Doxygen) :


.
├── README.md
├── Doxyfile
├── docs/                       # documentation générée (Doxygen), ignorée par svn
│   ├── html/
├── src/
│   ├── main.c
│   ├── GUI/
│   │   └── interface.c
│   ├── IA/
│   │   ├── STUPID_AI.c
│   ├── jeu/
│   │   ├── capture.c
│   │   ├── checkScore.c
│   │   ├── jeu.c
│   │   └── mouvement.c
│   └── network/
│       ├── joueur.c
│       ├── protocole.c
│       └── server.c
└── include/
    ├── GUI/
    │   └── interface.h
    ├── ia/
    │   └── STUPID_AI.h
    ├── jeu/
    │   ├── capture.h
    │   ├── checkScore.h
    │   ├── jeu.h
    │   └── mouvement.h
    └── network/
        ├── joueur.h
        ├── protocole.h
        └── server.h


> ⚠️ Aucun `tests/` n'existe actuellement à la racine
> du projet — en cours par l'équipe.

## Dépôt SVN

Le code source est versionné sur le SVN (gr-4) de l'ENSISA :

```
https://svn.ensisa.uha.fr/scm/svn/imm2627-gr4
```

### Commandes de base


# Récupérer une copie de travail
svn checkout https://svn.ensisa.uha.fr/scm/svn/imm2627-gr4

# Mettre à jour sa copie locale
svn update      # ou : svn up

# Voir l'état des changements locaux
svn status      # ? non suivi | A ajouté | M modifié | D supprimé | ! manquant

# Ajouter un fichier ou dossier au suivi
svn add <fichier|dossier>

# Envoyer ses changements (toujours avec un message clair)
svn commit -m "message décrivant le changement"   # ou : svn ci -m "..."
# Pour ne commit qu'un fichier précis parmi plusieurs modifiés :
svn commit <fichier> -m "message"

# Voir l'historique
svn log

# Annuler des changements locaux non commités
svn revert <fichier>


**Bonnes pratiques imposées par le sujet** :
- des commits **réguliers**, avec un **message utile** ;
- des commits réalisés par **différentes personnes** de l'équipe ;
- **aucun fichier généré** (binaires, `.o`, exécutable, rapports d'analyse,
  fichiers d'IDE, ...) ne doit être versionné — utiliser une propriété
  `svn:ignore` si besoin, par exemple pour la doc générée :
  
  svn propset svn:ignore "html
  latex" docs
  svn propset svn:ignore ".vscode" .

### Gestion des conflits

En cas de conflit lors d'un `svn update` (`C` dans le résultat), inspecter le
conflit avant de choisir :

svn resolve --accept=theirs-full <fichier>   # accepter la version du serveur
svn resolve --accept=mine-full <fichier>     # garder sa propre version
svn resolve --accept=working <fichier>       # après fusion manuelle du fichier

Un conflit peut aussi être **d'arborescence** (ex : un dossier créé en
local et sur le serveur en même temps) : SVN propose alors de **fusionner
les dossiers** (`m`), ce qui est en général le bon choix pour ne rien
perdre.

## Compilation

Le projet utilise **Make** afin d'automatiser la compilation du programme.

### Prérequis

Sur Ubuntu/Debian, installer les outils nécessaires avec :

sudo apt update
sudo apt install build-essential pkg-config libgtk-4-dev

### Commandes de compilation

make
make clean
make fclean
make re
make run-local
make help

## Utilisation

L'exécutable produit doit s'appeler **`game`**. D'après le code de
`src/main.c`, les options suivantes sont actuellement reconnues :

./game -l
# Mode local : les deux joueurs jouent sur la même interface

./game -s <port>
# Mode serveur réseau : écoute sur le port donné

./game -c <ip>:<port>
# Mode client réseau : se connecte au serveur à l'adresse IP:port donnée

./game -s -ia <port>
# Mode serveur réseau IA : écoute sur le port donné

./game -c -ia <ip>:<port>
# Mode client réseau IA : se connecte au serveur à l'adresse IP:port donnée



## Documentation (Doxygen)

La documentation du code est générée automatiquement à partir des
commentaires `/** ... */` (balises `\brief`, `\param`, `\return`, etc.)
présents dans les fichiers sources et d'en-tête.


doxygen Doxyfile


La configuration actuelle (`Doxyfile`) scanne récursivement `src/` et
`include/`, et place la documentation générée dans `docs/` :
- HTML : `docs/html/index.html`
- LaTeX : `docs/latex/` (génère aussi un Makefile local pour produire un PDF
  via `pdflatex`, sans rapport avec la compilation du jeu)

`EXTRACT_ALL` est activé, donc toutes les fonctions apparaissent dans la doc
même sans commentaire — mais le contenu ne sera vraiment utile qu'une fois
les fonctions commentées avec la syntaxe Doxygen (`/** \brief ... */`).

## Tests

Un seul fichier de tests existe actuellement : **`src/IA/test_ia.c`**
(écrit par Mohammed EL AISSAOUI), qui couvre uniquement le module **IA**.
Il ne dépend d'aucun framework externe (pas de Criterion) : les tests sont
de simples fonctions utilisant `assert()` (C standard), lancées depuis un
`main()` qui affiche `Tests IA Klosnowo : OK` si tout passe.

Ce qui est testé :
- `test_coup_legal_initial` : l'IA choisit un coup légal en position de
  départ ;
- `test_conquete_immediate` : l'IA reconnaît et joue un coup de conquête
  (roi atteignant la cité adverse) quand c'est possible ;
- `test_barricade` : l'IA choisit une position de barricade valide.

Compilation et exécution manuelles :

gcc -Iinclude src/IA/test_ia.c src/IA/ia.c src/IA/evaluation.c \
    src/jeu/*.c -o test_ia
./test_ia

## Licence

- Le **code source** de ce projet est réalisé dans un cadre pédagogique
  (immersion ENSISA), sans licence spécifique appliquée.
- Les **règles du jeu Klosnowo** sont © Cyril Meyer, publiées sous licence
  [Creative Commons Attribution 4.0 International (CC BY 4.0)](https://creativecommons.org/licenses/by/4.0/deed.fr).
