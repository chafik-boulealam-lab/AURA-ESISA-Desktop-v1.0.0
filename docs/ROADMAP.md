# Feuille de route — AURA ESISA (2026)

## Etat actuel (fait)

- Application desktop GTK3 + C
- Auth SQLite (inscription, connexion, verification e-mail)
- Entretiens : 3 matieres, 4 niveaux, banque CSV + anti-repetition
- Evaluation IA Groq (libcurl + cJSON) + fallback local
- Dashboard, classement filtre, rapports PDF
- Build Windows : `build.ps1` → `bin\bin\`
- Lanceurs : `AURA.bat`, mode `/debug`, `launch_aura.ps1`
- CI GitHub Actions (make + tests)

## Avant soutenance

- [ ] Demo live preparee (parcours 2 min)
- [ ] Video LinkedIn (script : `docs/LINKEDIN_VIDEO.md`)
- [ ] Package release : `scripts\package_release.ps1`
- [ ] Verifier cle Groq sur machine de demo

## Ameliorations court terme

- Icone application (`rsc/aura.ico` + windres)
- Assets reels (logo, intro) a la place des placeholders
- Traduction complete FR (ecran legacy `main.c` si encore utilise)
- Tests supplementaires (question_bank, auth)

## Perspectives

- Sync cloud (Supabase) pour classement multi-machines
- Generateur de questions IA a la volee
- Installateur Windows (Inno Setup / NSIS)
- Distribution Itch.io

## Equipe

| Role | Perimetre |
|------|-----------|
| UI / GTK | `login_ui.c`, `dashboard_ui.c`, theme |
| Backend C | `api.c`, `db.c`, `interview.c`, `report.c` |
| Build / release | `build.ps1`, `Makefile`, CI |
| Doc / soutenance | `docs/`, README |