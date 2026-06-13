#include "chartwidget_qtcharts.h"
#include <QtCharts/QChart>
#include <QtCharts/QLegend>
#include <QDateTime>
#include <algorithm>
#include <cmath>

ChartWidgetQtCharts::ChartWidgetQtCharts(QWidget *parent) : QChartView(parent) {
    setupChart();
}

void ChartWidgetQtCharts::setupChart() {
    chart = new QChart();
    chart->setBackgroundBrush(QBrush(Qt::white));
    chart->setMargins(QMargins(10, 10, 10, 10));
    
    // Создаем серию данных
    series = new QLineSeries();
    series->setName("Температура");
    series->setPen(QPen(Qt::blue, 2));
    
    // Серия для средней линии
    averageSeries = new QLineSeries();
    averageSeries->setName("Среднее");
    averageSeries->setPen(QPen(Qt::red, 1, Qt::DashLine));
    
    // Оси
    axisX = new QDateTimeAxis();
    axisX->setTitleText("Время");
    axisX->setFormat("dd.MM HH:mm");
    axisX->setTickCount(5);
    
    axisY = new QValueAxis();
    axisY->setTitleText("Температура (°C)");
    axisY->setLabelFormat("%.1f");
    
    chart->addSeries(series);
    chart->addSeries(averageSeries);
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    
    series->attachAxis(axisX);
    series->attachAxis(axisY);
    averageSeries->attachAxis(axisX);
    averageSeries->attachAxis(axisY);
    
    // Легенда
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    
    setChart(chart);
    setRenderHint(QPainter::Antialiasing);
}

void ChartWidgetQtCharts::updateChart(const QVector<QPair<qint64, double>> &data, const QString &title) {
    if (!title.isEmpty()) {
        chart->setTitle(title);
    }
    
    series->clear();
    averageSeries->clear();
    
    if (data.isEmpty()) {
        chart->setTitle("Нет данных");
        return;
    }
    
    double sum = 0;
    double minVal = std::numeric_limits<double>::max();
    double maxVal = std::numeric_limits<double>::lowest();
    qint64 minTime = std::numeric_limits<qint64>::max();
    qint64 maxTime = std::numeric_limits<qint64>::lowest();
    
    // Добавляем точки
    for (const auto &pair : data) {
        QDateTime dt = QDateTime::fromSecsSinceEpoch(pair.first);
        series->append(dt.toMSecsSinceEpoch(), pair.second);
        
        sum += pair.second;
        minVal = qMin(minVal, pair.second);
        maxVal = qMax(maxVal, pair.second);
        minTime = qMin(minTime, pair.first);
        maxTime = qMax(maxTime, pair.first);
    }
    
    // Рассчитываем среднее
    double average = sum / data.size();
    
    // Добавляем линию среднего
    QDateTime startDt = QDateTime::fromSecsSinceEpoch(minTime);
    QDateTime endDt = QDateTime::fromSecsSinceEpoch(maxTime);
    averageSeries->append(startDt.toMSecsSinceEpoch(), average);
    averageSeries->append(endDt.toMSecsSinceEpoch(), average);
    
    // Настраиваем масштаб
    axisX->setRange(startDt, endDt);
    
    double yRange = maxVal - minVal;
    double yPadding = yRange * 0.1;
    if (yRange < 1.0) yPadding = 0.5;
    
    axisY->setRange(minVal - yPadding, maxVal + yPadding);
    
    chart->update();
}