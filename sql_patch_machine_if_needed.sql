-- Patch optionnel si votre base locale ne contient pas encore SERIE_MACHINE
BEGIN
  EXECUTE IMMEDIATE 'CREATE TABLE SERIE_MACHINE (
'
    || '  id_serie NUMBER PRIMARY KEY,
'
    || '  nom_serie VARCHAR2(100) NOT NULL,
'
    || '  capacite_production NUMBER,
'
    || '  date_mise_service DATE,
'
    || '  etat_serie VARCHAR2(30),
'
    || '  responsable VARCHAR2(100),
'
    || '  description VARCHAR2(255)
'
    || ')';
EXCEPTION WHEN OTHERS THEN
  IF SQLCODE != -955 THEN RAISE; END IF;
END;
/

BEGIN
  EXECUTE IMMEDIATE 'ALTER TABLE MACHINE ADD (id_serie NUMBER)';
EXCEPTION WHEN OTHERS THEN
  IF SQLCODE != -1430 THEN RAISE; END IF;
END;
/

BEGIN
  EXECUTE IMMEDIATE 'ALTER TABLE MACHINE ADD CONSTRAINT fk_machine_serie FOREIGN KEY (id_serie) REFERENCES SERIE_MACHINE(id_serie)';
EXCEPTION WHEN OTHERS THEN
  IF SQLCODE != -2261 THEN RAISE; END IF;
END;
/
