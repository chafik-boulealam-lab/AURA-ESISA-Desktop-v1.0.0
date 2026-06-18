# Guide de soutenance — ESISA Interview Simulator

## Structure recommandee (10 minutes)

### Slide 1 — Titre (30 s)
- Nom du projet, logo ESISA, membres du groupe, encadrant
- **ESISA Interview Simulator** — simulateur d'entretiens techniques

### Slide 2 — Problematique & objectif (1 min 30)
- **Probleme** : les etudiants manquent de pratique avant les entretiens techniques
- **Solution** : application desktop qui simule un jury sur 3 matieres ESISA
- **Objectif** : tester, noter et suivre la progression (Architecture, Algorithmique, C)

### Slide 3 — Vue d'ensemble fonctionnelle (1 min 30)
1. Connexion etudiant (SQLite)
2. Choix matiere → niveau → 5 questions
3. Evaluation IA (Groq) + fallback local
4. Sauvegarde score + rapports + classement

### Slide 4 — Architecture (2 min)
```
[GTK3 Frontend]  ←→  [Backend C]
       |                    |
       |              libcurl + cJSON
       |                    |
       |              [API Groq REST]
       |
  [SQLite local.db]
```

- **Type** : Application Desktop (GTK3 + C)
- **Separation** : UI (`dashboard_ui.c`, `login_ui.c`) / logique (`interview.c`, `api.c`, `db.c`)

### Slide 5 — Stack technique & IA (2 min)
| Couche | Technologie |
|--------|-------------|
| Backend | C (pointeurs, structs, malloc/free) |
| Reseau | libcurl (HTTP POST) |
| JSON | cJSON (parse reponse Groq) |
| BDD locale | SQLite |
| BDD prod | Supabase/MongoDB (roadmap) |
| IA | API Groq (`llama3-8b-8192`) |

**Flux IA** : `soumettre_reponse()` → JSON → Groq → `parse_ai_response()` → `parse_score_from_feedback()`

### Slide 6 — Demo live (2 min)
- Lancer **`AURA.bat`** (ou `launch_aura.ps1`)
- Connexion → Lancer entretien → repondre → score affiche
- Montrer Rapports / Classement / Profil
- Si crash : `AURA.bat /debug` pour voir les erreurs

### Slide 7 — Pipeline & deploiement (1 min)
- Dev : MSYS2 + `build.ps1`
- Tests : `tests/test_api.c` + GitHub Actions
- Package : `scripts\package_release.ps1` → `AURA-ESISA.zip`

### Slide 8 — Limites & perspectives (30 s)
- Dependance API Groq (fallback local)
- Questions statiques CSV + generation IA
- Sync cloud Supabase a venir

## Demo technique C (a preparer)
```bash
cd tests && mingw32-make test
```

## Criteres couverts
- Backend C compilable avec tests
- Integration IA REST + parsing JSON
- SQLite locale
- Separation frontend/backend
- Deploiement desktop (Itch.io)