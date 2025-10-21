# BixEngine

BixEngine est une refonte du projet d'origine avec une structure de dossiers claire et des
outils prêts à accueillir les bibliothèques SDL nécessaires pour la compilation Windows.

## Structure du projet

```
BixEngine/
├── Build/                 # Artefacts de build générés par MSBuild/XMake
├── Binaries/              # Binaires runtime (exécutables, DLL, logs)
├── Content/               # Contenu du moteur (scènes, assets, etc.)
├── Runtime/
│   ├── Include/Bix/       # En-têtes publics modulaires (Core, Engine, Game, ...)
│   └── Source/            # Implémentations et code privé du runtime
├── Samples/               # Points d'entrée et exemples de jeu
├── ThirdParty/            # Dépendances externes (SDL3, ImGui)
└── Tools/                 # Scripts utilitaires (ex: téléchargement SDL3)
```

## Pré-requis

* Visual Studio 2022 + MSBuild v143
* (Optionnel) Python 3 si vous souhaitez automatiser le téléchargement des librairies SDL3 via
  vos propres scripts

## Préparation de l'environnement SDL3

Les en-têtes de SDL3 sont versionnés dans `ThirdParty/SDL3-3.2.22`, mais les bibliothèques
binaires Windows ne le sont pas. Téléchargez manuellement les archives officielles SDL3 et
décompressez les binaires dans `ThirdParty/SDL3-3.2.22/lib`. Conservez la structure `lib/x64`
et `lib/x86` attendue par le projet Visual Studio.

## Compilation

### Avec Visual Studio

Ouvrez `BixEngine/BixEngine.vcxproj` ou la solution `vsxmake2022/BixEngine.sln`. Assurez-vous
que les bibliothèques SDL3 ont été copiées au préalable comme décrit ci-dessus avant de lancer
la compilation.

### Avec xmake

```bash
cd BixEngine
xmake
xmake run BixEngine
```

Utilisez `xmake f --sdl3_dir=/path/vers/SDL3` pour pointer vers une installation SDL3
customisée si nécessaire.
