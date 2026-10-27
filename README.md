# Doom II RPG - Nintendo 64 Port

Port de **Doom II RPG** pour Nintendo 64 basé sur la rétro-ingénierie [DoomIIRPG-RE](https://github.com/Erick194/DoomIIRPG-RE) de Erick Vásquez García ([GEC]), propulsé par [Libdragon](https://github.com/DragonMinded/libdragon).

---

## 🎮 À propos du portage N64

Ce projet vise à porter fidèlement le jeu Doom II RPG (sorti à l'origine sur BREW/J2ME puis iPhone OS) sur la console Nintendo 64.

### Spécifications & Objectifs techniques
- **Système de base** : Nintendo 64 avec **Expansion Pak (8 Mo)** requis.
- **SDK** : [Libdragon](https://github.com/DragonMinded/libdragon) (toolchain GCC `mips64-elf`).
- **Résolution d'affichage** : 320×240 standard N64 (mode Letterbox 320×213 respectant le ratio 3:2 d'origine de l'iPhone sans déformation).
- **Moteur de rendu** : Intégration du rasterizer software fixed-point d'origine (`TinyGL`) directement dans le framebuffer 16-bit N64 (RGBA5551), ou accélération via l'API RDPQ/OpenGL de Libdragon.
- **Système de fichiers (DFS)** : Assets stockés directement dans la cartouche ROM (jusqu'à 64 Mo) via **DragonFS**, éliminant le besoin de décompresser un gros ZIP de 60 Mo en RAM.
- **Audio** : Effets sonores et musiques convertis et joués via le sous-système audio/mixer matériel de Libdragon.
- **Contrôles** : Prise en charge native du contrôleur Nintendo 64.

---

## 📦 Étape 1 : Extraction des Assets

Les assets originaux (textures, sprites, musiques, sons, cartes) sont contenus dans l'application iOS originale (`Doom 2 RPG.ipa`).

### Prérequis
- Python 3
- Le fichier `Doom 2 RPG.ipa` placé à la racine du projet

### Extraire les assets
Exécutez simplement le script d'extraction :

```bash
python3 tools/extract_assets.py
```

Le script extrait automatiquement les 544 fichiers requis du package vers `filesystem/Packages/` :
- `Audio (WAV)` : 148 fichiers (~37.5 Mo, dont ~31 Mo de musiques non compressées)
- `Données (.bin)` : 68 fichiers (cartes, modèles 3D, définitions de monstres, palettes)
- `Graphismes (.bmp)` : 219 fichiers (interfaces, portraits, HUD)
- `ComicBook` : 56 planches pour les scènes narratives
- `PNG` : 48 fichiers complémentaires

---

## 🗺️ Feuille de route du portage

- [x] **Étape 1 : Extraction & Analyse des Assets** (`extract_assets.py`, `filesystem/`)
- [x] **Étape 2 : Configuration du build Libdragon** (`Makefile`, couches de compatibilité N64)
- [x] **Étape 3 : Système de fichiers & Flux de données** (chargement direct depuis DragonFS `rom:/Packages/...`)
- [x] **Étape 4 : Rendu vidéo & Framebuffer N64** (affichage 320×240, letterbox 320×213 au ratio 3:2 natif, hook d'affichage `display_get()` / `rdpq` / `gl_context`)
- [x] **Étape 5 : Gestion des contrôles N64** (D-Pad, Stick analogique, A, B, Z, R, C-Buttons, Rumble Pak)
- [x] **Étape 6 : Audio Libdragon** (mixer matériel RSP 16 canaux, audio 44.1 kHz, compression VADPCM WAV64)
- [x] **Gestion des canaux Stéréo Libdragon Mixer (32 canaux max)** :
  - Chaque flux audio logique (`SoundStream channel[10]`) est désormais mappé sur une paire dédiée de canaux physiques Libdragon (`canal = index * 2`, 20 canaux initialisés via `mixer_init(20)`).
  - Évite les collisions et l'assertion `mixer_ch_set_vol: cannot call on secondary stereo channel` lorsqu'un son mono (navigation de menu, tir) est joué pendant la lecture d'une piste stéréo (musique de titre, etc.).
- [x] **Corrections Bas-Niveau MIPS/GCC 14 & OpenGL Libdragon** :
  - Endianness Big-Endian dans les flux binaires et couleurs de brouillard (`JavaStream.cpp`, `GLES.cpp`)
  - Flags GCC 14 (`-fno-lifetime-dse`, `-fno-delete-null-pointer-checks`) indispensables pour le code C++ décompilé appelant manuellement des destructeurs (`IS.~InputStream()`, `nullptr->~Image()`) sans double-free
  - Chargement direct DragonFS (`dfs_open`/`dfs_read`/`dfs_close`) et libération systématique des tampons dans `InputStream::close()` (`ZipFile.cpp`, `JavaStream.cpp`, `Resource.cpp`)
  - Conformité stricte OpenGL Libdragon : conversion universelle RGB565 → RGBA5551 (`GL_UNSIGNED_SHORT_5_5_5_1`), paramètres `GL_TEXTURE_WRAP` avant `glTexImage2D`, cache LRU textures 3D borné à 1 Mo RDRAM (`Image.cpp`, `GLES.cpp`, `Render.cpp`, `ComicBook.cpp`)
  - Gestion du cycle de vie du contexte OpenGL (`rdpq_is_attached()`) : protection de `glViewport`, `glScissor`, `glClear` lors de l'initialisation du moteur hors boucle de rendu active (`GLES.cpp`)
  - Correction du pipeline de sommets 2D : désactivation de `GL_TEXTURE_COORD_ARRAY` lors du tracé de rectangles unis dans [`Graphics::FMGL_fillRect()`](file:///Users/gcoursaget/Dropbox/64dev/doom-ii-rpg-64/src/Graphics.cpp#L226-L235) pour éviter un déréférencement NULL sur le processeur RSP (`rsp_pipeline.c`)
  - Sécurisation de l'allocation des textures dynamiques : calcul précis de la taille de texture puissance de deux et garde stricte sur `glBindTexture` empêchant toute liaison d'identifiant non alloué (`0xffffffff`) (`Image.cpp`, `Graphics.cpp`, `ComicBook.cpp`, `GLES.cpp`)
  - Conformité matérielle TMEM N64 (4 Ko) & Budget RDRAM 8 Mo :
    - Découpage automatique des textures 2D (`Image`, polices, logos, menus, ComicBook) en tuiles matérielles `<= 64×32` RGBA5551 (`4 096` octets max par tuile, largeur minimale `4` px pour alignement 8 octets RDP) avec cache LRU global borné à `1,25 Mo` (`src/Image.cpp`, `src/Graphics.cpp`, `src/ComicBook.cpp`)
    - Rééchantillonnage automatique des textures 3D > 2048 texels vers la limite matérielle TMEM 4 Ko et cache LRU 3D borné à `512 Ko` (`src/GLES.cpp`)
    - Division par 2 de l'empreinte RAM des BMP indexés (`1 octet/pixel` au lieu de 2, libération immédiate de `pRGB888`) et chargement à la demande des mini-jeux uniquement dans l'état `ST_MINI_GAME` (`src/App.cpp`, `src/Canvas.cpp`)
  - **Correction du blocage de fin/skip de cinématique & Système de sauvegarde N64** :
    - Gestion sécurisée des flux d'écriture `OutputStream::openFile` sur DragonFS (filesystem cartouche en lecture seule) avec mode puits sans échec (`noWrite = true`) évitant les erreurs d'E/S fatales et le crash `exit(0)` lors de l'enregistrement de configuration (`saveConfig`)
    - Élimination des déréférencements de pointeurs nuls (`dialogBuffer->dispose()`, `imgMapCursor->~Image()`, `imgMainBG->~Image()`) lors du passage de `ST_INTRO_MOVIE` au menu principal (`Canvas::backToMain()`, `Applet::freeRuntimeImages()`)
    - Suppression de l'appel d'arrêt prématuré `app->shutdown()` sur la touche d'annulation dans `ST_INTRO_MOVIE` et routage unifié vers `Canvas::exitIntroMovie(false)`
- [ ] **Étape 7 : Optimisations mémoire & performances (en cours de validation)**

---

## 🔊 Sous-système Audio Nintendo 64

- **Mixer RSP** : 16 canaux matériels décodés en temps réel par le coprocesseur RSP (`audio_init(44100, 4)` + `mixer_init(16)`).
- **Compression VADPCM (WAV64)** : Les 148 fichiers audio originaux ont été convertis au format `wav64` avec compression matérielle VADPCM via `audioconv64`.
- **Réduction d'empreinte** : Le dossier audio est passé de **37.5 Mo à 10.0 Mo** (près de **27.5 Mo d'économisés**), permettant à la ROM totale de passer de **60 Mo à 32 Mo** (format cartouche 256 Mbits).
- **Streaming direct depuis la cartouche** : Chaque canal alloue dynamiquement son waveform sans saturer la RDRAM.

---

## 🕹️ Contrôles Nintendo 64

| Touche N64 | Action en jeu | Action dans les menus |
| :--- | :--- | :--- |
| **D-Pad Haut / Stick Haut** | Avancer | Naviguer vers le haut |
| **D-Pad Bas / Stick Bas** | Reculer | Naviguer vers le bas |
| **D-Pad Gauche / Stick Gauche** | Tourner à gauche | Page précédente / Décrémenter |
| **D-Pad Droite / Stick Droite** | Tourner à droite | Page suivante / Incrémenter |
| **Gâchette L** | Pas latéral gauche (Strafe gauche) | - |
| **Gâchette R** | Pas latéral droite (Strafe droite) | - |
| **Bouton A** | Attaquer / Parler / Utiliser | Valider / Sélectionner |
| **Bouton B** | Passer le tour | Retour / Annuler |
| **Bouton Start** | Pause / Ouvrir le menu principal | Fermer le menu |
| **Gâchette Z** | Afficher / Masquer l'Automap (Carte) | - |
| **Bouton C-Haut** | Arme suivante | - |
| **Bouton C-Bas** | Arme précédente | - |
| **Bouton C-Gauche** | Menu Objets / Inventaire | - |
| **Bouton C-Droite** | Menu PDA / Journal des missions | - |
| **Rumble Pak** | Retour de vibrations matériel | - |

---

## 🛠️ Compilation pour Nintendo 64

### Prérequis
- Toolchain Libdragon configurée (`N64_INST=/Users/gcoursaget/libdragon-toolchain`)
- Python 3 et les assets extraits dans `filesystem/` (voir Étape 1)

### Compiler la ROM :
```bash
export N64_INST=/Users/gcoursaget/libdragon-toolchain
export PATH=$N64_INST/bin:$PATH
make doom2rpg.z64
```
Cela produit la ROM prête pour la console / émulateur : **`doom2rpg.z64`**.

---

## Informations sur le projet original PC (DoomIIRPG-RE)

Créé par Erick Vásquez García ([GEC]).
Projet d'origine : https://github.com/Erick194/DoomIIRPG-RE

### Configuration des touches PC d'origine :
- Déplacements : Z/W, S, Q/A, D
- Tourner : Flèches Gauche/Droite
- Attaquer / Parler / Utiliser : Entrée
- Arme suivante / précédente : Z / X
- Passer le tour : C
- Automap : Tab
- Menu : Echap
- PDA : P
- Items : I
- Boissons : O
- Bot : B

### Cheat codes d'origine (dans le menu) :
- `3666` : Menu debug
- `1666` : Redémarre le niveau
- `4332` : Donne toutes les clés, objets et armes
- `3366` : Benchmark / test de vitesse
