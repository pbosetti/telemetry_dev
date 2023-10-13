#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "defines.h"
#include "qcustomplot.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  ui->endpointLine->setText(DEFAULT_ENDPOINT);
  ui->topicLine->setText(DEFAULT_TOPIC);

  _zmq = new MXZmq(this, ui->topicLine->text(), ui->endpointLine->text().toStdString());
  // if _zmq init fails, endpoint is reset to default, so we want to synchro back the UI
  ui->endpointLine->setText(_zmq->endpoint());

  connect(ui->connectButton, &QPushButton::toggled, this, [=](bool checked) {
    if (checked) {
      _zmq->connect();
      ui->connectButton->setText("Disconnect");
      ui->statusbar->showMessage("Connected", 5000);
    } else {
      _zmq->disconnect();
      ui->connectButton->setText("Connect");
      ui->statusbar->showMessage("Disconnected", 5000);
    }
  });

  double a = 10;

  connect(_zmq, &MXZmq::gotNewMessage, this, [=](){
    ui->logMessageArea->appendPlainText(_zmq->payload());
  });
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::appendLogMessage(bool status) {
  ui->logMessageArea->appendPlainText(status ? "Checked" : "Not checked");
}
