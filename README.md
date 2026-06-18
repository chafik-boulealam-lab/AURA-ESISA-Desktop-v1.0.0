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
|---|---|
| Language | C |
| Desktop UI | GTK3 |
| HTTP | libcurl |
| JSON | cJSON |
| Database | SQLite3 |
| AI | Groq API (llama-3.1-8b-instant) |
| Build | Make + GCC + PowerShell scripts |

## Architecture
The project is organized around a production desktop app and an optional web prototype.

```text
[ Desktop App ]
apps/desktop/src/main.c
  -> ui/       (login_ui, dashboard_ui, startup)
  -> core/     (interview, evaluation, api, db, auth, report, questions)
  -> infra/    (filesystem, config, launcher, assets)

[ Web Prototype (optional) ]
apps/web/client
apps/web/server
```

Data flow:
```text
User answer -> AI request (libcurl) -> JSON parse (cJSON)
          -> verdict + score -> store in SQLite -> dashboard/report
```

## Prerequisites
Windows (MSYS2 MinGW64):

```bash
pacman -S --noconfirm mingw-w64-x86_64-gtk3 mingw-w64-x86_64-pkg-config \
  mingw-w64-x86_64-curl mingw-w64-x86_64-sqlite3 mingw-w64-x86_64-gcc make
```

## Setup
Set your API key (recommended via environment variable):

```powershell
$env:AURA_API_KEY="your_key_here"
```

Or use config file:

```ini
config/aura.cfg
groq_api_key=votre_cle_groq
```

## Build
```powershell
powershell -ExecutionPolicy Bypass -File scripts\build.ps1
```

## Usage
Launch:

```bat
AURA.bat
```

Debug mode:

```bat
AURA.bat /debug
```

PowerShell launcher:

```powershell
.\scripts\launch_aura.ps1
.\scripts\launch_aura.ps1 -Debug
```

## Tests
```bash
mingw32-make -C apps/desktop/tests test
```

## Release Packaging
```powershell
powershell -ExecutionPolicy Bypass -File scripts\package_release.ps1
```

Output:
- AURA-ESISA/
- AURA-ESISA.zip

## Project Structure
```text
aura/
|- apps/
|  |- desktop/   # production app
|  |- web/       # optional prototype
|- assets/
|- config/
|- data/
|- docs/
|- scripts/
|- AURA.bat
|- Makefile
```

## Documentation
- [Guide simulateur](docs/SIMULATOR_GUIDE.md)
- [Soutenance](docs/SOUTENANCE.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Release instructions](docs/RELEASE_INSTRUCTIONS.md)

## Organisation
Published under GitHub organization: chafik-boulealam-lab

## Team
- Laarichi Ayoub
  - LinkedIn: https://www.linkedin.com/in/ayoub-laarichi-833425361
- Chiboub Taha Adnane
  - LinkedIn: https://www.linkedin.com/in/taha-adnane-chiboub-1a5ab939a/
- Abdleziz Khoungi
  - Email: a.khoungi@esisa.ac.ma
  - LinkedIn: https://www.linkedin.com/in/abdelaziz-khoungui-428355397/

## License
Academic project developed at ESISA for the End-of-Semester C Project. Not licensed for commercial use.

## Acknowledgments
- ESISA - Ecole Superieure d'Ingenierie en Sciences Appliquees
- Prof. Mehdi Iraqi Houssaini - Course instructor
- Prof. Chafik Boulealam - Project supervision