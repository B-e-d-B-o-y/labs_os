#include "chartwidget.h"
#include <qwt_symbol.h>
#include <qwt_text.h>
#include <QDateTime>
#include <algorithm>
#include <cmath>

ChartWidget::ChartWidget(QWidget *parent) : QwtPlot(parent) {
    setupChart();
}

void ChartWidget::setupChart() {
    // Настраиваем фон
    setCanvasBackground(Qt::white);
    
    // Добавляем сетку
    grid = new QwtPlotGrid();
    grid->setPen(QPen(Qt::gray, 0, Qt::DotLine));
    grid->attach(this);
    
    // Настраиваем оси
    setAxisTitle(QwtPlot::xBottom, "Время");
    setAxisTitle(QwtPlot::yLeft, "Температура (°C)");
    
    // Используем шкалу времени для оси X
    QwtDateScaleDraw *scaleDraw = new QwtDateScaleDraw(Qt::LocalTime);
    scaleDraw->setDateFormat(QwtDate::Millisecond, "HH:mm:ss");
    scaleDraw->setDateFormat(QwtDate::Second, "HH:mm:ss");
    scaleDraw->setDateFormat(QwtDate::Minute, "HH:mm");
    scaleDraw->setDateFormat(QwtDate::Hour, "HH:mm");
    scaleDraw->setDateFormat(QwtDate::Day, "dd.MM");
    setAxisScaleDraw(QwtPlot::xBottom, scaleDraw);
    
    QwtDateScaleEngine *scaleEngine = new QwtDateScaleEngine(Qt::LocalTime);
    setAxisScaleEngine(QwtPlot::xBottom, scaleEngine);
    
    // Создаем кривую
    curve = new QwtPlotCurve("Температура");
    curve->setPen(QPen(Qt::blue, 2));
    curve->setRenderHint(QwtPlotItem::RenderAntialiased);
    
    // Добавляем символы для точек (только если точек мало)
    QwtSymbol *symbol = new QwtSymbol(QwtSymbol::Ellipse,
        QBrush(Qt::white), QPen(Qt::blue, 1), QSize(6, 6));
    curve->setSymbol(symbol);
    
    curve->attach(this);
    
    // Маркер для средней температуры
    averageMarker = new QwtPlotMarker();
    averageMarker->setLineStyle(QwtPlotMarker::HLine);
    averageMarker->setLinePen(QPen(Qt::red, 1, Qt::DashLine));
    averageMarker->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
    averageMarker->setLabelOrientation(Qt::Horizontal);
    
    // Легенда
    QwtLegend *legend = new QwtLegend();
    legend->setDefaultItemMode(QwtLegendData::Checkable);
    insertLegend(legend, QwtPlot::RightLegend);
    
    // Включаем масштабирование
    enableAxis(QwtPlot::xBottom, true);
    enableAxis(QwtPlot::yLeft, true);
}

void ChartWidget::updateChart(const QVector<QPair<qint64, double>> &data, const QString &title) {
    if (!title.isEmpty()) {
        QwtText plotTitle(title);
        plotTitle.setFont(QFont("Arial", 12, QFont::Bold));
        setTitle(plotTitle);
    }
    
    points.clear();
    double sum = 0;
    double minVal = std::numeric_limits<double>::max();
    double maxVal = std::numeric_limits<double>::lowest();
    
    // Преобразуем данные в QPointF
    for (const auto &pair : data) {
        double x = pair.first * 1000.0; // Конвертируем секунды в миллисекунды
        double y = pair.second;
        points.append(QPointF(x, y));
        
        sum += y;
        minVal = qMin(minVal, y);
        maxVal = qMax(maxVal, y);
    }
    
    if (points.isEmpty()) {
        curve->setSamples(QVector<QPointF>());
        averageMarker->detach();
        replot();
        return;
    }
    
    // Обновляем кривую
    curve->setSamples(points);
    
    // Рассчитываем и отображаем среднее значение
    double average = sum / points.size();
    averageMarker->setYValue(average);
    
    QwtText avgLabel(QString("Среднее: %1°C").arg(average, 0, 'f', 1));
    avgLabel.setColor(Qt::red);
    averageMarker->setLabel(avgLabel);
    averageMarker->attach(this);
    
    // Настраиваем масштаб осей
    double xMin = points.first().x();
    double xMax = points.last().x();
    
    // Добавляем небольшие отступы
    double xPadding = (xMax - xMin) * 0.05;
    setAxisScale(QwtPlot::xBottom, xMin - xPadding, xMax + xPadding);
    
    // Настраиваем ось Y с учетом min/max значений
    double yRange = maxVal - minVal;
    double yPadding = yRange * 0.1;
    if (yRange < 1.0) yPadding = 0.5; // Минимальный отступ
    
    setAxisScale(QwtPlot::yLeft, minVal - yPadding, maxVal + yPadding);
    
    // Если точек много, скрываем символы
    if (points.size() > 50) {
        curve->setSymbol(nullptr);
    } else {
        QwtSymbol *symbol = new QwtSymbol(QwtSymbol::Ellipse,
            QBrush(Qt::white), QPen(Qt::blue, 1), QSize(6, 6));
        curve->setSymbol(symbol);
    }
    
    replot();
}