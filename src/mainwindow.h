#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include "mxzmq.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private:
  void saveSettings();
  void loadSettings();

private slots:
  void appendLogMessage(bool status);
  void connectButtonToggled(bool checked);
  void endpointLineEdited(const QString &text);
  void newMessageReceived();
  void closeEvent(QCloseEvent *event);

private:
  Ui::MainWindow *ui;
  MXZmq *_zmq;
  QSettings _settings = QSettings(APP_DOMAIN, APP_NAME);
};
#endif // MAINWINDOW_H
