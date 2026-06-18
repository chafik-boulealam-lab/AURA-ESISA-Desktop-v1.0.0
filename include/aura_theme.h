#ifndef AURA_THEME_H
#define AURA_THEME_H

/* Palette Obsidian Luxe — tokens partages entre ecrans GTK */
#define AURA_CSS_BASE \
    "* { font-family: 'Segoe UI', 'Inter', sans-serif; outline: none; }" \
    "scrollbar trough { background-color: rgba(20, 16, 31, 0.6); border-radius: 8px; }" \
    "scrollbar slider { background-color: rgba(240, 180, 41, 0.35); border-radius: 8px; min-width: 8px; min-height: 8px; }" \
    "scrollbar slider:hover { background-color: rgba(139, 124, 246, 0.55); }" \
    "progressbar trough { background-color: rgba(30, 24, 40, 0.9); border-radius: 999px; min-height: 8px; }" \
    "progressbar progress { background-image: linear-gradient(90deg, #F0B429 0%, #8B7CF6 100%); border-radius: 999px; }" \
    "button { background-image: none; box-shadow: none; }" \
    "entry { caret-color: #F0B429; }" \
    "textview { caret-color: #F0B429; }"

#endif