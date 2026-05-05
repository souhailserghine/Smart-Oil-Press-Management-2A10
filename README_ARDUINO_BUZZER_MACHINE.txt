Integration Arduino / DHT22 / buzzer pour le module Machine

Cette version garde les anciens metiers avances du module Machine inchanges.

Sketch Arduino fourni : arduino_machine_dht22_buzzer.ino

Comportement du buzzer :
- temperature < 30 C : bip normal
- temperature >= 30 C : bip accelere
- temperature >= 40 C : son continu

Format envoye sur le port serie :
- TEMP:27.4;STATE:NORMAL
- TEMP:33.1;STATE:ALERTE
- TEMP:41.2;STATE:DANGER

Si vous souhaitez reconnecter ce sketch plus tard a Qt, utilisez QSerialPort et faites un UPDATE sur la table MACHINE :
UPDATE MACHINE SET temperature_actuelle = :temp, etat_machine = :etat WHERE id_machine = :id;

Regle conseillee :
- NORMAL -> ACTIVE
- ALERTE -> ACTIVE
- DANGER -> MAINTENANCE
