# thermo-four

Boîtier de thermomètre de four connecté, à base d'ESP32-S3. Il réutilise des sondes NTC de cuisson (jack 2,5 mm) et remplace le boîtier d'un thermomètre-minuteur classique (Beka T&T Exacto).

**État : prototype.** Le firmware compile, la logique de mesure et d'alarme est testée. Il n'a pas encore été essayé sur le matériel réel.

## Fonctions

- 2 sondes (viande + air du four), écran OLED, 3 boutons, buzzer à volume réglable
- Consigne haute, seuil bas optionnel, pré-alerte, minuteur
- Détection de sonde débranchée
- Interface web sur `http://thermo.local` (ou l'IP affichée à l'écran par un appui long sur Mode) : températures en direct, réglages, enregistrements programmés avec graphique et export CSV, calibration des sondes sur 3 points
- Notifications push via [ntfy](https://ntfy.sh) et webhook optionnel (n8n…)
- Batterie LiPo avec jauge, alerte à 15 %, extinction par appui très long sur Mode

## Matériel

Voir [`docs/cablage.md`](docs/cablage.md) pour le brochage et le schéma de câblage. Composants principaux : Adafruit ESP32-S3 Feather (ADA5323), ADS1115, OLED 1,3" SH1106, sondes NTC à jack 2,5 mm, piézo passif, LiPo 2000 mAh.

## Compilation et flashage

Prérequis : [PlatformIO](https://platformio.org) (`pip install platformio`).

```bash
pio run -e feather_s3                 # compiler
pio run -e feather_s3 -t upload       # flasher le firmware
pio run -e feather_s3 -t uploadfs     # flasher l'interface web (dossier data/)
pio device monitor                    # console série
```

Les binaires de chaque version (`vX.Y.Z`) sont publiés dans les *Releases* GitHub.

## Première mise en route

1. Au premier démarrage (ou en maintenant **Mode** pendant la mise sous tension), le boîtier crée un point d'accès `Thermo-Four-XXXX`, mot de passe `thermofour`. Le nom et le mot de passe sont affichés à l'écran.
2. Se connecter à ce réseau, ouvrir `http://192.168.4.1`, onglet **Réglages** : saisir jusqu'à 3 réseaux Wi‑Fi (box, partage de connexion du téléphone) et changer le mot de passe du portail.
3. Après redémarrage, ouvrir `http://thermo.local` ou l'IP affichée.
4. Notifications : installer l'appli ntfy et s'abonner au sujet indiqué dans **Réglages** (un sujet aléatoire est créé au premier démarrage).
5. **Calibrer chaque sonde** (onglet Calibration) : eau glacée (0 °C), température ambiante, eau bouillante (100 °C).

## Boutons

| Bouton | Appui court | Appui long |
|---|---|---|
| **+** / **−** | consigne ±1 °C | défilement par pas de 5 °C |
| **Mode** | changer de sonde affichée / couper l'alarme | 1,5 s : afficher l'IP · 4 s : éteindre |

## API

`GET /api/state`, `GET|POST /api/settings`, `POST /api/timer`, `POST /api/ack`, `POST /api/buzzer/test`, `POST /api/record`, `GET /api/logs`, `POST /api/logs/delete`, `GET|POST /api/calibrate`, flux SSE `GET /events`. Les fichiers CSV sont servis sous `/logs/<nom>.csv`.

## Développement

```bash
pio test -e native    # tests unitaires : calculs NTC/Steinhart-Hart et alarmes
```

La logique sans dépendance matérielle se trouve dans `lib/thermo_core/` ; le reste est dans `src/`, un module par fonction.
