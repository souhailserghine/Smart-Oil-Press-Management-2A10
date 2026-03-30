#include "personnel.h"
#include "ui_personnel.h"
    
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QChart>
#include <QtCharts/QChartGlobal>
#include <QVBoxLayout>



personnel::personnel(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::personnel)
{
    ui->setupUi(this);
    /* ajouteé*/

    QPieSeries *series = new QPieSeries();
    series->append("Actifs", 42);
    series->append("En congé", 8);
    series->append("Suspendus", 3);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Répartition des employés");
    chart->legend()->setAlignment(Qt::AlignRight);
    
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    
    // Create layout if it doesn't exist
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(ui->chartStatusContainer->layout());
    if (!layout) {
        layout = new QVBoxLayout(ui->chartStatusContainer);
        layout->setContentsMargins(0, 0, 0, 0);
    }
    
    layout->addWidget(chartView);

    /*fina joute */
}

personnel::~personnel()
{
    delete ui;
}
// Qt 6: Charts classes are accessible without a QtCharts namespace when linked
