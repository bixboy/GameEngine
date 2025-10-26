# BixEngine

BixEngine est un moteur de jeu C++ structuré autour de trois grands ensembles :

```
engine/       → Code du moteur et bibliothèques partagées
apps/         → Applications finales (ex. bix_run)
tools/        → Outils autonomes (ex. bix_header_tool)
third_party/  → Dépendances externes fournies avec le projet
```

## Construction

Le projet utilise [xmake](https://xmake.io).

```bash
xmake f -c          # Nettoie la configuration précédente
xmake               # Compile l'ensemble des cibles
xmake project -k compile_commands
```

Le binaire `bix_run` dépend automatiquement de la bibliothèque `bix_engine`. Les entêtes de réflexion sont générés dans `engine/include/Bix/Generated/` via l'outil `bix_header_tool`.
