/*-------------------------------------------------
#
# neural-style GUI with QT by AbsurdePhoton
#
# v1 2018-01-08
#
#-------------------------------------------------*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QFileDialog>
#include <QProcess>
#include <QTextEdit>
#include <QScrollBar>
#include <QTextCursor>
#include "QTextStream"
#include <QFileInfo>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    QProcess * process;
    bool running;

public slots:
    void Run();
    void Abort();
    void SelectImageSource();
    void SelectImageStyle();
    void SelectImageStyle2();
    void ChangeStyleBlendWeight();
    void ChangeBackend();
    void SelectNetwork();
    void ErrorOutput();
    void Started();
    void StandardOutput();
    void Finished();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
