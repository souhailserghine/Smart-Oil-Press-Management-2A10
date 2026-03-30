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
#include <QtWidgets/QFormLayout>
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
    QWidget *formLayoutWidget;
    QFormLayout *formLayout;
    QLabel *nomLabel;
    QLineEdit *nom;
    QLabel *prNom;
    QLineEdit *Prenom;
    QLabel *dateDEmbaucheLabel;
    QLineEdit *Numero;
    QLabel *label;
    QLineEdit *adresse;
    QLabel *label_4;
    QLineEdit *nbArbre;
    QLabel *label_6;
    QLineEdit *typeolive;
    QLabel *label_3;
    QLineEdit *Mail;
    QLabel *label_2;
    QLineEdit *Region;
    QHBoxLayout *qjouterEmpRow;
    QPushButton *quitter;
    QSpacerItem *horizontalSpacer_ajouteremp;
    QPushButton *ajouterEmpBtn;
    QWidget *consulterpersonnel;
    QVBoxLayout *consulterLayout;
    QHBoxLayout *horizontalLayout;
    QLineEdit *lineEdit;
    QComboBox *comboBox;
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
    QWidget *formLayoutWidget_2;
    QFormLayout *formLayout_2;
    QLabel *nomLabel_2;
    QLineEdit *Mnom;
    QLabel *prNom_2;
    QLineEdit *Mprenom;
    QLabel *dateDEmbaucheLabel_2;
    QLineEdit *Mnumero;
    QLabel *label_7;
    QLineEdit *Madresse;
    QLabel *label_8;
    QLineEdit *MnbArbre;
    QLabel *label_9;
    QLineEdit *Mtypeolive;
    QLabel *label_10;
    QLineEdit *Mmail;
    QLabel *label_11;
    QLineEdit *Mregion;
    QHBoxLayout *qjouterEmpRow_2;
    QPushButton *retour;
    QSpacerItem *horizontalSpacer_ajouteremp_2;
    QPushButton *btn_valider_modif;
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
        MainWindow->resize(922, 760);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        metierspersonnel = new QStackedWidget(centralwidget);
        metierspersonnel->setObjectName("metierspersonnel");
        metierspersonnel->setGeometry(QRect(70, 20, 701, 641));
        ajoutpersonnel = new QWidget();
        ajoutpersonnel->setObjectName("ajoutpersonnel");
        formLayoutWidget = new QWidget(ajoutpersonnel);
        formLayoutWidget->setObjectName("formLayoutWidget");
        formLayoutWidget->setGeometry(QRect(70, 10, 571, 601));
        formLayout = new QFormLayout(formLayoutWidget);
        formLayout->setObjectName("formLayout");
        formLayout->setVerticalSpacing(43);
        formLayout->setContentsMargins(0, 0, 0, 0);
        nomLabel = new QLabel(formLayoutWidget);
        nomLabel->setObjectName("nomLabel");

        formLayout->setWidget(0, QFormLayout::LabelRole, nomLabel);

        nom = new QLineEdit(formLayoutWidget);
        nom->setObjectName("nom");

        formLayout->setWidget(0, QFormLayout::FieldRole, nom);

        prNom = new QLabel(formLayoutWidget);
        prNom->setObjectName("prNom");

        formLayout->setWidget(1, QFormLayout::LabelRole, prNom);

        Prenom = new QLineEdit(formLayoutWidget);
        Prenom->setObjectName("Prenom");

        formLayout->setWidget(1, QFormLayout::FieldRole, Prenom);

        dateDEmbaucheLabel = new QLabel(formLayoutWidget);
        dateDEmbaucheLabel->setObjectName("dateDEmbaucheLabel");

        formLayout->setWidget(2, QFormLayout::LabelRole, dateDEmbaucheLabel);

        Numero = new QLineEdit(formLayoutWidget);
        Numero->setObjectName("Numero");

        formLayout->setWidget(2, QFormLayout::FieldRole, Numero);

        label = new QLabel(formLayoutWidget);
        label->setObjectName("label");

        formLayout->setWidget(3, QFormLayout::LabelRole, label);

        adresse = new QLineEdit(formLayoutWidget);
        adresse->setObjectName("adresse");

        formLayout->setWidget(3, QFormLayout::FieldRole, adresse);

        label_4 = new QLabel(formLayoutWidget);
        label_4->setObjectName("label_4");

        formLayout->setWidget(4, QFormLayout::LabelRole, label_4);

        nbArbre = new QLineEdit(formLayoutWidget);
        nbArbre->setObjectName("nbArbre");

        formLayout->setWidget(4, QFormLayout::FieldRole, nbArbre);

        label_6 = new QLabel(formLayoutWidget);
        label_6->setObjectName("label_6");

        formLayout->setWidget(5, QFormLayout::LabelRole, label_6);

        typeolive = new QLineEdit(formLayoutWidget);
        typeolive->setObjectName("typeolive");

        formLayout->setWidget(5, QFormLayout::FieldRole, typeolive);

        label_3 = new QLabel(formLayoutWidget);
        label_3->setObjectName("label_3");

        formLayout->setWidget(6, QFormLayout::LabelRole, label_3);

        Mail = new QLineEdit(formLayoutWidget);
        Mail->setObjectName("Mail");

        formLayout->setWidget(6, QFormLayout::FieldRole, Mail);

        label_2 = new QLabel(formLayoutWidget);
        label_2->setObjectName("label_2");

        formLayout->setWidget(7, QFormLayout::LabelRole, label_2);

        Region = new QLineEdit(formLayoutWidget);
        Region->setObjectName("Region");

        formLayout->setWidget(7, QFormLayout::FieldRole, Region);

        qjouterEmpRow = new QHBoxLayout();
        qjouterEmpRow->setObjectName("qjouterEmpRow");
        quitter = new QPushButton(formLayoutWidget);
        quitter->setObjectName("quitter");

        qjouterEmpRow->addWidget(quitter);

        horizontalSpacer_ajouteremp = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        qjouterEmpRow->addItem(horizontalSpacer_ajouteremp);

        ajouterEmpBtn = new QPushButton(formLayoutWidget);
        ajouterEmpBtn->setObjectName("ajouterEmpBtn");
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/img/export.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        ajouterEmpBtn->setIcon(icon);
        ajouterEmpBtn->setIconSize(QSize(18, 18));

        qjouterEmpRow->addWidget(ajouterEmpBtn);


        formLayout->setLayout(8, QFormLayout::FieldRole, qjouterEmpRow);

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

        comboBox = new QComboBox(consulterpersonnel);
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->setObjectName("comboBox");

        horizontalLayout->addWidget(comboBox);


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
        formLayoutWidget_2 = new QWidget(modifier);
        formLayoutWidget_2->setObjectName("formLayoutWidget_2");
        formLayoutWidget_2->setGeometry(QRect(80, 20, 571, 601));
        formLayout_2 = new QFormLayout(formLayoutWidget_2);
        formLayout_2->setObjectName("formLayout_2");
        formLayout_2->setVerticalSpacing(43);
        formLayout_2->setContentsMargins(0, 0, 0, 0);
        nomLabel_2 = new QLabel(formLayoutWidget_2);
        nomLabel_2->setObjectName("nomLabel_2");

        formLayout_2->setWidget(0, QFormLayout::LabelRole, nomLabel_2);

        Mnom = new QLineEdit(formLayoutWidget_2);
        Mnom->setObjectName("Mnom");

        formLayout_2->setWidget(0, QFormLayout::FieldRole, Mnom);

        prNom_2 = new QLabel(formLayoutWidget_2);
        prNom_2->setObjectName("prNom_2");

        formLayout_2->setWidget(1, QFormLayout::LabelRole, prNom_2);

        Mprenom = new QLineEdit(formLayoutWidget_2);
        Mprenom->setObjectName("Mprenom");

        formLayout_2->setWidget(1, QFormLayout::FieldRole, Mprenom);

        dateDEmbaucheLabel_2 = new QLabel(formLayoutWidget_2);
        dateDEmbaucheLabel_2->setObjectName("dateDEmbaucheLabel_2");

        formLayout_2->setWidget(2, QFormLayout::LabelRole, dateDEmbaucheLabel_2);

        Mnumero = new QLineEdit(formLayoutWidget_2);
        Mnumero->setObjectName("Mnumero");

        formLayout_2->setWidget(2, QFormLayout::FieldRole, Mnumero);

        label_7 = new QLabel(formLayoutWidget_2);
        label_7->setObjectName("label_7");

        formLayout_2->setWidget(3, QFormLayout::LabelRole, label_7);

        Madresse = new QLineEdit(formLayoutWidget_2);
        Madresse->setObjectName("Madresse");

        formLayout_2->setWidget(3, QFormLayout::FieldRole, Madresse);

        label_8 = new QLabel(formLayoutWidget_2);
        label_8->setObjectName("label_8");

        formLayout_2->setWidget(4, QFormLayout::LabelRole, label_8);

        MnbArbre = new QLineEdit(formLayoutWidget_2);
        MnbArbre->setObjectName("MnbArbre");

        formLayout_2->setWidget(4, QFormLayout::FieldRole, MnbArbre);

        label_9 = new QLabel(formLayoutWidget_2);
        label_9->setObjectName("label_9");

        formLayout_2->setWidget(5, QFormLayout::LabelRole, label_9);

        Mtypeolive = new QLineEdit(formLayoutWidget_2);
        Mtypeolive->setObjectName("Mtypeolive");

        formLayout_2->setWidget(5, QFormLayout::FieldRole, Mtypeolive);

        label_10 = new QLabel(formLayoutWidget_2);
        label_10->setObjectName("label_10");

        formLayout_2->setWidget(6, QFormLayout::LabelRole, label_10);

        Mmail = new QLineEdit(formLayoutWidget_2);
        Mmail->setObjectName("Mmail");

        formLayout_2->setWidget(6, QFormLayout::FieldRole, Mmail);

        label_11 = new QLabel(formLayoutWidget_2);
        label_11->setObjectName("label_11");

        formLayout_2->setWidget(7, QFormLayout::LabelRole, label_11);

        Mregion = new QLineEdit(formLayoutWidget_2);
        Mregion->setObjectName("Mregion");

        formLayout_2->setWidget(7, QFormLayout::FieldRole, Mregion);

        qjouterEmpRow_2 = new QHBoxLayout();
        qjouterEmpRow_2->setObjectName("qjouterEmpRow_2");
        retour = new QPushButton(formLayoutWidget_2);
        retour->setObjectName("retour");

        qjouterEmpRow_2->addWidget(retour);

        horizontalSpacer_ajouteremp_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        qjouterEmpRow_2->addItem(horizontalSpacer_ajouteremp_2);

        btn_valider_modif = new QPushButton(formLayoutWidget_2);
        btn_valider_modif->setObjectName("btn_valider_modif");
        btn_valider_modif->setIcon(icon);
        btn_valider_modif->setIconSize(QSize(18, 18));

        qjouterEmpRow_2->addWidget(btn_valider_modif);


        formLayout_2->setLayout(8, QFormLayout::FieldRole, qjouterEmpRow_2);

        metierspersonnel->addWidget(modifier);
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
        menubar->setGeometry(QRect(0, 0, 922, 26));
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
        nomLabel->setText(QCoreApplication::translate("MainWindow", "Nom", nullptr));
        nom->setPlaceholderText(QCoreApplication::translate("MainWindow", "Nom", nullptr));
        prNom->setText(QCoreApplication::translate("MainWindow", "Pr\303\251nom", nullptr));
        Prenom->setText(QString());
        Prenom->setPlaceholderText(QCoreApplication::translate("MainWindow", "Pr\303\251nom", nullptr));
        dateDEmbaucheLabel->setText(QCoreApplication::translate("MainWindow", "Numero", nullptr));
        Numero->setPlaceholderText(QCoreApplication::translate("MainWindow", "Numero", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Adresse", nullptr));
        adresse->setText(QString());
        adresse->setPlaceholderText(QCoreApplication::translate("MainWindow", "Adresse", nullptr));
        label_4->setText(QCoreApplication::translate("MainWindow", "Nombre d'arbre ", nullptr));
        nbArbre->setPlaceholderText(QCoreApplication::translate("MainWindow", "Nombre d'arbre", nullptr));
        label_6->setText(QCoreApplication::translate("MainWindow", "Type d'huile", nullptr));
        typeolive->setPlaceholderText(QCoreApplication::translate("MainWindow", "Type d'huile", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "Mail", nullptr));
        Mail->setPlaceholderText(QCoreApplication::translate("MainWindow", "Mail", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "Region ", nullptr));
        Region->setPlaceholderText(QCoreApplication::translate("MainWindow", "Region", nullptr));
        quitter->setText(QCoreApplication::translate("MainWindow", "Quiter", nullptr));
        ajouterEmpBtn->setText(QCoreApplication::translate("MainWindow", "Ajouter", nullptr));
        comboBox->setItemText(0, QCoreApplication::translate("MainWindow", "Region", nullptr));
        comboBox->setItemText(1, QCoreApplication::translate("MainWindow", "Rendement Moyen", nullptr));
        comboBox->setItemText(2, QCoreApplication::translate("MainWindow", "Quantit\303\251 apport\303\251", nullptr));

        comboBox->setPlaceholderText(QCoreApplication::translate("MainWindow", "Trier", nullptr));
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
        prNom_2->setText(QCoreApplication::translate("MainWindow", "Pr\303\251nom", nullptr));
        Mprenom->setText(QString());
        Mprenom->setPlaceholderText(QCoreApplication::translate("MainWindow", "Pr\303\251nom", nullptr));
        dateDEmbaucheLabel_2->setText(QCoreApplication::translate("MainWindow", "Numero", nullptr));
        Mnumero->setPlaceholderText(QCoreApplication::translate("MainWindow", "Numero", nullptr));
        label_7->setText(QCoreApplication::translate("MainWindow", "Adresse", nullptr));
        Madresse->setText(QString());
        Madresse->setPlaceholderText(QCoreApplication::translate("MainWindow", "Adresse", nullptr));
        label_8->setText(QCoreApplication::translate("MainWindow", "Nombre d'arbre ", nullptr));
        MnbArbre->setPlaceholderText(QCoreApplication::translate("MainWindow", "Nombre d'arbre", nullptr));
        label_9->setText(QCoreApplication::translate("MainWindow", "Type d'huile", nullptr));
        Mtypeolive->setPlaceholderText(QCoreApplication::translate("MainWindow", "Type d'huile", nullptr));
        label_10->setText(QCoreApplication::translate("MainWindow", "Mail", nullptr));
        Mmail->setPlaceholderText(QCoreApplication::translate("MainWindow", "Mail", nullptr));
        label_11->setText(QCoreApplication::translate("MainWindow", "Region ", nullptr));
        Mregion->setPlaceholderText(QCoreApplication::translate("MainWindow", "Region", nullptr));
        retour->setText(QCoreApplication::translate("MainWindow", "Retour", nullptr));
        btn_valider_modif->setText(QCoreApplication::translate("MainWindow", "Modifier", nullptr));
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
