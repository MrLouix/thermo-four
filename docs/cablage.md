# Câblage et brochage — prototype v1

Carte : **Adafruit ESP32-S3 Feather** (ADA5323, 8 Mo de flash, sans PSRAM).
Bus I2C : SDA = GPIO3, SCL = GPIO4 (connecteur STEMMA QT et broches SDA/SCL). L'alimentation du connecteur STEMMA QT est commandée par GPIO7, que le firmware met à l'état haut au démarrage.
Jauge batterie **MAX17048** intégrée à la carte, en I2C à l'adresse 0x36 : aucune entrée ADC n'est nécessaire pour la batterie.

## Affectation des broches

| Signal | Broche Feather | GPIO | Remarque |
|---|---|---|---|
| I2C SDA | SDA | 3 | ADS1115, OLED, MAX17048 (5 kΩ de rappel déjà sur la carte) |
| I2C SCL | SCL | 4 | |
| Alim. STEMMA QT / I2C | — | 7 | Sortie à l'état haut (`I2C_POWER`) |
| Bouton **+** | D5 | 5 | Vers GND, rappel interne |
| Bouton **−** | D6 | 6 | Vers GND, rappel interne |
| Bouton **Mode** | D9 | 9 | Vers GND, rappel interne ; réveille la carte de la veille profonde |
| Buzzer (PWM) | D10 | 10 | Base du transistor via 1 kΩ |
| Excitation des sondes | D11 | 11 | Alimente les ponts uniquement pendant la mesure (économie + pas d'auto-échauffement) |
| ALERT/RDY ADS1115 | D12 | 12 | Réservé, non utilisé en v1 |
| LED intégrée | D13 | 13 | Clignote pendant un enregistrement |

Broches à ne pas utiliser : GPIO0 (bouton BOOT), GPIO45 et GPIO46 (broches lues au démarrage, « strapping »), GPIO33 (NeoPixel) et GPIO21 (alimentation de la NeoPixel).

## ADS1115

| Broche ADS1115 | Connexion |
|---|---|
| VDD | 3V3 de la carte |
| GND | GND |
| SDA / SCL | Bus I2C |
| ADDR | GND → adresse 0x48 |
| A0 | Point milieu du pont de la sonde S1 (viande) |
| A1 | Point milieu du pont de la sonde S2 (four) |
| A2 | Ligne d'excitation (D11), pour une mesure ratiométrique |
| A3 | Libre |

Gain ±4,096 V. Le firmware calcule `rapport = V(Ax) / V(A2)`, ce qui rend la mesure indépendante de la tension exacte de la batterie et du régulateur.

## Pont de mesure (une fois par sonde)

```
D11 (EXC) ──┬──────────────► A2 (mesure de l'excitation)
            │
           [R_ref 0,1 %]
            │
            ├──────────────► A0 (ou A1)
            │       │
            │     [100 nF]
            │       │
            │      GND
            │
         jack 2,5 mm : pointe
           (sonde NTC)
         jack 2,5 mm : manchon ── GND
```

- **R_ref** : à choisir à la réception des sondes, proche de √(R_NTC(50 °C) × R_NTC(250 °C)). Pour une NTC 100 kΩ / B 3950, cela donne environ 3,5 kΩ, soit **3,3 kΩ** en série E24 (valeur par défaut du firmware). Pour une NTC de 10 kΩ, cela donne environ 330 Ω.
- Le **100 nF** filtre le bruit (Wi‑Fi, câble de la sonde qui fait antenne).
- Sonde absente : A0 ≈ excitation → « circuit ouvert ». Sonde en court-circuit : A0 ≈ 0. Les deux cas sont détectés.
- Si les jacks sont stéréo, laisser la bague (« ring ») non connectée.

## Buzzer

```
D10 ──[1 kΩ]──► base du NPN 2N2222 (ou grille d'un AO3400)
3V3 ──► piézo + ; piézo − ──► collecteur ; émetteur ──► GND
        (10 kΩ en parallèle sur le piézo pour le décharger)
```

Volume : 4 niveaux (coupé, faible, moyen, fort) obtenus en faisant varier le rapport cyclique du PWM, autour de la fréquence de résonance du piézo (environ 2,7–4 kHz, voir sa fiche technique). Si les écarts entre niveaux ne s'entendent pas assez, ajouter 100–220 Ω en série avec le piézo.

## Écran OLED SH1106 (I2C)

VCC → 3V3, GND → GND, SDA/SCL → bus I2C. Adresse 0x3C.

## Alimentation

- LiPo 1S 2000 mAh sur le connecteur JST-PH de la Feather. **Vérifier la polarité** : rouge = + du côté marqué « + » sur la carte.
- Interrupteur coupe-batterie en option : à placer entre la broche EN et GND de la Feather (l'interrupteur désactive le régulateur), plutôt que de couper le fil de la batterie.
- Charge par USB-C.

## Implantation dans le boîtier

- Les jacks et le côté de la batterie à l'opposé de la face aimantée qui touche le four.
- L'ADS1115 près des jacks, avec des fils courts entre les ponts et les entrées A0/A1.
- L'antenne de la Feather (bord du module) éloignée des aimants et du métal du four.
