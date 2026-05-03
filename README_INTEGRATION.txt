INTEGRATION PROJET - VERSION FUSIONNEE
====================================

Contenu:
- projet principal Smart-Oil-Press-Management-2A10
- integration du module Gestion des Machines
- metiers avances visibles dans l'onglet Machines > Avance
- ajout dynamique d'un formulaire Serie machine dans la page Ajouter si le .ui d'origine ne l'avait pas
- correction des ressources images manquantes
- CMake ajuste pour Qt Widgets + Charts + Sql

A verifier apres ouverture dans Qt Creator:
1. Ouvrir CMakeLists.txt
2. Supprimer/Nettoyer le dossier build
3. Construire > Nettoyer le projet
4. Construire > Reconstruire le projet
5. Executer

Fonctionnalites Machines integrees:
- CRUD machine
- ajout de serie machine
- recherche / tri / export CSV
- analyse de performance
- maintenance predictive simplifiee

Remarque base de donnees:
- le projet suppose l'existence des tables MACHINE et SERIE_MACHINE
- si votre base locale est plus ancienne, executer sql_patch_machine_if_needed.sql

Mise a jour supplementaire (fusion finale)
------------------------------------------
- integration du module Citernes depuis smartoil
- ouverture du module Citernes depuis le bouton sidebar et les boutons de la page Citernes
- ajout des fichiers citernes.cpp / citernes.h / citernes.ui au projet CMake
- ajout de Qt PrintSupport pour l'export PDF du module Citernes
- correction du flux Ajout Qualite: ID_LOT est maintenant laisse a la base (sequence + trigger)
- si QUALITE n'a pas encore d'auto-ID dans votre base, executer sql_patch_qualite_auto_id.sql


Note correctif v2:
- Les fichiers login.cpp/login.h/login.ui existent encore dans le dossier source mais ne sont plus compilés, car ils définissaient un second MainWindow et provoquaient un conflit MOC sous Qt 6. Le projet démarre via mainwindow.cpp/mainwindow.ui.


Mise a jour integration finale:
- ajout du module Stocks (stocks.cpp/.h/.ui)
- acces depuis le bouton sidebar Stock et les boutons du module 2
- fenetre Stocks harmonisee avec l'interface existante (toolbar, icones qrc, style qss)
- fallback base de donnees pour le module Stocks afin de fonctionner avec la connexion principale deja ouverte
