#include "addcommanddialog.h"
#include "ui_addcommanddialog.h"

#include <QFileDialog>

AddCommandDialog::AddCommandDialog(const Commands &cmd, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddCommandDialog)
{
    ui->setupUi(this);
    setWindowTitle(trUtf8(" "));
    m_currentCommand = cmd;
    switch (cmd) {
    case playCommand:
        ui->stackedWidget->setCurrentWidget(ui->playPage);
        break;
    case cueCommand:
        ui->stackedWidget->setCurrentWidget(ui->cuePage);
        break;
    case returnCommand:
        ui->stackedWidget->setCurrentWidget(ui->returnPage);
        break;
    default:
        setEnabled(false);
        break;
    }

    connect(ui->toolButtonSelectFileCue, SIGNAL(clicked()), this, SLOT(onSelectFileButtonPushed()));
    connect(ui->toolButtonSelectFilePlay, SIGNAL(clicked()), this, SLOT(onSelectFileButtonPushed()));
    connect(ui->pushButtonAccept, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ui->pushButtonReject, SIGNAL(clicked()), this, SLOT(reject()));
}

AddCommandDialog::~AddCommandDialog()
{
    delete ui;
}

QString AddCommandDialog::getChannel() const
{
    QString retVal;
    switch (m_currentCommand) {
    case playCommand:
        retVal = ui->lineEditChannelPlay->text();
        break;
    case cueCommand:
        retVal = ui->lineEditChannelCue->text();
        break;
    case returnCommand:
        retVal = ui->lineEditChannelReturn->text();
        break;
    default:
        break;
    }
    return retVal;
}

QString AddCommandDialog::getFilename() const
{
    QString retVal;
    switch (m_currentCommand) {
    case playCommand:
        retVal =  ui->lineEditFilePlay->text();
        break;
    case cueCommand:
        retVal = ui->lineEditFileCue->text();
        break;
    default:
        break;
    }
    return retVal;
}

bool AddCommandDialog::isLoop() const
{
    return ui->radioButtonLoopTrue->isChecked();
}

QTime AddCommandDialog::getTimeCodeId() const
{
    if (ui->groupBoxTimeIn->isChecked())
        return ui->timeEditTimeIn->time();
    else
        return QTime();
}

QTime AddCommandDialog::getDuration() const
{
    if (ui->groupBoxDuration->isChecked())
        return ui->timeEditDuration->time();
    else
        return QTime();
}

void AddCommandDialog::onSelectFileButtonPushed()
{
    QString fileName = QFileDialog::getOpenFileName(this,
         tr("Select file"), "", tr("All Files (*.*)"));
    if (fileName.isEmpty())
        return;
    switch (m_currentCommand) {
    case playCommand:
        ui->lineEditFilePlay->setText(fileName);
        break;
    case cueCommand:
        ui->lineEditFileCue->setText(fileName);
        break;
    default:
        break;
    }
}
