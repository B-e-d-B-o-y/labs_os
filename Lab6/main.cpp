#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Устанавливаем тему
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Настройки шрифта
    QFont defaultFont("Segoe UI", 10);
    app.setFont(defaultFont);
    
    // Создаем главное окно
    MainWindow window;
    window.setWindowTitle("Temperature Monitor - Lab6");
    window.resize(1200, 700);
    window.show();
    
    return app.exec();
}