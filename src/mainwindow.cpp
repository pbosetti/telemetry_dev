#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "defines.h"
#include "qcustomplot.h"

/*
  _     _  __                      _      
 | |   (_)/ _| ___  ___ _   _  ___| | ___ 
 | |   | | |_ / _ \/ __| | | |/ __| |/ _ \
 | |___| |  _|  __/ (__| |_| | (__| |  __/
 |_____|_|_|  \___|\___|\__, |\___|_|\___|
                        |___/             
*/
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  loadSettings();

  _zmq = new MXZmq(this, ui->topicLine->text(),
                   ui->endpointLine->text().toStdString());
  // if _zmq init fails, endpoint is reset to default, so we want to synchro
  // back the UI
  ui->endpointLine->setText(_zmq->endpoint());

  connect(ui->connectButton, &QPushButton::toggled, this,
          &MainWindow::connectButtonToggled);

  connect(ui->endpointLine, &QLineEdit::textEdited, this,
          &MainWindow::endpointLineEdited);

  connect(_zmq, &MXZmq::gotNewMessage, this, &MainWindow::newMessageReceived);
}

MainWindow::~MainWindow() { 
  delete ui; 
}


/*
  __  __      _   _               _     
 |  \/  | ___| |_| |__   ___   __| |___ 
 | |\/| |/ _ \ __| '_ \ / _ \ / _` / __|
 | |  | |  __/ |_| | | | (_) | (_| \__ \
 |_|  |_|\___|\__|_| |_|\___/ \__,_|___/
                                        
*/

void MainWindow::saveSettings() {
  _settings.beginGroup("MainWindow");
  _settings.setValue("geometry", saveGeometry());
  _settings.setValue("endpoint", ui->endpointLine->text());
  _settings.setValue("topic", ui->topicLine->text());
  _settings.endGroup();
}

void MainWindow::loadSettings() {
  _settings.beginGroup("MainWindow");
  const auto topic = _settings.value("topic", DEFAULT_TOPIC).toString();
  ui->topicLine->setText(topic);

  const auto endpoint =
      _settings.value("endpoint", DEFAULT_ENDPOINT).toString();
  ui->endpointLine->setText(endpoint);

  const auto geometry = _settings.value("geometry", QByteArray()).toByteArray();
  if (geometry.isEmpty())
    setGeometry(800, 800, 600, 600);
  else
    restoreGeometry(geometry);
  _settings.endGroup();
}

/*
  ____  _       _       
 / ___|| | ___ | |_ ___ 
 \___ \| |/ _ \| __/ __|
  ___) | | (_) | |_\__ \
 |____/|_|\___/ \__|___/
                        
*/

void MainWindow::appendLogMessage(bool status) {
  ui->logMessageArea->appendPlainText(status ? "Checked" : "Not checked");
}

void MainWindow::connectButtonToggled(bool checked) {
  if (checked) {
    _zmq->connect();
    ui->connectButton->setText("Disconnect");
    ui->statusbar->showMessage("Connected", 5000);
    ui->endpointLine->setEnabled(false);
    ui->topicLine->setEnabled(false);
  } else {
    _zmq->disconnect();
    ui->connectButton->setText("Connect");
    ui->statusbar->showMessage("Disconnected", 5000);
    ui->endpointLine->setEnabled(true);
    ui->topicLine->setEnabled(true);
  }
}

void MainWindow::endpointLineEdited(const QString &text) {
  if (_zmq->setEndpoint(text)) {
    ui->connectButton->setEnabled(true);
    ui->endpointLine->setStyleSheet("QLineEdit { color: black; }");
  } else {
    ui->connectButton->setEnabled(false);
    ui->endpointLine->setStyleSheet("QLineEdit { color: red; }");
  }
}

void MainWindow::newMessageReceived() {
  ui->logMessageArea->appendPlainText(_zmq->payload());
}

void MainWindow::closeEvent(QCloseEvent *event) {
  qDebug() << "Saving settings to " << _settings.fileName();
  _zmq->disconnect();
  saveSettings();
}