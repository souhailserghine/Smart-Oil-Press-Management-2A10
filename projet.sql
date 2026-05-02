-- =============================================================
-- Projet2A1.sql
-- Script safe pour SQL Developer Oracle 11.x
-- IMPORTANT : Executer en mode Script (F5), PAS Run Statement (Ctrl+Enter)
-- =============================================================

-- ETAPE 1 : Supprime les tables existantes (ordre inverse des FK)
--           Ignore ORA-00942 si la table n'existe pas
-- -------------------------------------------------------------
BEGIN
  BEGIN EXECUTE IMMEDIATE 'DROP TABLE EMP_MACH CASCADE CONSTRAINTS';
  EXCEPTION WHEN OTHERS THEN IF SQLCODE != -942 THEN RAISE; END IF; END;
  BEGIN EXECUTE IMMEDIATE 'DROP TABLE CITERNE CASCADE CONSTRAINTS';
  EXCEPTION WHEN OTHERS THEN IF SQLCODE != -942 THEN RAISE; END IF; END;
  BEGIN EXECUTE IMMEDIATE 'DROP TABLE QUALITE CASCADE CONSTRAINTS';
  EXCEPTION WHEN OTHERS THEN IF SQLCODE != -942 THEN RAISE; END IF; END;
  BEGIN EXECUTE IMMEDIATE 'DROP TABLE STOCK CASCADE CONSTRAINTS';
  EXCEPTION WHEN OTHERS THEN IF SQLCODE != -942 THEN RAISE; END IF; END;
  BEGIN EXECUTE IMMEDIATE 'DROP TABLE MACHINE CASCADE CONSTRAINTS';
  EXCEPTION WHEN OTHERS THEN IF SQLCODE != -942 THEN RAISE; END IF; END;
  BEGIN EXECUTE IMMEDIATE 'DROP TABLE SERIE_MACHINE CASCADE CONSTRAINTS';
  EXCEPTION WHEN OTHERS THEN IF SQLCODE != -942 THEN RAISE; END IF; END;
  BEGIN EXECUTE IMMEDIATE 'DROP TABLE AGRICULTEUR CASCADE CONSTRAINTS';
  EXCEPTION WHEN OTHERS THEN IF SQLCODE != -942 THEN RAISE; END IF; END;
  BEGIN EXECUTE IMMEDIATE 'DROP TABLE EMPLOYE CASCADE CONSTRAINTS';
  EXCEPTION WHEN OTHERS THEN IF SQLCODE != -942 THEN RAISE; END IF; END;
END;
/

-- -------------------------------------------------------------
-- ETAPE 2 : Supprime les sequences existantes
--           Ignore ORA-02289 si la sequence n'existe pas
-- -------------------------------------------------------------
BEGIN
  BEGIN EXECUTE IMMEDIATE 'DROP SEQUENCE seq_emp';
  EXCEPTION WHEN OTHERS THEN IF SQLCODE != -2289 THEN RAISE; END IF; END;
  BEGIN EXECUTE IMMEDIATE 'DROP SEQUENCE seq_serie';
  EXCEPTION WHEN OTHERS THEN IF SQLCODE != -2289 THEN RAISE; END IF; END;
  BEGIN EXECUTE IMMEDIATE 'DROP SEQUENCE seq_machine';
  EXCEPTION WHEN OTHERS THEN IF SQLCODE != -2289 THEN RAISE; END IF; END;
  BEGIN EXECUTE IMMEDIATE 'DROP SEQUENCE seq_agri';
  EXCEPTION WHEN OTHERS THEN IF SQLCODE != -2289 THEN RAISE; END IF; END;
  BEGIN EXECUTE IMMEDIATE 'DROP SEQUENCE seq_stock';
  EXCEPTION WHEN OTHERS THEN IF SQLCODE != -2289 THEN RAISE; END IF; END;
  BEGIN EXECUTE IMMEDIATE 'DROP SEQUENCE seq_lot';
  EXCEPTION WHEN OTHERS THEN IF SQLCODE != -2289 THEN RAISE; END IF; END;
  BEGIN EXECUTE IMMEDIATE 'DROP SEQUENCE seq_citerne';
  EXCEPTION WHEN OTHERS THEN IF SQLCODE != -2289 THEN RAISE; END IF; END;
END;
/

-- =============================================================
-- ETAPE 3 : Creation des tables, sequences et triggers
--           Ordre : tables parents avant tables enfants
-- =============================================================

-- ----------------------------
-- EMPLOYE (independante)
-- ----------------------------
CREATE TABLE EMPLOYE (
    id_emp         NUMBER        CONSTRAINT nn_employe_id     NOT NULL,
    nom_emp        VARCHAR2(50)  CONSTRAINT nn_employe_nom    NOT NULL,
    prenom_emp     VARCHAR2(50)  CONSTRAINT nn_employe_prenom NOT NULL,
    email          VARCHAR2(100) CONSTRAINT nn_employe_email  NOT NULL,
    role           VARCHAR2(30)  CONSTRAINT nn_employe_role   NOT NULL,
    mdp            VARCHAR2(255) CONSTRAINT nn_employe_mdp    NOT NULL,
    photo          BLOB,
    empreinte      BLOB,
    modele_faciale BLOB,
    CONSTRAINT pk_employe       PRIMARY KEY (id_emp),
    CONSTRAINT uq_employe_email UNIQUE      (email)
);

CREATE SEQUENCE seq_emp START WITH 1 INCREMENT BY 1;

CREATE OR REPLACE TRIGGER trg_emp
BEFORE INSERT ON EMPLOYE
FOR EACH ROW
BEGIN
    :NEW.id_emp := seq_emp.NEXTVAL;
END;
/

-- ----------------------------
-- SERIE_MACHINE (independante)
-- ----------------------------
CREATE TABLE SERIE_MACHINE (
    id_serie            NUMBER        CONSTRAINT nn_serie_id      NOT NULL,
    nom_serie           VARCHAR2(50)  CONSTRAINT nn_serie_nom     NOT NULL,
    capacite_production NUMBER        CONSTRAINT nn_serie_capacite NOT NULL,
    date_mise_service   DATE,
    etat_serie          VARCHAR2(30),
    responsable         VARCHAR2(100),
    description         VARCHAR2(255),
    CONSTRAINT pk_serie_machine PRIMARY KEY (id_serie)
);

CREATE SEQUENCE seq_serie START WITH 1 INCREMENT BY 1;

CREATE OR REPLACE TRIGGER trg_serie
BEFORE INSERT ON SERIE_MACHINE
FOR EACH ROW
BEGIN
    :NEW.id_serie := seq_serie.NEXTVAL;
END;
/

-- ----------------------------
-- MACHINE (-> SERIE_MACHINE)
-- ----------------------------
CREATE TABLE MACHINE (
    id_machine            NUMBER       CONSTRAINT nn_machine_id      NOT NULL,
    nom_machine           VARCHAR2(50) CONSTRAINT nn_machine_nom     NOT NULL,
    type_machine          VARCHAR2(50),
    etat_machine          VARCHAR2(30),
    date_installation     DATE,
    heures_fonctionnement NUMBER       DEFAULT 0,
    temperature_actuelle  NUMBER(5,2),
    id_serie              NUMBER       CONSTRAINT nn_machine_serie   NOT NULL,
    CONSTRAINT pk_machine       PRIMARY KEY (id_machine),
    CONSTRAINT fk_machine_serie FOREIGN KEY (id_serie) REFERENCES SERIE_MACHINE(id_serie)
        ON DELETE CASCADE
);

CREATE SEQUENCE seq_machine START WITH 1 INCREMENT BY 1;

CREATE OR REPLACE TRIGGER trg_machine
BEFORE INSERT ON MACHINE
FOR EACH ROW
BEGIN
    :NEW.id_machine := seq_machine.NEXTVAL;
END;
/

-- ----------------------------
-- EMP_MACH (-> SERIE_MACHINE, EMPLOYE)
-- ----------------------------
CREATE TABLE EMP_MACH (
    id_serie NUMBER CONSTRAINT nn_empmach_serie NOT NULL,
    id_emp   NUMBER CONSTRAINT nn_empmach_emp   NOT NULL,
    CONSTRAINT pk_emp_mach       PRIMARY KEY (id_serie, id_emp),
    CONSTRAINT fk_emp_mach_serie FOREIGN KEY (id_serie) REFERENCES SERIE_MACHINE(id_serie)
        ON DELETE CASCADE,
    CONSTRAINT fk_emp_mach_emp   FOREIGN KEY (id_emp)   REFERENCES EMPLOYE(id_emp)
        ON DELETE CASCADE
);

-- ----------------------------
-- AGRICULTEUR (independante)
-- ----------------------------
CREATE TABLE AGRICULTEUR (
    id_agri            NUMBER        CONSTRAINT nn_agri_id  NOT NULL,
    nom_agri           VARCHAR2(50)  CONSTRAINT nn_agri_nom NOT NULL,
    prenom_agri        VARCHAR2(50),
    adresse_agri       VARCHAR2(255),
    num_agri           VARCHAR2(20),
    mail_agri          VARCHAR2(100),
    region_agri        VARCHAR2(50),
    nb_arbres          NUMBER,
    type_olives        VARCHAR2(50),
    qtolives_anneeprec NUMBER,
    rende_moy          NUMBER(5,2),
    date_premvisi      DATE,
    note_qualtmoy      NUMBER(5,2),
    daterecolte        DATE,
    CONSTRAINT pk_agriculteur PRIMARY KEY (id_agri)
);

CREATE SEQUENCE seq_agri START WITH 1 INCREMENT BY 1;

CREATE OR REPLACE TRIGGER trg_agri
BEFORE INSERT ON AGRICULTEUR
FOR EACH ROW
BEGIN
    :NEW.id_agri := seq_agri.NEXTVAL;
END;
/

-- ----------------------------
-- STOCK (-> AGRICULTEUR, SERIE_MACHINE)
-- ----------------------------
CREATE TABLE STOCK (
    id_stock       NUMBER       CONSTRAINT nn_stock_id NOT NULL,
    nom_stock      VARCHAR2(50),
    categ_stock    VARCHAR2(30),
    dateajt_stock  DATE         DEFAULT SYSDATE,
    descript_stock VARCHAR2(255),
    datemaj_stock  DATE,
    qt_stock       NUMBER,
    id_agri        NUMBER,
    id_serie       NUMBER,
    CONSTRAINT pk_stock       PRIMARY KEY (id_stock),
    CONSTRAINT fk_stock_agri  FOREIGN KEY (id_agri)  REFERENCES AGRICULTEUR(id_agri)
        ON DELETE CASCADE,
    CONSTRAINT fk_stock_serie FOREIGN KEY (id_serie) REFERENCES SERIE_MACHINE(id_serie)
        ON DELETE CASCADE
);

CREATE SEQUENCE seq_stock START WITH 1 INCREMENT BY 1;

CREATE OR REPLACE TRIGGER trg_stock
BEFORE INSERT ON STOCK
FOR EACH ROW
BEGIN
    :NEW.id_stock := seq_stock.NEXTVAL;
END;
/

-- ----------------------------
-- QUALITE (-> STOCK)
-- ----------------------------
CREATE TABLE QUALITE (
    id_lot                 NUMBER        CONSTRAINT nn_qualite_id NOT NULL,
    date_production        DATE,
    quantite_produite      NUMBER,
    temperature_production NUMBER(5,2),
    ph                     NUMBER(4,2),
    acidite                NUMBER(4,2),
    amerture               NUMBER(4,2),
    score                  NUMBER(5,2),
    code_couleur           VARCHAR2(20),
    statut_qualite         VARCHAR2(30),
    responsable_controle   VARCHAR2(100),
    max_quantite           NUMBER,
    id_stock               NUMBER,
    CONSTRAINT pk_qualite       PRIMARY KEY (id_lot),
    CONSTRAINT fk_qualite_stock FOREIGN KEY (id_stock) REFERENCES STOCK(id_stock)
        ON DELETE CASCADE
);

CREATE SEQUENCE seq_lot START WITH 1 INCREMENT BY 1;

CREATE OR REPLACE TRIGGER trg_lot
BEFORE INSERT ON QUALITE
FOR EACH ROW
BEGIN
    :NEW.id_lot := seq_lot.NEXTVAL;
END;
/

-- ----------------------------
-- CITERNE (-> QUALITE)
-- ----------------------------
CREATE TABLE CITERNE (
    id_citerne          NUMBER      CONSTRAINT nn_citerne_id  NOT NULL,
    capaciteMax         NUMBER      CONSTRAINT nn_citerne_cap NOT NULL,
    niveauActuel        NUMBER,
    typeHuile           VARCHAR2(30),
    temperature_citerne NUMBER(5,2),
    etat_citerne        VARCHAR2(30),
    id_lot              NUMBER,
    CONSTRAINT pk_citerne     PRIMARY KEY (id_citerne),
    CONSTRAINT fk_citerne_lot FOREIGN KEY (id_lot) REFERENCES QUALITE(id_lot)
        ON DELETE SET NULL
);

CREATE SEQUENCE seq_citerne START WITH 1 INCREMENT BY 1;

CREATE OR REPLACE TRIGGER trg_citerne
BEFORE INSERT ON CITERNE
FOR EACH ROW
BEGIN
    :NEW.id_citerne := seq_citerne.NEXTVAL;
END;
/

-- =============================================================
-- Fin du script
-- =============================================================
