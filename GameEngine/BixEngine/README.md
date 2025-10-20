# BixEngine

BixEngine est une refonte du projet d'origine avec une structure de dossiers claire et un outil
permettant de récupérer automatiquement les bibliothèques SDL nécessaires pour la compilation
Windows.

## Structure du projet

```
BixEngine/
├── Build/                 # Artefacts de build générés par MSBuild/XMake
├── Binaries/              # Binaires runtime (exécutables, DLL, logs)
├── Content/               # Contenu du moteur (scènes, assets, etc.)
├── Runtime/               # Code source du moteur (Core, Graphics, Input, ...)
├── Samples/               # Points d'entrée et exemples de jeu
├── ThirdParty/            # Dépendances externes (SDL3, ImGui)
└── Tools/                 # Scripts utilitaires (ex: téléchargement SDL3)
```

## Pré-requis

* Visual Studio 2022 + MSBuild v143
* Python 3 (pour exécuter le script de récupération des librairies SDL3)

## Préparation de l'environnement SDL3

Les en-têtes de SDL3 sont versionnés dans `ThirdParty/SDL3-3.2.22`, mais les bibliothèques
binaires Windows ne le sont pas. Avant une compilation Windows, exécutez :

```powershell
cd BixEngine
python Tools\fetch_sdl3.py --arch all
```

Le script installe simultanément les binaires x86 et x64, tout en conservant des dossiers de
compatibilité (`lib\Win32`, `lib\Win64`, …) pour les anciens projets. `--arch` vaut `all` par
défaut et aucune donnée n'est retéléchargée si les bibliothèques requises sont déjà présentes.

> **Astuce :** Le projet Visual Studio déclenche automatiquement ce script en pré-build.
Si Python n'est pas installé ou pas accessible via la commande `python`, installez-le
et ajoutez-le à votre `PATH`.

## Compilation

### Avec Visual Studio

Ouvrez `BixEngine/BixEngine.vcxproj` ou la solution `vsxmake2022/BixEngine.sln`. Un évènement
pré-build s'assure que les binaires SDL3 sont présents avant l'édition de liens.

### Avec xmake

```bash
cd BixEngine
xmake
xmake run BixEngine
```

Utilisez `xmake f --sdl3_dir=/path/vers/SDL3` pour pointer vers une installation SDL3
customisée si nécessaire.
