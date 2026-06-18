# Script vidéo LinkedIn — AURA ESISA (60–90 secondes)

## Format recommandé

- **Durée :** 60–90 s (max 2 min)
- **Format :** 1080p, capture d'écran + voix off ou face caméra en intro
- **Outil :** OBS Studio (gratuit) ou enregistrement Windows (Win + G)
- **Sous-titres :** Ajoutez-les dans LinkedIn à la publication

---

## Storyboard (6 plans)

### Plan 1 — Accroche (0–10 s)

**À l'écran :** Logo / écran de login AURA  
**Voix off :**

> « Vous préparez un entretien technique en informatique ? AURA, notre simulateur ESISA, vous entraîne comme en conditions réelles. »

### Plan 2 — Choix matière/niveau (10–25 s)

**À l'écran :** Dashboard → Lancer entretien → Architecture / Algorithmique / C → Junior à Expert  
**Voix off :**

> « Trois matières, quatre niveaux, plus de 400 questions par combinaison — sans répétition grâce à notre banque SQLite. »

### Plan 3 — Réponse + Valider (25–45 s)

**À l'écran :** Question affichée → taper une réponse → cliquer **Valider** → badge VRAI ou FAUX  
**Voix off :**

> « L'IA Groq évalue chaque réponse instantanément : vrai, faux ou partiel, avec un feedback détaillé. »

### Plan 4 — Classement (45–55 s)

**À l'écran :** Page Classement avec filtres matière + niveau  
**Voix off :**

> « Suivez votre progression et comparez-vous aux autres étudiants avec un classement filtrable. »

### Plan 5 — PDF (55–70 s)

**À l'écran :** Terminer entretien → popup PDF → rapport ouvert  
**Voix off :**

> « Chaque session génère un rapport PDF exportable pour votre portfolio ou votre préparation de soutenance. »

### Plan 6 — Call to action (70–90 s)

**À l'écran :** README GitHub ou lien dépôt  
**Voix off :**

> « Projet de fin d'année ESISA — stack C, GTK3, SQLite et IA. Code open source sur GitHub. #ESISA #PFA #IA #EntretienTech »

---

## Texte de publication LinkedIn (à copier)

```
🎓 PFA ESISA — AURA Interview Simulator

Nous avons développé un simulateur d'entretiens techniques en C pour se préparer aux oraux :

✅ 400+ questions / matière / niveau (Architecture, Algorithmique, Programmation C)
✅ Évaluation IA Groq — VRAI / FAUX en temps réel
✅ Classement avec filtres par matière et niveau
✅ Rapport PDF par session
✅ Interface GTK3 + SQLite

Stack : C · GTK3 · SQLite · Groq API · libcurl

🔗 GitHub : [votre lien]
🎥 Démo ci-dessous

#ESISA #PFA #Informatique #IA #Entretien #CProgramming #OpenSource
```

---

## Checklist avant publication

- [ ] Masquer la clé API dans `config/aura.cfg` (jamais visible à l'écran)
- [ ] Utiliser un compte démo (pas de vraies données personnelles)
- [ ] Vérifier que le PDF s'ouvre correctement
- [ ] Ajouter le lien GitHub dans le post
- [ ] Taguer ESISA et coéquipiers si pertinent

## Enregistrement rapide (OBS)

1. Scène : Capture fenêtre AURA (1920×1080)
2. Micro activé pour voix off
3. Enregistrer 2–3 prises, garder la meilleure
4. Export MP4 → Upload LinkedIn natif (meilleure portée que lien YouTube externe)