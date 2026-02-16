-- Smart Oil Press Management - Oracle Schema
-- Generated on 2026-02-14

-- 1) EMPLOYE
CREATE TABLE EMPLOYE (
    id_emp NUMBER PRIMARY KEY,
    nom_emp VARCHAR2(50) NOT NULL,
    prenom_emp VARCHAR2(50) NOT NULL,
    email VARCHAR2(100) UNIQUE NOT NULL,
    role VARCHAR2(50)
        CHECK (role IN ('ADMIN','TECHNICIEN','OPERATEUR')),
    mdp VARCHAR2(100) NOT NULL,
    photo BLOB,
    empreinte BLOB,
    modele_faciale BLOB
);

CREATE SEQUENCE seq_employe START WITH 1 INCREMENT BY 1 NOCACHE;

CREATE OR REPLACE TRIGGER trg_employe
BEFORE INSERT ON EMPLOYE
FOR EACH ROW
BEGIN
    SELECT seq_employe.NEXTVAL INTO :NEW.id_emp FROM dual;
END;
/

-- 2) MACHINE
CREATE TABLE MACHINE (
    id_machine NUMBER PRIMARY KEY,
    nom_machine VARCHAR2(100) NOT NULL,
    type_machine VARCHAR2(50),
    etat_machine VARCHAR2(20)
        CHECK (etat_machine IN ('ACTIVE','PANNE','MAINTENANCE')),
    date_installation DATE,
    heures_fonctionnement NUMBER
        CHECK (heures_fonctionnement >= 0),
    temperature_actuelle NUMBER
        CHECK (temperature_actuelle BETWEEN -20 AND 200)
);

CREATE SEQUENCE seq_machine START WITH 1 INCREMENT BY 1 NOCACHE;

CREATE OR REPLACE TRIGGER trg_machine
BEFORE INSERT ON MACHINE
FOR EACH ROW
BEGIN
    SELECT seq_machine.NEXTVAL INTO :NEW.id_machine FROM dual;
END;
/

-- 3) OPERE (relation N:N)
CREATE TABLE OPERE (
    id_machine NUMBER,
    id_emp NUMBER,
    PRIMARY KEY (id_machine, id_emp),
    CONSTRAINT fk_op_machine
        FOREIGN KEY (id_machine)
        REFERENCES MACHINE(id_machine)
        ON DELETE CASCADE,
    CONSTRAINT fk_op_emp
        FOREIGN KEY (id_emp)
        REFERENCES EMPLOYE(id_emp)
        ON DELETE CASCADE
);

-- 4) AGRICULTEUR
CREATE TABLE AGRICULTEUR (
    id_agri NUMBER PRIMARY KEY,
    nom_agri VARCHAR2(50) NOT NULL,
    prenom_agri VARCHAR2(50) NOT NULL,
    adresse_agri VARCHAR2(200),
    num_agri VARCHAR2(20),
    mail_agri VARCHAR2(100) UNIQUE,
    region_agri VARCHAR2(100),
    nb_arbres NUMBER
        CHECK (nb_arbres >= 0),
    type_olives VARCHAR2(100),
    qtolives_anneeprec NUMBER
        CHECK (qtolives_anneeprec >= 0),
    rende_moy NUMBER
        CHECK (rende_moy BETWEEN 0 AND 100),
    date_premvisi DATE,
    note_qualtmoy NUMBER
        CHECK (note_qualtmoy BETWEEN 0 AND 10)
);

CREATE SEQUENCE seq_agriculteur START WITH 1 INCREMENT BY 1 NOCACHE;

CREATE OR REPLACE TRIGGER trg_agriculteur
BEFORE INSERT ON AGRICULTEUR
FOR EACH ROW
BEGIN
    SELECT seq_agriculteur.NEXTVAL INTO :NEW.id_agri FROM dual;
END;
/

-- 5) CITERNE
CREATE TABLE CITERNE (
    id_citerne NUMBER PRIMARY KEY,
    capaciteMax NUMBER
        CHECK (capaciteMax > 0),
    niveauActuel NUMBER
        CHECK (niveauActuel >= 0),
    typeHuile VARCHAR2(50)
        CHECK (typeHuile IN ('EXTRA_VIERGE','VIERGE','LAMPANTE')),
    temperature_citerne NUMBER
        CHECK (temperature_citerne BETWEEN -10 AND 100),
    etat_citerne VARCHAR2(20)
        CHECK (etat_citerne IN ('PLEINE','VIDE','EN_UTILISATION')),
    id_agri NUMBER,
    CONSTRAINT chk_niveau_capacite
        CHECK (niveauActuel <= capaciteMax),
    CONSTRAINT fk_citerne_agri
        FOREIGN KEY (id_agri)
        REFERENCES AGRICULTEUR(id_agri)
        ON DELETE CASCADE
);

CREATE SEQUENCE seq_citerne START WITH 1 INCREMENT BY 1 NOCACHE;

CREATE OR REPLACE TRIGGER trg_citerne
BEFORE INSERT ON CITERNE
FOR EACH ROW
BEGIN
    SELECT seq_citerne.NEXTVAL INTO :NEW.id_citerne FROM dual;
END;
/

-- 6) STOCK
CREATE TABLE STOCK (
    id_stock NUMBER PRIMARY KEY,
    nom_stock VARCHAR2(100) NOT NULL,
    categ_stock VARCHAR2(50),
    dateajt_stock DATE,
    descript_stock VARCHAR2(200),
    datemaj_stock DATE,
    qt_stock NUMBER
        CHECK (qt_stock >= 0),
    id_agri NUMBER,
    id_citerne NUMBER,
    CONSTRAINT fk_stock_agri
        FOREIGN KEY (id_agri)
        REFERENCES AGRICULTEUR(id_agri)
        ON DELETE CASCADE,
    CONSTRAINT fk_stock_citerne
        FOREIGN KEY (id_citerne)
        REFERENCES CITERNE(id_citerne)
        ON DELETE CASCADE
);

CREATE SEQUENCE seq_stock START WITH 1 INCREMENT BY 1 NOCACHE;

CREATE OR REPLACE TRIGGER trg_stock
BEFORE INSERT ON STOCK
FOR EACH ROW
BEGIN
    SELECT seq_stock.NEXTVAL INTO :NEW.id_stock FROM dual;
END;
/

-- 7) QUALITE
CREATE TABLE QUALITE (
    id_qual NUMBER PRIMARY KEY,
    date_prod DATE NOT NULL,
    date_exp DATE NOT NULL,
    qt_prod NUMBER
        CHECK (qt_prod > 0),
    temp_prod NUMBER
        CHECK (temp_prod BETWEEN 0 AND 100),
    ph_prod NUMBER
        CHECK (ph_prod BETWEEN 0 AND 14),
    acid_prod NUMBER
        CHECK (acid_prod >= 0),
    note_gout NUMBER
        CHECK (note_gout BETWEEN 0 AND 10),
    code_couleur VARCHAR2(20),
    type_emb VARCHAR2(50)
        CHECK (type_emb IN ('BOUTEILLE','BIDON','VRAC')),
    date_emb DATE,
    vol_emb NUMBER
        CHECK (vol_emb > 0),
    statut_qual VARCHAR2(20)
        CHECK (statut_qual IN ('VALIDE','REFUSE','EN_ANALYSE')),
    resp_cntrl VARCHAR2(100),
    observ VARCHAR2(200),
    id_citerne NUMBER,
    CONSTRAINT chk_date_validite
        CHECK (date_exp > date_prod),
    CONSTRAINT fk_qualite_citerne
        FOREIGN KEY (id_citerne)
        REFERENCES CITERNE(id_citerne)
        ON DELETE CASCADE
);

CREATE SEQUENCE seq_qualite START WITH 1 INCREMENT BY 1 NOCACHE;

CREATE OR REPLACE TRIGGER trg_qualite
BEFORE INSERT ON QUALITE
FOR EACH ROW
BEGIN
    SELECT seq_qualite.NEXTVAL INTO :NEW.id_qual FROM dual;
END;
/
