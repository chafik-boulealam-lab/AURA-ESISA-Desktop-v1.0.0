#include "question_seed.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static const char *DOMAINS[] = {
    "Architecture des ordinateurs",
    "Algorithmique",
    "Programmation C"
};
static const char *LEVELS[] = { "Junior", "Intermediate", "Advanced", "Expert" };

static int level_index(const char *level) {
    for (int i = 0; i < 4; i++)
        if (level && strcmp(level, LEVELS[i]) == 0) return i;
    return 0;
}

static int domain_index(const char *domain) {
    for (int i = 0; i < 3; i++)
        if (domain && strcmp(domain, DOMAINS[i]) == 0) return i;
    return 0;
}

static void trim(char *s) {
    if (!s) return;
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
}

static int insert_question(sqlite3 *db, const char *domain, const char *level,
    const char *text, const char *ref, const char *topic) {
    const char *sql =
        "INSERT OR IGNORE INTO question_bank(domain, level, text, reference_answer, topic, source) "
        "VALUES(?, ?, ?, ?, ?, 'seed');";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_text(stmt, 1, domain, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, level, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, text, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, ref, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, topic, -1, SQLITE_TRANSIENT);
    int ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok ? 1 : 0;
}

static void build_arch_question(int li, int idx, char *text, size_t tlen, char *ref, size_t rlen, char *topic, size_t topiclen) {
    static const char *topics[] = {
        "CPU", "RAM", "ROM", "cache L1", "cache L2", "bus systeme", "registres",
        "ALU", "UC", "horloge", "pipeline", "branchement", "interruption", "DMA",
        "memoire virtuelle", "pagination", "MMU", "TLB", "Harvard", "Von Neumann",
        "RISC", "CISC", "multicoeur", "hyperthreading", "SSD", "HDD", "RAID",
        "endianness", "ISA", "fetch-decode-execute", "hazard pipeline", "forwarding",
        "prediction branchement", "coherence cache", "MESI", "bus PCI", "USB",
        "GPIO", "UART", "I2C", "SPI", "FPGA", "ASIC", "SoC", "GPU", "TPU",
        "vectorisation", "SIMD", "registre statut", "mode utilisateur", "mode noyau"
    };
    int ntopics = (int)(sizeof(topics) / sizeof(topics[0]));
    int t = idx % ntopics;
    int v = idx / ntopics;
    snprintf(topic, topiclen, "%s", topics[t]);

    const char *junior[] = {
        "Qu'est-ce que %s et quel est son role dans un ordinateur ?",
        "Definissez le concept de %s en architecture des ordinateurs.",
        "A quoi sert %s dans le fonctionnement d'une machine ?",
        "Donnez une definition simple de %s.",
        "Pourquoi %s est-il important dans un systeme informatique ?",
        "Citez le role principal de %s.",
        "Expliquez en une phrase ce qu'est %s.",
        "Quelle fonction remplit %s ?"
    };
    const char *intermediate[] = {
        "Expliquez le fonctionnement de %s et donnez un exemple concret.",
        "Comparez %s avec un composant voisin dans l'architecture.",
        "Decrivez comment %s interagit avec le processeur et la memoire.",
        "Quels sont les avantages et limites de %s ?",
        "Comment %s influence-t-il les performances globales ?",
        "Illustrez un scenario ou %s est critique.",
        "Quelle est la difference entre %s et une alternative courante ?",
        "Expliquez le cycle d'utilisation de %s lors d'une execution."
    };
    const char *advanced[] = {
        "Analysez l'impact de %s sur la latence et le debit d'un systeme.",
        "Comment optimiser l'utilisation de %s dans un design embarque ?",
        "Quels problemes de coherence ou de contention peut causer %s ?",
        "Decrivez les mecanismes internes de %s au niveau materiel.",
        "Quels trade-offs architecturaux implique %s ?",
        "Comment %s interagit avec le pipeline et les hazards ?",
        "Proposez une strategie de dimensionnement pour %s.",
        "Expliquez les cas limites et erreurs frequentes liees a %s."
    };
    const char *expert[] = {
        "Discutez des evolutions modernes de %s (cloud, edge, HPC).",
        "Comparez %s sur ARM vs x86 avec arguments techniques precis.",
        "Quels risques de securite sont lies a %s et comment les mitiger ?",
        "Comment %s se comporte sous charge massive et contention ?",
        "Analysez %s dans le contexte de la virtualisation et des conteneurs.",
        "Quels benchmarks pertinents pour mesurer %s et pourquoi ?",
        "Expliquez les implications energetiques de %s.",
        "Proposez une architecture hybride exploitant %s pour l'IA embarquee."
    };
    const char **templates;
    switch (li) {
        case 1: templates = intermediate; break;
        case 2: templates = advanced; break;
        case 3: templates = expert; break;
        default: templates = junior; break;
    }
    snprintf(text, tlen, templates[v % 8], topics[t]);
    snprintf(ref, rlen,
        "Reponse attendue sur %s (niveau %s, variante %d): definition precise, "
        "mecanisme, exemple et lien avec CPU/memoire/bus selon le sujet.",
        topics[t], LEVELS[li], v);
}

static void build_algo_question(int li, int idx, char *text, size_t tlen, char *ref, size_t rlen, char *topic, size_t topiclen) {
    static const char *topics[] = {
        "recherche lineaire", "recherche dichotomique", "tri bulle", "tri fusion",
        "tri rapide", "pile", "file", "liste chainee", "arbre binaire", "BST",
        "AVL", "heap", "table de hachage", "graphe oriente", "graphe non oriente",
        "DFS", "BFS", "Dijkstra", "Bellman-Ford", "Floyd-Warshall", "Kruskal",
        "Prim", "programmation dynamique", "glouton", "diviser pour regner",
        "backtracking", "NP-complet", "P vs NP", "complexite O(n)", "memoire auxiliaire",
        "parcours infixe", "parcours prefixe", "tas binaire", "union-find",
        "tri par comptage", "radix sort", "LCS", "knapsack", "edit distance",
        "topological sort", "cycle detection", "matching", "flow max", "A*",
        "heuristique admissible", "amortized analysis", "skip list", "trie",
        "B-tree", "red-black tree"
    };
    int ntopics = (int)(sizeof(topics) / sizeof(topics[0]));
    int t = idx % ntopics;
    int v = idx / ntopics;
    snprintf(topic, topiclen, "%s", topics[t]);

    const char *junior[] = {
        "Quelle est la definition de %s en algorithmique ?",
        "Donnez la complexite typique de %s et expliquez pourquoi.",
        "Citez un cas d'utilisation concret de %s.",
        "Quelle structure de donnees est associee a %s ?",
        "Expliquez %s avec un petit exemple.",
        "Quelle est la difference entre %s et une approche naive ?",
        "Quand utiliser %s plutot qu'une autre methode ?",
        "Definissez %s et son invariant principal."
    };
    const char *intermediate[] = {
        "Implementez mentalement %s: etapes et complexite temporelle/spatiale.",
        "Comparez %s avec une alternative classique.",
        "Tracez %s sur un petit exemple (5 elements).",
        "Quels pieges courants lors de l'implementation de %s ?",
        "Comment prouver la correction de %s ?",
        "Quelle variante iterative/recursive pour %s ?",
        "Analysez le pire et meilleur cas de %s.",
        "Expliquez %s sur un graphe exemple."
    };
    const char *advanced[] = {
        "Optimisez %s pour de grandes donnees: quelles ameliorations ?",
        "Quand %s echoue-t-il et quelles alternatives ?",
        "Analysez amortized vs worst-case pour %s.",
        "Adaptez %s a un contexte contraint memoire.",
        "Combinez %s avec une autre technique: schema et gain.",
        "Probleme ouvert lie a %s: limites connues.",
        "Parallelisez %s: idees et verrous.",
        "Preuve de borne inferieure liee a %s."
    };
    const char *expert[] = {
        "Comparez %s en pratique sur donnees reelles (cache, branchements).",
        "Choix entre %s et approches modernes (GPU, SIMD) pour HPC.",
        "Securite et robustesse: attaques ou degenerescence de %s.",
        "Variantes industrielles de %s (STL, Java Collections).",
        "Design d'API autour de %s pour un moteur de recherche.",
        "Micro-optimisations de %s en C sur x86.",
        "Impact de %s sur latence P99 en production.",
        "Roadmap d'evolution algorithmique autour de %s."
    };
    const char **templates;
    switch (li) {
        case 1: templates = intermediate; break;
        case 2: templates = advanced; break;
        case 3: templates = expert; break;
        default: templates = junior; break;
    }
    snprintf(text, tlen, templates[v % 8], topics[t]);
    snprintf(ref, rlen,
        "Reponse attendue sur %s: definition, complexite, exemple, correction "
        "et comparaison selon niveau %s (variante %d).",
        topics[t], LEVELS[li], v);
}

static void build_c_question(int li, int idx, char *text, size_t tlen, char *ref, size_t rlen, char *topic, size_t topiclen) {
    static const char *topics[] = {
        "pointeurs", "malloc", "free", "calloc", "realloc", "tableaux", "chaines",
        "struct", "union", "enum", "typedef", "fonctions", "recursion", "scope",
        "static", "const", "volatile", "preprocesseur", "macros", "fichiers",
        "stdio", "fprintf", "scanf", "format string", "buffer overflow",
        "undefined behavior", "segmentation fault", "stack", "heap", "passage par valeur",
        "passage par adresse", "double pointeur", "void*", "cast", "sizeof",
        "alignement", "bitwise", "operateurs", "short-circuit", "ternaire",
        "switch", "goto", "headers", "linkage", "extern", "inline", "restrict",
        "VLA", "string.h", "memcpy", "memset", "qsort", "function pointer",
        "callback", "errno"
    };
    int ntopics = (int)(sizeof(topics) / sizeof(topics[0]));
    int t = idx % ntopics;
    int v = idx / ntopics;
    snprintf(topic, topiclen, "%s", topics[t]);

    const char *junior[] = {
        "Qu'est-ce que %s en langage C ?",
        "Donnez un exemple simple illustrant %s.",
        "Pourquoi %s est-il important en programmation C ?",
        "Quelle erreur debutant frequente avec %s ?",
        "Definissez %s et son usage courant.",
        "Quelle syntaxe de base pour %s ?",
        "Quand utiliser %s dans un programme C ?",
        "Expliquez %s en une ou deux phrases."
    };
    const char *intermediate[] = {
        "Expliquez %s avec un extrait de code commente.",
        "Quelle difference entre %s et une approche sans %s ?",
        "Quels risques si %s est mal utilise ?",
        "Comment debugger un probleme lie a %s ?",
        "Bonnes pratiques pour %s en projet reel.",
        "Interaction entre %s et la gestion memoire.",
        "Comparez %s en C89 vs C99 vs C11.",
        "Cas test unitaire pour valider %s."
    };
    const char *advanced[] = {
        "Analysez undefined behavior lie a %s avec exemple.",
        "Optimisations du compilateur autour de %s.",
        "Securite: vulnerabilites classiques via %s.",
        "Pattern design utilisant %s dans une lib C.",
        "Portabilite de %s entre Linux et Windows.",
        "Performance: impact de %s sur cache et branches.",
        "Refactoring legacy: moderniser %s.",
        "Tests fuzzing cibles sur %s."
    };
    const char *expert[] = {
        "Norme C: ce que la spec dit exactement sur %s.",
        "ABI et %s: implications appel systeme.",
        "Concurrency et %s: pieges memoire.",
        "Static analysis rules pour %s (MISRA).",
        "Reverse engineering: reconnaitre %s dans du binaire.",
        "Interop C/Rust autour de %s.",
        "Formal verification et %s.",
        "Evolution future du standard pour %s."
    };
    const char **templates;
    switch (li) {
        case 1: templates = intermediate; break;
        case 2: templates = advanced; break;
        case 3: templates = expert; break;
        default: templates = junior; break;
    }
    snprintf(text, tlen, templates[v % 8], topics[t]);
    snprintf(ref, rlen,
        "Reponse attendue sur %s en C: syntaxe, semantique, exemple code, "
        "pieges UB et bonnes pratiques (niveau %s, variante %d).",
        topics[t], LEVELS[li], v);
}

static void build_question(const char *domain, const char *level, int idx,
    char *text, size_t tlen, char *ref, size_t rlen, char *topic, size_t topiclen) {
    int di = domain_index(domain);
    int li = level_index(level);
    switch (di) {
        case 1: build_algo_question(li, idx, text, tlen, ref, rlen, topic, topiclen); break;
        case 2: build_c_question(li, idx, text, tlen, ref, rlen, topic, topiclen); break;
        default: build_arch_question(li, idx, text, tlen, ref, rlen, topic, topiclen); break;
    }
}

int qb_seed_domain_level(sqlite3 *db, const char *domain, const char *level, int target_count) {
    if (!db || !domain || !level || target_count <= 0) return 0;
    int inserted = 0;
    char text[512], ref[512], topic[128];
    for (int i = 0; i < target_count; i++) {
        build_question(domain, level, i, text, sizeof(text), ref, sizeof(ref), topic, sizeof(topic));
        inserted += insert_question(db, domain, level, text, ref, topic);
    }
    return inserted;
}

int qb_import_csv(sqlite3 *db, const char *csv_path) {
    if (!db || !csv_path) return 0;
    FILE *f = fopen(csv_path, "r");
    if (!f) return 0;
    char line[4096];
    int imported = 0;
    bool header = false;
    while (fgets(line, sizeof(line), f)) {
        if (!header) {
            if (strncmp(line, "id,", 3) == 0) { header = true; continue; }
            header = true;
        }
        char *nl = strchr(line, '\n'); if (nl) *nl = '\0';
        char *cr = strchr(line, '\r'); if (cr) *cr = '\0';
        trim(line);
        if (!line[0] || line[0] == '#') continue;

        char *c1 = strchr(line, ','); if (!c1) continue; *c1 = '\0';
        char *c2 = strchr(c1 + 1, ','); if (!c2) continue; *c2 = '\0';
        char *c3 = strchr(c2 + 1, ','); if (!c3) continue; *c3 = '\0';
        char *c4 = strchr(c3 + 1, ','); if (!c4) continue; *c4 = '\0';

        trim(c1 + 1); trim(c2 + 1); trim(c3 + 1); trim(c4 + 1);
        if (!c3[1]) continue;
        imported += insert_question(db, c1 + 1, c2 + 1, c3 + 1, c4 + 1, "csv");
    }
    fclose(f);
    return imported;
}