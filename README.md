# AURA — ESISA Interview Simulator (PFA)

Simulateur d'entretiens techniques desktop pour la formation ESISA.  
**Matières :** Architecture des ordinateurs, Algorithmique, Programmation C.  
**Stack :** C, GTK3, SQLite, Groq API (`llama-3.1-8b-instant`), libcurl, cJSON.

## Structure du projet

```
PFA/
├── apps/
│   ├── desktop/              # Application GTK principale (production)
│   │   ├── src/
│   │   │   ├── ui/           # login_ui, dashboard_ui, startup
│   │   │   ├── core/         # interview, api, db, auth, questions...
│   │   │   ├── infra/        # launcher, filesystem, config, assets
│   │   │   └── legacy/       # stubs non compiles
│   │   ├── include/{ui,core,infra}/
│   │   └── tests/
│   └── web/                  # Prototype HTTP (independant du desktop)
│       ├── server/           # API REST C (port 8080)
│       └── client/           # Frontend HTML/JS
├── assets/                   # Images, sons, fonts
├── config/                   # aura.cfg (gitignore)
├── data/                     # questions.csv, local.db
├── dist/bin/                 # Runtime apres build (exe + DLLs)
├── docs/                     # Documentation
├── scripts/                  # build.ps1, package_release.ps1, launchers
├── AURA.bat                  # Lanceur principal (double-clic)
└── Makefile                  # Orchestrateur (delegue a apps/desktop)
```

## Demarrage rapide (Windows)

### Prerequis (MSYS2 MinGW64)

```bash
pacman -S --noconfirm mingw-w64-x86_64-gtk3 mingw-w64-x86_64-pkg-config \
  mingw-w64-x86_64-curl mingw-w64-x86_64-sqlite3 mingw-w64-x86_64-gcc make
```

### Configuration API Groq

Creez `config/aura.cfg` :

```ini
groq_api_key=votre_cle_groq
```

### Build & lancement

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build.ps1
```

Puis double-cliquez **`AURA.bat`**

Debug console :

```bat
AURA.bat /debug
```

PowerShell :

```powershell
.\scripts\launch_aura.ps1
.\scripts\launch_aura.ps1 -Debug
```

## Release

```powershell
powershell -ExecutionPolicy Bypass -File scripts\package_release.ps1
```

Produit `AURA-ESISA.zip` pret a partager.

## Tests

```bash
mingw32-make -C apps/desktop/tests test
```

## Prototype web (optionnel)

```bash
mingw32-make -C apps/web/server all
# Ouvrir apps/web/client/index.html avec le serveur sur :8080
```

## Documentation

- [Guide simulateur](docs/SIMULATOR_GUIDE.md)
- [Soutenance](docs/SOUTENANCE.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Release](docs/RELEASE_INSTRUCTIONS.md)

## Licence

Projet academique ESISA — usage educatif.

## Organisation

Ce projet est publie sous l'organisation GitHub chafik-boulealam-lab.

## Team

- Laarichi Ayoub
  - Email: a.laarichi@esisa.ac.ma
  - LinkedIn: https://www.linkedin.com/in/ayoub-laarichi-833425361
- Chiboub Taha Adnane
  - LinkedIn: https://www.linkedin.com/in/taha-adnane-chiboub-1a5ab939a/
- Abdleziz Khoungi
  - Email: a.khoungi@esisa.ac.ma

## License

Academic project developed at ESISA for the End-of-Semester C Project. Not licensed for commercial use.

## Acknowledgments

- ESISA — Ecole Superieure d'Ingenierie en Sciences Appliquees
- Prof. Mehdi Iraqi Houssaini — Course instructor
- Prof. Chafik Boulealam — Project supervision