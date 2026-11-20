# CLAUDE.md

Firmware PlatformIO (Arduino, ESP32-S3) d'un thermomètre de four connecté. Code et documentation en français.

## Commandes

- Tests : `pio test -e native` (logique pure dans `lib/thermo_core/`, sans Arduino)
- Compilation : `pio run -e feather_s3` ; interface web : `pio run -e feather_s3 -t buildfs`
- La CI (`.github/workflows/ci.yml`) lance les deux ; les releases sont publiées sur les tags `v*`.

## Conventions

- Brochage centralisé dans `include/config.h`, documenté dans `docs/cablage.md` : modifier les deux ensemble.
- Toute logique testable sans matériel va dans `lib/thermo_core/` avec un test dans `test/test_core/`.
- C++17 (`-std=gnu++17`, cœur Arduino-ESP32 2.0.x).
- L'interface web (`data/www/index.html`) est autonome : pas de CDN (doit marcher en mode point d'accès sans internet).

## GitHub Actions : stockage des artefacts

Le stockage Actions du compte est limité (0,5 Go par mois, partagé entre tous les dépôts privés).

Pour tout workflow créé ou modifié :

- Chaque étape `actions/upload-artifact` fixe `retention-days` : 3 jours au plus pour un binaire, 7 jours au plus pour des rapports ou des journaux.
- Ne pas envoyer d'artefact si le fichier est déjà publié ailleurs (release).
- Ne pas envoyer de binaire lourd à chaque push : le limiter à `main`, aux tags ou à `workflow_dispatch`.
- Éviter de déclencher deux fois le même travail (`push` et `pull_request` sur la même branche).
