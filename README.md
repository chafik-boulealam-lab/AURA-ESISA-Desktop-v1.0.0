# AURA - ESISA Interview Simulator (PFA)

## Overview
AURA is a desktop interview simulator built for ESISA training. It helps students practice technical interviews across three subjects with AI-assisted evaluation and local performance tracking.

Covered subjects:
- Architecture des ordinateurs
- Algorithmique
- Programmation C

## Features
- Desktop GTK interface (login, dashboard, interview flow)
- AI feedback and scoring via Groq API
- Local fallback evaluation when AI is unavailable
- SQLite persistence for users, sessions, scores, and reports
- Question bank from CSV + anti-repetition logic
- Session report export (PDF/TXT fallback)
- Packaged Windows release with launcher and runtime DLLs

## Demo Flow
```text
1) User logs in
2) User starts interview (domain + level)
3) AURA asks questions (bank + optional AI)
4) User answers
5) AURA evaluates answers and computes score
6) Dashboard displays stats + generated report
```

## Tech Stack
| Layer | Technology |
# AURA - ESISA Interview Simulator (PFA)

Simulateur desktop d'entretiens techniques pour les etudiants ESISA.

Matieres couvertes:
- Architecture des ordinateurs
- Algorithmique
- Programmation C

## Fonctionnalites
- Interface desktop GTK (connexion, tableau de bord, session d'entretien)
- Evaluation des reponses avec Groq API
- Evaluation locale de secours si l'API est indisponible
- Sauvegarde locale avec SQLite (sessions, scores, rapports)
- Banque de questions CSV avec mecanisme anti-repetition
- Export de rapport de session (PDF avec fallback TXT)

## Structure du projet
```text
aura/
|- apps/
|  |- desktop/   # application principale
|  |- web/       # prototype optionnel
|- assets/
|- config/
|- data/
|- docs/
|- scripts/
|- AURA.bat
|- Makefile
```

## Prerequis (Windows / MSYS2 MinGW64)
```bash
pacman -S --noconfirm mingw-w64-x86_64-gtk3 mingw-w64-x86_64-pkg-config \
  mingw-w64-x86_64-curl mingw-w64-x86_64-sqlite3 mingw-w64-x86_64-gcc make
```

## Configuration API
Option recommandee (variable d'environnement):

```powershell
$env:AURA_API_KEY="your_key_here"
```

Option fichier config:

```ini
config/aura.cfg
groq_api_key=votre_cle_groq
```

## Build
```powershell
powershell -ExecutionPolicy Bypass -File scripts\build.ps1
```

## Lancement
```bat
AURA.bat
```

Mode debug:

```bat
AURA.bat /debug
```

## Tests
```bash
mingw32-make -C apps/desktop/tests test
```

## Packaging release
```powershell
powershell -ExecutionPolicy Bypass -File scripts\package_release.ps1
```

Sortie:
- AURA-ESISA/
- AURA-ESISA.zip

## Documentation
- [Guide simulateur](docs/SIMULATOR_GUIDE.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Release instructions](docs/RELEASE_INSTRUCTIONS.md)

## Organisation
Projet publie sous l'organisation GitHub: chafik-boulealam-lab

## Team
- Laarichi Ayoub
  - LinkedIn: https://www.linkedin.com/in/ayoub-laarichi-833425361
- Chiboub Taha Adnane
  - LinkedIn: https://www.linkedin.com/in/taha-adnane-chiboub-1a5ab939a/
- Abdleziz Khoungi
  - LinkedIn: https://www.linkedin.com/in/abdelaziz-khoungui-428355397/

## License
Academic project developed at ESISA for the End-of-Semester C Project. Not licensed for commercial use.

## Acknowledgments
- ESISA - Ecole Superieure d'Ingenierie en Sciences Appliquees
- Prof. Mehdi Iraqi Houssaini - Course instructor
- Prof. Chafik Boulealam - Project supervision