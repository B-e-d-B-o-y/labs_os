#ifndef CHARTWIDGET_QTCHARTS_H
#define CHARTWIDGET_QTCHARTS_H

#include <QWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QDateTimeAxis>
#include <QVector>
#include <QPair>

QT_CHARTS_USE_NAMESPACE

class ChartWidgetQtCharts : public QChartView {
    Q_OBJECT

public:
    explicit ChartWidgetQtCharts(QWidget *parent = nullptr);
    void updateChart(const QVector<QPair<qint64, double>> &data, const QString &title = "");
    
private:
    QChart *chart;
    QLineSeries *series;
    QDateTimeAxis *axisX;
    QValueAxis *axisY;
    QLineSeries *averageSeries;
    
    void setupChart();
};

#endif // CHARTWIDGET_QTCHARTS_H