# AURA — ESISA Interview Simulator (PFA)

Simulateur d'entretiens techniques desktop pour la formation ESISA.  
**Matières :** Architecture des ordinateurs, Algorithmique, Programmation C.  
**Stack :** C, GTK3, SQLite, Groq API (`llama-3.1-8b-instant`), libcurl, cJSON.

## Fonctionnalités

| Module | Description |
|--------|-------------|
| **Banque de questions** | 400+ questions uniques par matière et par niveau (Junior → Expert), anti-répétition par utilisateur |
| **Évaluation IA** | Verdict **VRAI / FAUX / PARTIEL** + score /10 à chaque réponse |
| **Bouton Valider** | Corriger la réponse sans passer à la question suivante |
| **Classement pro** | Filtres par matière et niveau, moyenne, meilleur score, nb d'entretiens |
| **Rapport PDF** | Généré automatiquement à la fin de chaque session (`data/reports/`) |
| **Auth** | Inscription, connexion, vérification email (SQLite) |

## Démarrage rapide (Windows)

### Prérequis (MSYS2 MinGW64)

```bash
pacman -S --noconfirm mingw-w64-x86_64-gtk3 mingw-w64-x86_64-pkg-config \
  mingw-w64-x86_64-curl mingw-w64-x86_64-sqlite3 mingw-w64-x86_64-gcc make
```

### Configuration API Groq

Créez `config/aura.cfg` (non versionné) :

```ini
groq_api_key=votre_cle_groq
```

Ou variable d'environnement : `AURA_API_KEY`.

### Build & lancement

```powershell
cd C:\Users\ok\Downloads\PFA
powershell -ExecutionPolicy Bypass -File build.ps1
```

Puis double-cliquez : **`AURA.bat`** (racine du projet)

En cas de probleme :

```bat
AURA.bat /debug
REM ou Lancer_AURA.bat (equivalent)
```

PowerShell (memes verifications) :

```powershell
.\launch_aura.ps1          # GUI
.\launch_aura.ps1 -Debug   # console
```

## Parcours utilisateur

1. **Connexion** → Dashboard
2. **Lancer un entretien** → Choisir matière → Choisir niveau
3. Répondre → **Valider** (correction sur place) ou **Suivant**
4. **Terminer** → Score SQLite + rapport PDF
5. Consulter **Rapports**, **Classement** (filtres matière/niveau), **Profil**

## Structure du projet

```
PFA/
├── src/           # Code C (UI, API, BDD, banque questions, PDF)
├── include/       # Headers
├── data/          # questions.csv, local.db, reports/
├── config/        # aura.cfg (gitignoré)
├── bin/bin/       # AURA.exe + DLLs (build)
├── docs/          # Soutenance, guide vidéo LinkedIn
├── tests/         # Tests API
└── build.ps1      # Script de build Windows
```

## Release (distribution)

```powershell
powershell -ExecutionPolicy Bypass -File build.ps1
powershell -ExecutionPolicy Bypass -File scripts\package_release.ps1
```

Produit `AURA-ESISA.zip` pret a partager.

## Tests

```bash
cd tests && make test
```

## Équipe & contexte

- **Projet :** PFA ESISA — Simulateur d'entretiens techniques
- **Auteurs :** Chafik Boulealam & équipe lab
- **Dépôt GitHub :** `chafik-boulealam-lab` (voir section ci-dessous)

## Publication GitHub

```bash
cd PFA
git init
git add .
git commit -m "AURA ESISA Interview Simulator — PFA 2026"
git branch -M main
git remote add origin https://github.com/chafik-boulealam-lab/aura-esisa.git
git push -u origin main
```

> Ne jamais committer `config/aura.cfg` ni `bin/bin/config/aura.cfg` (clé API).

## Documentation

- [Guide simulateur](docs/SIMULATOR_GUIDE.md)
- [Soutenance 10 min](docs/SOUTENANCE.md)
- [Script vidéo LinkedIn](docs/LINKEDIN_VIDEO.md)

## Licence

Projet académique ESISA — usage éducatif.