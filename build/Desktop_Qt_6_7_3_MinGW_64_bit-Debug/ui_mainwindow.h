/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.7.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDateEdit>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QStackedWidget *metierspersonnel;
    QWidget *ajoutpersonnel;
    QLineEdit *nom;
    QLabel *nomLabel;
    QLabel *errorNom;
    QLabel *prNom;
    QLineEdit *Prenom;
    QLabel *errorPrenom;
    QLabel *dateDEmbaucheLabel;
    QLineEdit *Numero;
    QLabel *errorNumero;
    QLabel *label;
    QLineEdit *adresse;
    QLabel *errorAdresse;
    QLabel *label_3;
    QLineEdit *Mail;
    QLabel *label_2;
    QLabel *errorMail;
    QLineEdit *Region;
    QPushButton *quitter;
    QPushButton *ajouterEmpBtn;
    QLabel *errorRegion;
    QLabel *label_4;
    QWidget *consulterpersonnel;
    QVBoxLayout *consulterLayout;
    QHBoxLayout *horizontalLayout;
    QLineEdit *lineEdit;
    QLabel *label_12;
    QComboBox *comboTri;
    QComboBox *comboOrdre;
    QPushButton *btnTrier;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer;
    QToolButton *toolButton_2;
    QToolButton *toolButton_4;
    QToolButton *toolButton_3;
    QToolButton *toolButton;
    QTableWidget *tableau;
    QHBoxLayout *exportRow;
    QSpacerItem *horizontalSpacer_export;
    QPushButton *pushButton;
    QWidget *modifier;
    QLabel *nomLabel_2;
    QLineEdit *Mnom;
    QLabel *errorMNom;
    QLineEdit *Mprenom;
    QLabel *dateDEmbaucheLabel_2;
    QLabel *prNom_2;
    QLabel *errorMPrenom;
    QLineEdit *Mnumero;
    QLabel *errorMNumero;
    QLineEdit *Madresse;
    QLineEdit *MnbArbre;
    QLineEdit *Mtypeolive;
    QLineEdit *Mmail;
    QLineEdit *Mregion;
    QPushButton *retour;
    QPushButton *btn_valider_modif;
    QLabel *label_11;
    QLabel *label_10;
    QLabel *label_9;
    QLabel *label_8;
    QLabel *label_7;
    QLabel *errorMAdresse;
    QLabel *errorMMail;
    QLabel *errorMRegion;
    QWidget *historique;
    QTableWidget *tableHistorique;
    QLineEdit *noteH;
    QLabel *label_13;
    QPushButton *btnAjouterHistorique;
    QLabel *label_14;
    QLabel *label_15;
    QLabel *label_16;
    QLabel *label_17;
    QLineEdit *typeH;
    QLineEdit *nbArbreH;
    QLineEdit *quantiteH;
    QLineEdit *anneeH;
    QPushButton *btn_retour;
    QLabel *label_6;
    QDateEdit *dateRecolteH;
    QWidget *page;
    QLineEdit *typeH_2;
    QLineEdit *nbArbreH_2;
    QLineEdit *anneeH_2;
    QLineEdit *quantiteH_2;
    QLabel *label_18;
    QDateEdit *dateRecolteH_2;
    QLabel *label_19;
    QLabel *label_20;
    QLabel *label_21;
    QLineEdit *noteH_2;
    QLabel *label_22;
    QLabel *label_23;
    QLabel *label_24;
    QPushButton *retour_2;
    QPushButton *modifier_2;
    QWidget *statPersonnel;
    QVBoxLayout *statPersonnelLayout;
    QWidget *chartStatusContainer;
    QGroupBox *groupBox;
    QComboBox *choixtri;
    QLabel *label_5;
    QPushButton *btnok;
    QWidget *metieravancee;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1275, 760);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        metierspersonnel = new QStackedWidget(centralwidget);
        metierspersonnel->setObjectName("metierspersonnel");
        metierspersonnel->setGeometry(QRect(150, 30, 931, 691));
        ajoutpersonnel = new QWidget();
        ajoutpersonnel->setObjectName("ajoutpersonnel");
        nom = new QLineEdit(ajoutpersonnel);
        nom->setObjectName("nom");
        nom->setGeometry(QRect(240, 60, 558, 26));
        nomLabel = new QLabel(ajoutpersonnel);
        nomLabel->setObjectName("nomLabel");
        nomLabel->setGeometry(QRect(180, 60, 33, 21));
        errorNom = new QLabel(ajoutpersonnel);
        errorNom->setObjectName("errorNom");
        errorNom->setGeometry(QRect(240, 90, 558, 33));
        errorNom->setStyleSheet(QString::fromUtf8("color: red;\n"
"font-size: 11px;"));
        prNom = new QLabel(ajoutpersonnel);
        prNom->setObjectName("prNom");
        prNom->setGeometry(QRect(170, 130, 51, 26));
        Prenom = new QLineEdit(ajoutpersonnel);
        Prenom->setObjectName("Prenom");
        Prenom->setGeometry(QRect(240, 130, 558, 26));
        errorPrenom = new QLabel(ajoutpersonnel);
        errorPrenom->setObjectName("errorPrenom");
        errorPrenom->setGeometry(QRect(247, 160, 561, 31));
        errorPrenom->setStyleSheet(QString::fromUtf8("color: red;\n"
"font-size: 11px;"));
        dateDEmbaucheLabel = new QLabel(ajoutpersonnel);
        dateDEmbaucheLabel->setObjectName("dateDEmbaucheLabel");
        dateDEmbaucheLabel->setGeometry(QRect(170, 190, 54, 26));
        Numero = new QLineEdit(ajoutpersonnel);
        Numero->setObjectName("Numero");
        Numero->setGeometry(QRect(240, 190, 560, 26));
        errorNumero = new QLabel(ajoutpersonnel);
        errorNumero->setObjectName("errorNumero");
        errorNumero->setGeometry(QRect(249, 220, 551, 31));
        errorNumero->setStyleSheet(QString::fromUtf8("color: red;\n"
"font-size: 11px;"));
        label = new QLabel(ajoutpersonnel);
        label->setObjectName("label");
        label->setGeometry(QRect(170, 260, 52, 26));
        adresse = new QLineEdit(ajoutpersonnel);
        adresse->setObjectName("adresse");
        adresse->setGeometry(QRect(240, 260, 561, 26));
        errorAdresse = new QLabel(ajoutpersonnel);
        errorAdresse->setObjectName("errorAdresse");
        errorAdresse->setGeometry(QRect(240, 290, 561, 31));
        errorAdresse->setStyleSheet(QString::fromUtf8("color: red;\n"
"font-size: 11px;"));
        label_3 = new QLabel(ajoutpersonnel);
        label_3->setObjectName("label_3");
        label_3->setGeometry(QRect(180, 330, 29, 26));
        Mail = new QLineEdit(ajoutpersonnel);
        Mail->setObjectName("Mail");
        Mail->setGeometry(QRect(240, 330, 561, 26));
        label_2 = new QLabel(ajoutpersonnel);
        label_2->setObjectName("label_2");
        label_2->setGeometry(QRect(170, 400, 51, 26));
        errorMail = new QLabel(ajoutpersonnel);
        errorMail->setObjectName("errorMail");
        errorMail->setGeometry(QRect(240, 360, 561, 31));
        errorMail->setStyleSheet(QString::fromUtf8("color: red;\n"
"font-size: 11px;"));
        Region = new QLineEdit(ajoutpersonnel);
        Region->setObjectName("Region");
        Region->setGeometry(QRect(240, 400, 561, 26));
        quitter = new QPushButton(ajoutpersonnel);
        quitter->setObjectName("quitter");
        quitter->setGeometry(QRect(540, 470, 93, 29));
        ajouterEmpBtn = new QPushButton(ajoutpersonnel);
        ajouterEmpBtn->setObjectName("ajouterEmpBtn");
        ajouterEmpBtn->setGeometry(QRect(770, 470, 93, 29));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/img/export.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        ajouterEmpBtn->setIcon(icon);
        ajouterEmpBtn->setIconSize(QSize(18, 18));
        errorRegion = new QLabel(ajoutpersonnel);
        errorRegion->setObjectName("errorRegion");
        errorRegion->setGeometry(QRect(240, 430, 561, 31));
        errorRegion->setStyleSheet(QString::fromUtf8("color: red;\n"
"font-size: 11px;"));
        label_4 = new QLabel(ajoutpersonnel);
        label_4->setObjectName("label_4");
        label_4->setGeometry(QRect(242, 20, 311, 20));
        QFont font;
        font.setPointSize(9);
        font.setBold(true);
        label_4->setFont(font);
        metierspersonnel->addWidget(ajoutpersonnel);
        consulterpersonnel = new QWidget();
        consulterpersonnel->setObjectName("consulterpersonnel");
        consulterLayout = new QVBoxLayout(consulterpersonnel);
        consulterLayout->setObjectName("consulterLayout");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        lineEdit = new QLineEdit(consulterpersonnel);
        lineEdit->setObjectName("lineEdit");

        horizontalLayout->addWidget(lineEdit);

        label_12 = new QLabel(consulterpersonnel);
        label_12->setObjectName("label_12");

        horizontalLayout->addWidget(label_12);

        comboTri = new QComboBox(consulterpersonnel);
        comboTri->addItem(QString());
        comboTri->addItem(QString());
        comboTri->addItem(QString());
        comboTri->addItem(QString());
        comboTri->setObjectName("comboTri");

        horizontalLayout->addWidget(comboTri);

        comboOrdre = new QComboBox(consulterpersonnel);
        comboOrdre->addItem(QString());
        comboOrdre->addItem(QString());
        comboOrdre->setObjectName("comboOrdre");

        horizontalLayout->addWidget(comboOrdre);

        btnTrier = new QPushButton(consulterpersonnel);
        btnTrier->setObjectName("btnTrier");

        horizontalLayout->addWidget(btnTrier);


        consulterLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);

        toolButton_2 = new QToolButton(consulterpersonnel);
        toolButton_2->setObjectName("toolButton_2");
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/img/refresh.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        toolButton_2->setIcon(icon1);
        toolButton_2->setAutoRaise(true);

        horizontalLayout_2->addWidget(toolButton_2);

        toolButton_4 = new QToolButton(consulterpersonnel);
        toolButton_4->setObjectName("toolButton_4");
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/img/filter.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        toolButton_4->setIcon(icon2);
        toolButton_4->setAutoRaise(true);

        horizontalLayout_2->addWidget(toolButton_4);

        toolButton_3 = new QToolButton(consulterpersonnel);
        toolButton_3->setObjectName("toolButton_3");
        toolButton_3->setIcon(icon);
        toolButton_3->setAutoRaise(true);

        horizontalLayout_2->addWidget(toolButton_3);

        toolButton = new QToolButton(consulterpersonnel);
        toolButton->setObjectName("toolButton");
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/img/search.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        toolButton->setIcon(icon3);
        toolButton->setAutoRaise(true);

        horizontalLayout_2->addWidget(toolButton);


        consulterLayout->addLayout(horizontalLayout_2);

        tableau = new QTableWidget(consulterpersonnel);
        if (tableau->columnCount() < 10)
            tableau->setColumnCount(10);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        tableau->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        tableau->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        tableau->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        tableau->setHorizontalHeaderItem(3, __qtablewidgetitem3);
        QTableWidgetItem *__qtablewidgetitem4 = new QTableWidgetItem();
        tableau->setHorizontalHeaderItem(4, __qtablewidgetitem4);
        QTableWidgetItem *__qtablewidgetitem5 = new QTableWidgetItem();
        tableau->setHorizontalHeaderItem(5, __qtablewidgetitem5);
        QTableWidgetItem *__qtablewidgetitem6 = new QTableWidgetItem();
        tableau->setHorizontalHeaderItem(6, __qtablewidgetitem6);
        QTableWidgetItem *__qtablewidgetitem7 = new QTableWidgetItem();
        tableau->setHorizontalHeaderItem(7, __qtablewidgetitem7);
        QTableWidgetItem *__qtablewidgetitem8 = new QTableWidgetItem();
        tableau->setHorizontalHeaderItem(8, __qtablewidgetitem8);
        QTableWidgetItem *__qtablewidgetitem9 = new QTableWidgetItem();
        tableau->setHorizontalHeaderItem(9, __qtablewidgetitem9);
        if (tableau->rowCount() < 1)
            tableau->setRowCount(1);
        QTableWidgetItem *__qtablewidgetitem10 = new QTableWidgetItem();
        tableau->setItem(0, 0, __qtablewidgetitem10);
        QTableWidgetItem *__qtablewidgetitem11 = new QTableWidgetItem();
        tableau->setItem(0, 1, __qtablewidgetitem11);
        QTableWidgetItem *__qtablewidgetitem12 = new QTableWidgetItem();
        tableau->setItem(0, 2, __qtablewidgetitem12);
        QTableWidgetItem *__qtablewidgetitem13 = new QTableWidgetItem();
        tableau->setItem(0, 3, __qtablewidgetitem13);
        QTableWidgetItem *__qtablewidgetitem14 = new QTableWidgetItem();
        tableau->setItem(0, 4, __qtablewidgetitem14);
        QTableWidgetItem *__qtablewidgetitem15 = new QTableWidgetItem();
        tableau->setItem(0, 5, __qtablewidgetitem15);
        QTableWidgetItem *__qtablewidgetitem16 = new QTableWidgetItem();
        tableau->setItem(0, 6, __qtablewidgetitem16);
        QTableWidgetItem *__qtablewidgetitem17 = new QTableWidgetItem();
        tableau->setItem(0, 7, __qtablewidgetitem17);
        QTableWidgetItem *__qtablewidgetitem18 = new QTableWidgetItem();
        tableau->setItem(0, 8, __qtablewidgetitem18);
        QTableWidgetItem *__qtablewidgetitem19 = new QTableWidgetItem();
        tableau->setItem(0, 9, __qtablewidgetitem19);
        tableau->setObjectName("tableau");

        consulterLayout->addWidget(tableau);

        exportRow = new QHBoxLayout();
        exportRow->setObjectName("exportRow");
        horizontalSpacer_export = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        exportRow->addItem(horizontalSpacer_export);

        pushButton = new QPushButton(consulterpersonnel);
        pushButton->setObjectName("pushButton");
        pushButton->setIcon(icon);
        pushButton->setIconSize(QSize(18, 18));

        exportRow->addWidget(pushButton);


        consulterLayout->addLayout(exportRow);

        metierspersonnel->addWidget(consulterpersonnel);
        modifier = new QWidget();
        modifier->setObjectName("modifier");
        nomLabel_2 = new QLabel(modifier);
        nomLabel_2->setObjectName("nomLabel_2");
        nomLabel_2->setGeometry(QRect(140, 70, 33, 26));
        Mnom = new QLineEdit(modifier);
        Mnom->setObjectName("Mnom");
        Mnom->setGeometry(QRect(200, 70, 452, 26));
        errorMNom = new QLabel(modifier);
        errorMNom->setObjectName("errorMNom");
        errorMNom->setGeometry(QRect(200, 100, 451, 20));
        errorMNom->setStyleSheet(QString::fromUtf8("color: red;\n"
"font-size: 11px;"));
        Mprenom = new QLineEdit(modifier);
        Mprenom->setObjectName("Mprenom");
        Mprenom->setGeometry(QRect(200, 130, 452, 26));
        dateDEmbaucheLabel_2 = new QLabel(modifier);
        dateDEmbaucheLabel_2->setObjectName("dateDEmbaucheLabel_2");
        dateDEmbaucheLabel_2->setGeometry(QRect(130, 190, 54, 26));
        prNom_2 = new QLabel(modifier);
        prNom_2->setObjectName("prNom_2");
        prNom_2->setGeometry(QRect(130, 120, 51, 35));
        errorMPrenom = new QLabel(modifier);
        errorMPrenom->setObjectName("errorMPrenom");
        errorMPrenom->setGeometry(QRect(200, 160, 451, 21));
        errorMPrenom->setStyleSheet(QString::fromUtf8("color: red;\n"
"font-size: 11px;"));
        Mnumero = new QLineEdit(modifier);
        Mnumero->setObjectName("Mnumero");
        Mnumero->setGeometry(QRect(200, 190, 452, 26));
        errorMNumero = new QLabel(modifier);
        errorMNumero->setObjectName("errorMNumero");
        errorMNumero->setGeometry(QRect(200, 220, 451, 21));
        errorMNumero->setStyleSheet(QString::fromUtf8("color: red;\n"
"font-size: 11px;"));
        Madresse = new QLineEdit(modifier);
        Madresse->setObjectName("Madresse");
        Madresse->setGeometry(QRect(200, 250, 452, 26));
        MnbArbre = new QLineEdit(modifier);
        MnbArbre->setObjectName("MnbArbre");
        MnbArbre->setGeometry(QRect(200, 300, 452, 26));
        Mtypeolive = new QLineEdit(modifier);
        Mtypeolive->setObjectName("Mtypeolive");
        Mtypeolive->setGeometry(QRect(200, 350, 452, 26));
        Mmail = new QLineEdit(modifier);
        Mmail->setObjectName("Mmail");
        Mmail->setGeometry(QRect(200, 400, 452, 26));
        Mregion = new QLineEdit(modifier);
        Mregion->setObjectName("Mregion");
        Mregion->setGeometry(QRect(200, 450, 452, 26));
        retour = new QPushButton(modifier);
        retour->setObjectName("retour");
        retour->setGeometry(QRect(450, 560, 93, 29));
        btn_valider_modif = new QPushButton(modifier);
        btn_valider_modif->setObjectName("btn_valider_modif");
        btn_valider_modif->setGeometry(QRect(630, 560, 93, 29));
        btn_valider_modif->setIcon(icon);
        btn_valider_modif->setIconSize(QSize(18, 18));
        label_11 = new QLabel(modifier);
        label_11->setObjectName("label_11");
        label_11->setGeometry(QRect(130, 450, 51, 35));
        label_10 = new QLabel(modifier);
        label_10->setObjectName("label_10");
        label_10->setGeometry(QRect(140, 400, 29, 35));
        label_9 = new QLabel(modifier);
        label_9->setObjectName("label_9");
        label_9->setGeometry(QRect(100, 340, 79, 35));
        label_8 = new QLabel(modifier);
        label_8->setObjectName("label_8");
        label_8->setGeometry(QRect(80, 290, 110, 35));
        label_7 = new QLabel(modifier);
        label_7->setObjectName("label_7");
        label_7->setGeometry(QRect(130, 240, 52, 35));
        errorMAdresse = new QLabel(modifier);
        errorMAdresse->setObjectName("errorMAdresse");
        errorMAdresse->setGeometry(QRect(200, 280, 451, 20));
        errorMMail = new QLabel(modifier);
        errorMMail->setObjectName("errorMMail");
        errorMMail->setGeometry(QRect(200, 430, 451, 20));
        errorMMail->setStyleSheet(QString::fromUtf8("color: red;\n"
"font-size: 11px;"));
        errorMRegion = new QLabel(modifier);
        errorMRegion->setObjectName("errorMRegion");
        errorMRegion->setGeometry(QRect(200, 480, 451, 21));
        errorMRegion->setStyleSheet(QString::fromUtf8("color: red;\n"
"font-size: 11px;"));
        metierspersonnel->addWidget(modifier);
        historique = new QWidget();
        historique->setObjectName("historique");
        tableHistorique = new QTableWidget(historique);
        if (tableHistorique->columnCount() < 6)
            tableHistorique->setColumnCount(6);
        QTableWidgetItem *__qtablewidgetitem20 = new QTableWidgetItem();
        tableHistorique->setHorizontalHeaderItem(0, __qtablewidgetitem20);
        QTableWidgetItem *__qtablewidgetitem21 = new QTableWidgetItem();
        tableHistorique->setHorizontalHeaderItem(1, __qtablewidgetitem21);
        QTableWidgetItem *__qtablewidgetitem22 = new QTableWidgetItem();
        tableHistorique->setHorizontalHeaderItem(2, __qtablewidgetitem22);
        QTableWidgetItem *__qtablewidgetitem23 = new QTableWidgetItem();
        tableHistorique->setHorizontalHeaderItem(3, __qtablewidgetitem23);
        QTableWidgetItem *__qtablewidgetitem24 = new QTableWidgetItem();
        tableHistorique->setHorizontalHeaderItem(4, __qtablewidgetitem24);
        QTableWidgetItem *__qtablewidgetitem25 = new QTableWidgetItem();
        tableHistorique->setHorizontalHeaderItem(5, __qtablewidgetitem25);
        tableHistorique->setObjectName("tableHistorique");
        tableHistorique->setGeometry(QRect(20, 350, 631, 171));
        noteH = new QLineEdit(historique);
        noteH->setObjectName("noteH");
        noteH->setGeometry(QRect(240, 260, 281, 26));
        label_13 = new QLabel(historique);
        label_13->setObjectName("label_13");
        label_13->setGeometry(QRect(160, 260, 63, 20));
        btnAjouterHistorique = new QPushButton(historique);
        btnAjouterHistorique->setObjectName("btnAjouterHistorique");
        btnAjouterHistorique->setGeometry(QRect(550, 530, 93, 29));
        label_14 = new QLabel(historique);
        label_14->setObjectName("label_14");
        label_14->setGeometry(QRect(160, 210, 63, 20));
        label_15 = new QLabel(historique);
        label_15->setObjectName("label_15");
        label_15->setGeometry(QRect(120, 160, 121, 20));
        label_16 = new QLabel(historique);
        label_16->setObjectName("label_16");
        label_16->setGeometry(QRect(150, 120, 63, 20));
        label_17 = new QLabel(historique);
        label_17->setObjectName("label_17");
        label_17->setGeometry(QRect(160, 70, 63, 20));
        typeH = new QLineEdit(historique);
        typeH->setObjectName("typeH");
        typeH->setGeometry(QRect(240, 210, 281, 26));
        nbArbreH = new QLineEdit(historique);
        nbArbreH->setObjectName("nbArbreH");
        nbArbreH->setGeometry(QRect(240, 160, 281, 26));
        quantiteH = new QLineEdit(historique);
        quantiteH->setObjectName("quantiteH");
        quantiteH->setGeometry(QRect(240, 110, 281, 26));
        anneeH = new QLineEdit(historique);
        anneeH->setObjectName("anneeH");
        anneeH->setGeometry(QRect(240, 60, 281, 26));
        btn_retour = new QPushButton(historique);
        btn_retour->setObjectName("btn_retour");
        btn_retour->setGeometry(QRect(440, 530, 93, 29));
        label_6 = new QLabel(historique);
        label_6->setObjectName("label_6");
        label_6->setGeometry(QRect(130, 300, 91, 20));
        dateRecolteH = new QDateEdit(historique);
        dateRecolteH->setObjectName("dateRecolteH");
        dateRecolteH->setGeometry(QRect(240, 300, 281, 26));
        metierspersonnel->addWidget(historique);
        page = new QWidget();
        page->setObjectName("page");
        typeH_2 = new QLineEdit(page);
        typeH_2->setObjectName("typeH_2");
        typeH_2->setGeometry(QRect(360, 270, 281, 26));
        nbArbreH_2 = new QLineEdit(page);
        nbArbreH_2->setObjectName("nbArbreH_2");
        nbArbreH_2->setGeometry(QRect(360, 220, 281, 26));
        anneeH_2 = new QLineEdit(page);
        anneeH_2->setObjectName("anneeH_2");
        anneeH_2->setGeometry(QRect(360, 120, 281, 26));
        quantiteH_2 = new QLineEdit(page);
        quantiteH_2->setObjectName("quantiteH_2");
        quantiteH_2->setGeometry(QRect(360, 170, 281, 26));
        label_18 = new QLabel(page);
        label_18->setObjectName("label_18");
        label_18->setGeometry(QRect(280, 270, 63, 20));
        dateRecolteH_2 = new QDateEdit(page);
        dateRecolteH_2->setObjectName("dateRecolteH_2");
        dateRecolteH_2->setGeometry(QRect(360, 360, 281, 26));
        label_19 = new QLabel(page);
        label_19->setObjectName("label_19");
        label_19->setGeometry(QRect(270, 180, 63, 20));
        label_20 = new QLabel(page);
        label_20->setObjectName("label_20");
        label_20->setGeometry(QRect(280, 130, 63, 20));
        label_21 = new QLabel(page);
        label_21->setObjectName("label_21");
        label_21->setGeometry(QRect(250, 360, 91, 20));
        noteH_2 = new QLineEdit(page);
        noteH_2->setObjectName("noteH_2");
        noteH_2->setGeometry(QRect(360, 320, 281, 26));
        label_22 = new QLabel(page);
        label_22->setObjectName("label_22");
        label_22->setGeometry(QRect(240, 220, 121, 20));
        label_23 = new QLabel(page);
        label_23->setObjectName("label_23");
        label_23->setGeometry(QRect(280, 320, 63, 20));
        label_24 = new QLabel(page);
        label_24->setObjectName("label_24");
        label_24->setGeometry(QRect(130, 60, 391, 20));
        retour_2 = new QPushButton(page);
        retour_2->setObjectName("retour_2");
        retour_2->setGeometry(QRect(520, 440, 93, 29));
        modifier_2 = new QPushButton(page);
        modifier_2->setObjectName("modifier_2");
        modifier_2->setGeometry(QRect(690, 440, 93, 29));
        metierspersonnel->addWidget(page);
        statPersonnel = new QWidget();
        statPersonnel->setObjectName("statPersonnel");
        statPersonnelLayout = new QVBoxLayout(statPersonnel);
        statPersonnelLayout->setObjectName("statPersonnelLayout");
        chartStatusContainer = new QWidget(statPersonnel);
        chartStatusContainer->setObjectName("chartStatusContainer");
        groupBox = new QGroupBox(chartStatusContainer);
        groupBox->setObjectName("groupBox");
        groupBox->setGeometry(QRect(80, 80, 481, 401));
        choixtri = new QComboBox(groupBox);
        choixtri->addItem(QString());
        choixtri->addItem(QString());
        choixtri->setObjectName("choixtri");
        choixtri->setGeometry(QRect(30, 80, 271, 31));
        label_5 = new QLabel(groupBox);
        label_5->setObjectName("label_5");
        label_5->setGeometry(QRect(20, 39, 201, 21));
        btnok = new QPushButton(groupBox);
        btnok->setObjectName("btnok");
        btnok->setGeometry(QRect(320, 80, 91, 31));

        statPersonnelLayout->addWidget(chartStatusContainer);

        metierspersonnel->addWidget(statPersonnel);
        metieravancee = new QWidget();
        metieravancee->setObjectName("metieravancee");
        metierspersonnel->addWidget(metieravancee);
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1275, 26));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        metierspersonnel->setCurrentIndex(2);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        nom->setPlaceholderText(QCoreApplication::translate("MainWindow", "Nom", nullptr));
        nomLabel->setText(QCoreApplication::translate("MainWindow", "Nom", nullptr));
        errorNom->setText(QString());
        prNom->setText(QCoreApplication::translate("MainWindow", "Pr\303\251nom", nullptr));
        Prenom->setText(QString());
        Prenom->setPlaceholderText(QCoreApplication::translate("MainWindow", "Pr\303\251nom", nullptr));
        errorPrenom->setText(QString());
        dateDEmbaucheLabel->setText(QCoreApplication::translate("MainWindow", "Numero", nullptr));
        Numero->setPlaceholderText(QCoreApplication::translate("MainWindow", "Numero", nullptr));
        errorNumero->setText(QString());
        label->setText(QCoreApplication::translate("MainWindow", "Adresse", nullptr));
        adresse->setText(QString());
        adresse->setPlaceholderText(QCoreApplication::translate("MainWindow", "Adresse", nullptr));
        errorAdresse->setText(QString());
        label_3->setText(QCoreApplication::translate("MainWindow", "Mail", nullptr));
        Mail->setPlaceholderText(QCoreApplication::translate("MainWindow", "Mail", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "Region ", nullptr));
        errorMail->setText(QString());
        Region->setPlaceholderText(QCoreApplication::translate("MainWindow", "Region", nullptr));
        quitter->setText(QCoreApplication::translate("MainWindow", "Quiter", nullptr));
        ajouterEmpBtn->setText(QCoreApplication::translate("MainWindow", "Ajouter", nullptr));
        errorRegion->setText(QString());
        label_4->setText(QCoreApplication::translate("MainWindow", "Ajouter agriculteur :", nullptr));
        label_12->setText(QCoreApplication::translate("MainWindow", "Trier par :  ", nullptr));
        comboTri->setItemText(0, QCoreApplication::translate("MainWindow", "Aucun", nullptr));
        comboTri->setItemText(1, QCoreApplication::translate("MainWindow", "Rendement Moyen", nullptr));
        comboTri->setItemText(2, QCoreApplication::translate("MainWindow", "Quantit\303\251 apport\303\251", nullptr));
        comboTri->setItemText(3, QCoreApplication::translate("MainWindow", "Region", nullptr));

        comboTri->setPlaceholderText(QCoreApplication::translate("MainWindow", "Trier", nullptr));
        comboOrdre->setItemText(0, QCoreApplication::translate("MainWindow", "Ascendant", nullptr));
        comboOrdre->setItemText(1, QCoreApplication::translate("MainWindow", "Descendant", nullptr));

        comboOrdre->setCurrentText(QString());
        comboOrdre->setPlaceholderText(QCoreApplication::translate("MainWindow", "Ordre", nullptr));
        btnTrier->setText(QCoreApplication::translate("MainWindow", "Trier", nullptr));
        toolButton_2->setText(QCoreApplication::translate("MainWindow", "...", nullptr));
        toolButton_4->setText(QCoreApplication::translate("MainWindow", "...", nullptr));
        toolButton_3->setText(QCoreApplication::translate("MainWindow", "...", nullptr));
        toolButton->setText(QCoreApplication::translate("MainWindow", "...", nullptr));
        QTableWidgetItem *___qtablewidgetitem = tableau->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QCoreApplication::translate("MainWindow", "Id", nullptr));
        QTableWidgetItem *___qtablewidgetitem1 = tableau->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QCoreApplication::translate("MainWindow", "Nom", nullptr));
        QTableWidgetItem *___qtablewidgetitem2 = tableau->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QCoreApplication::translate("MainWindow", "Pr\303\251nom", nullptr));
        QTableWidgetItem *___qtablewidgetitem3 = tableau->horizontalHeaderItem(3);
        ___qtablewidgetitem3->setText(QCoreApplication::translate("MainWindow", "Num\303\251ro", nullptr));
        QTableWidgetItem *___qtablewidgetitem4 = tableau->horizontalHeaderItem(4);
        ___qtablewidgetitem4->setText(QCoreApplication::translate("MainWindow", "Adresse", nullptr));
        QTableWidgetItem *___qtablewidgetitem5 = tableau->horizontalHeaderItem(5);
        ___qtablewidgetitem5->setText(QCoreApplication::translate("MainWindow", "Nombre d'arbre", nullptr));
        QTableWidgetItem *___qtablewidgetitem6 = tableau->horizontalHeaderItem(6);
        ___qtablewidgetitem6->setText(QCoreApplication::translate("MainWindow", "Type d'huile", nullptr));
        QTableWidgetItem *___qtablewidgetitem7 = tableau->horizontalHeaderItem(7);
        ___qtablewidgetitem7->setText(QCoreApplication::translate("MainWindow", "Mail", nullptr));
        QTableWidgetItem *___qtablewidgetitem8 = tableau->horizontalHeaderItem(8);
        ___qtablewidgetitem8->setText(QCoreApplication::translate("MainWindow", "Region", nullptr));
        QTableWidgetItem *___qtablewidgetitem9 = tableau->horizontalHeaderItem(9);
        ___qtablewidgetitem9->setText(QCoreApplication::translate("MainWindow", "Actions", nullptr));

        const bool __sortingEnabled = tableau->isSortingEnabled();
        tableau->setSortingEnabled(false);
        QTableWidgetItem *___qtablewidgetitem10 = tableau->item(0, 0);
        ___qtablewidgetitem10->setText(QCoreApplication::translate("MainWindow", "1", nullptr));
        QTableWidgetItem *___qtablewidgetitem11 = tableau->item(0, 1);
        ___qtablewidgetitem11->setText(QCoreApplication::translate("MainWindow", "Bejaoui", nullptr));
        QTableWidgetItem *___qtablewidgetitem12 = tableau->item(0, 2);
        ___qtablewidgetitem12->setText(QCoreApplication::translate("MainWindow", "Abdelmajid", nullptr));
        QTableWidgetItem *___qtablewidgetitem13 = tableau->item(0, 3);
        ___qtablewidgetitem13->setText(QCoreApplication::translate("MainWindow", "52307668", nullptr));
        QTableWidgetItem *___qtablewidgetitem14 = tableau->item(0, 4);
        ___qtablewidgetitem14->setText(QCoreApplication::translate("MainWindow", "Mourouj 6", nullptr));
        QTableWidgetItem *___qtablewidgetitem15 = tableau->item(0, 5);
        ___qtablewidgetitem15->setText(QCoreApplication::translate("MainWindow", "20", nullptr));
        QTableWidgetItem *___qtablewidgetitem16 = tableau->item(0, 6);
        ___qtablewidgetitem16->setText(QCoreApplication::translate("MainWindow", "olive", nullptr));
        QTableWidgetItem *___qtablewidgetitem17 = tableau->item(0, 7);
        ___qtablewidgetitem17->setText(QCoreApplication::translate("MainWindow", "abdelmajidbejaoui05@gmail.com", nullptr));
        QTableWidgetItem *___qtablewidgetitem18 = tableau->item(0, 8);
        ___qtablewidgetitem18->setText(QCoreApplication::translate("MainWindow", "Ben arous", nullptr));
        tableau->setSortingEnabled(__sortingEnabled);

        pushButton->setText(QCoreApplication::translate("MainWindow", "Exporter", nullptr));
        nomLabel_2->setText(QCoreApplication::translate("MainWindow", "Nom", nullptr));
        Mnom->setPlaceholderText(QCoreApplication::translate("MainWindow", "Nom", nullptr));
        errorMNom->setText(QString());
        Mprenom->setText(QString());
        Mprenom->setPlaceholderText(QCoreApplication::translate("MainWindow", "Pr\303\251nom", nullptr));
        dateDEmbaucheLabel_2->setText(QCoreApplication::translate("MainWindow", "Numero", nullptr));
        prNom_2->setText(QCoreApplication::translate("MainWindow", "Pr\303\251nom", nullptr));
        errorMPrenom->setText(QString());
        Mnumero->setPlaceholderText(QCoreApplication::translate("MainWindow", "Numero", nullptr));
        errorMNumero->setText(QString());
        Madresse->setText(QString());
        Madresse->setPlaceholderText(QCoreApplication::translate("MainWindow", "Adresse", nullptr));
        MnbArbre->setPlaceholderText(QCoreApplication::translate("MainWindow", "Nombre d'arbre", nullptr));
        Mtypeolive->setPlaceholderText(QCoreApplication::translate("MainWindow", "Type d'huile", nullptr));
        Mmail->setPlaceholderText(QCoreApplication::translate("MainWindow", "Mail", nullptr));
        Mregion->setPlaceholderText(QCoreApplication::translate("MainWindow", "Region", nullptr));
        retour->setText(QCoreApplication::translate("MainWindow", "Retour", nullptr));
        btn_valider_modif->setText(QCoreApplication::translate("MainWindow", "Modifier", nullptr));
        label_11->setText(QCoreApplication::translate("MainWindow", "Region ", nullptr));
        label_10->setText(QCoreApplication::translate("MainWindow", "Mail", nullptr));
        label_9->setText(QCoreApplication::translate("MainWindow", "Type d'huile", nullptr));
        label_8->setText(QCoreApplication::translate("MainWindow", "Nombre d'arbre ", nullptr));
        label_7->setText(QCoreApplication::translate("MainWindow", "Adresse", nullptr));
        errorMAdresse->setText(QString());
        errorMMail->setText(QString());
        errorMRegion->setText(QString());
        QTableWidgetItem *___qtablewidgetitem19 = tableHistorique->horizontalHeaderItem(0);
        ___qtablewidgetitem19->setText(QCoreApplication::translate("MainWindow", "Ann\303\251e", nullptr));
        QTableWidgetItem *___qtablewidgetitem20 = tableHistorique->horizontalHeaderItem(1);
        ___qtablewidgetitem20->setText(QCoreApplication::translate("MainWindow", "Quantit\303\251", nullptr));
        QTableWidgetItem *___qtablewidgetitem21 = tableHistorique->horizontalHeaderItem(2);
        ___qtablewidgetitem21->setText(QCoreApplication::translate("MainWindow", "Nombre d'arbres", nullptr));
        QTableWidgetItem *___qtablewidgetitem22 = tableHistorique->horizontalHeaderItem(3);
        ___qtablewidgetitem22->setText(QCoreApplication::translate("MainWindow", "Type", nullptr));
        QTableWidgetItem *___qtablewidgetitem23 = tableHistorique->horizontalHeaderItem(4);
        ___qtablewidgetitem23->setText(QCoreApplication::translate("MainWindow", "Note", nullptr));
        QTableWidgetItem *___qtablewidgetitem24 = tableHistorique->horizontalHeaderItem(5);
        ___qtablewidgetitem24->setText(QCoreApplication::translate("MainWindow", "Date", nullptr));
        label_13->setText(QCoreApplication::translate("MainWindow", "Note :", nullptr));
        btnAjouterHistorique->setText(QCoreApplication::translate("MainWindow", "Ajouter", nullptr));
        label_14->setText(QCoreApplication::translate("MainWindow", "Type :", nullptr));
        label_15->setText(QCoreApplication::translate("MainWindow", "Nombre d'arbres :", nullptr));
        label_16->setText(QCoreApplication::translate("MainWindow", "Quantite :", nullptr));
        label_17->setText(QCoreApplication::translate("MainWindow", "Ann\303\251e :", nullptr));
        btn_retour->setText(QCoreApplication::translate("MainWindow", "Retour", nullptr));
        label_6->setText(QCoreApplication::translate("MainWindow", "Date recolte :", nullptr));
        label_18->setText(QCoreApplication::translate("MainWindow", "Type :", nullptr));
        label_19->setText(QCoreApplication::translate("MainWindow", "Quantite :", nullptr));
        label_20->setText(QCoreApplication::translate("MainWindow", "Ann\303\251e :", nullptr));
        label_21->setText(QCoreApplication::translate("MainWindow", "Date recolte :", nullptr));
        label_22->setText(QCoreApplication::translate("MainWindow", "Nombre d'arbres :", nullptr));
        label_23->setText(QCoreApplication::translate("MainWindow", "Note :", nullptr));
        label_24->setText(QCoreApplication::translate("MainWindow", "Modification d'un Stock :", nullptr));
        retour_2->setText(QCoreApplication::translate("MainWindow", "Retour", nullptr));
        modifier_2->setText(QCoreApplication::translate("MainWindow", "Modifier", nullptr));
        groupBox->setTitle(QCoreApplication::translate("MainWindow", "STATISTIQUE", nullptr));
        choixtri->setItemText(0, QCoreApplication::translate("MainWindow", "Quantit\303\251 totale par agriculteur ", nullptr));
        choixtri->setItemText(1, QCoreApplication::translate("MainWindow", "Rendement moyen par agriculteur ", nullptr));

        label_5->setText(QCoreApplication::translate("MainWindow", "Faire la statistique selon :", nullptr));
        btnok->setText(QCoreApplication::translate("MainWindow", "ok", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
