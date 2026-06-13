#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_legend.h>
#include <qwt_date_scale_draw.h>
#include <qwt_date_scale_engine.h>
#include <QVector>
#include <QPair>

class ChartWidget : public QwtPlot {
    Q_OBJECT

public:
    explicit ChartWidget(QWidget *parent = nullptr);
    void updateChart(const QVector<QPair<qint64, double>> &data, const QString &title = "");
    
private:
    QwtPlotCurve *curve;
    QwtPlotGrid *grid;
    QwtPlotMarker *averageMarker;
    QVector<QPointF> points;
    
    void setupChart();
};

#endif // CHARTWIDGET_H