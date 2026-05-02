INTÉGRATION ARDUINO -> TABLE MACHINE
===================================

Cette version lit la température depuis l'Arduino via QSerialPort et met à jour directement la table MACHINE.
Aucune table supplémentaire n'est nécessaire.

1) Code Arduino conseillé
-------------------------
Le projet accepte :
- TEMP:27.4;STATE:NORMAL
ou simplement la ligne de température de votre sketch :
- Température: 27.4 °C

2) Dans Qt
----------
- Ouvrir Module Machine
- Aller dans "Avancé"
- Dans "Connexion capteur Arduino / DHT22" :
  - choisir la machine cible
  - choisir le port COM
  - cliquer sur "Connecter le capteur"

3) Mise à jour Oracle
---------------------
La température reçue met à jour directement :
- MACHINE.temperature_actuelle
- MACHINE.etat_machine

Règles utilisées :
- température < 30  -> ACTIVE
- 30 <= température < 40 -> ACTIVE (alerte visuelle)
- température >= 40 -> MAINTENANCE

4) Si rien n'apparaît
---------------------
- vérifier le port COM dans l'IDE Arduino / Windows
- fermer le Serial Monitor avant d'ouvrir Qt
- vérifier que la connexion Oracle est déjà ouverte dans l'application
