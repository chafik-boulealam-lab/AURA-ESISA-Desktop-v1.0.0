# Instructions de release — AURA ESISA

## Build + package (recommande)

```powershell
cd C:\Users\ok\Downloads\PFA
powershell -ExecutionPolicy Bypass -File build.ps1
powershell -ExecutionPolicy Bypass -File scripts\package_release.ps1
```

Produit :
- `bin\bin\` — dossier de run local
- `AURA-ESISA\` — dossier distributable
- `AURA-ESISA.zip` — archive a partager

## Lancement pour l'utilisateur final

1. Extraire `AURA-ESISA.zip`
2. Double-cliquer **`AURA.bat`**
3. Configurer `config\aura.cfg` avec `groq_api_key=...` (ou variable `AURA_API_KEY`)

## Debug

```bat
AURA.bat /debug
```

## Structure `bin\bin\` (ou release)

```
AURA-ESISA/
├── AURA.exe
├── AURA.bat
├── Lancer_AURA.bat
├── *.dll
├── cacert.pem
├── assets/
├── config/aura.cfg
├── data/questions.csv
└── data/local.db
```

## CI / compilation seule

```bash
mingw32-make all
cd tests && mingw32-make test
```

## Icone (optionnel)

Placer `rsc/aura.ico` et ajouter windres au Makefile pour embarquer l'icone dans l'exe.