#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "addcommanddialog.h"

#include <QDebug>
#include <QUuid>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(trUtf8("test VPlay tcp client"));
    /*
    QString uuid = "74850096-7177-475C-9CA6-FAC3AA9B33B4";
    vm.beginGenerateQuery(uuid);
    vm.addPlayAction("1", "d:\\123.mpg", QTime(), QTime());
    vm.endGenerateQuery();
    vm.setServerName("127.0.0.1");
    vm.setServerPort(5500);
    vm.sendCommandsToServer("12345123");
*/
    //QByteArray packet = vm.getPacket();
    //qDebug() << packet.length() << packet.right(packet.length() - 32);
    m_modelForCommands = new ItemModelForCommands(this);
    ui->treeView->setWordWrap(true);
    ui->treeView->setModel(m_modelForCommands);
    ui->pushButtonRemove->setEnabled(false);
    connect(ui->pushButtonAdd, SIGNAL(clicked()), this, SLOT(onAddCommandButtonPushed()));
    connect(ui->pushButtonRemove, SIGNAL(clicked()), this, SLOT(onDeleteCommandButtonPushed()));
    connect(ui->pushButtonSend, SIGNAL(clicked()), this, SLOT(onSendCommandsButtonPushed()));

    connect(ui->treeView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(onCurrentRowChanged(QModelIndex,QModelIndex)));

    connect(ui->lineEditServer, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));
    connect(ui->lineEditServerId, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));

    connect(m_modelForCommands, SIGNAL(log(QString)), this, SLOT(onLogRecivied(QString)));
    updateInfoWidget();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onSendCommandsButtonPushed()
{
    m_modelForCommands->setServerAddress(ui->lineEditServer->text());
    m_modelForCommands->setServerId(ui->lineEditServerId->text());
    m_modelForCommands->setServerPort(ui->spinBoxPort->value());
    m_modelForCommands->sendCommands();
}

void MainWindow::onAddCommandButtonPushed()
{
    QMenu *menu = new QMenu();
    connect(menu, SIGNAL(aboutToHide()), menu, SLOT(deleteLater()));
    QAction *action = menu->addAction(trUtf8("Play command"));
    connect(action, SIGNAL(triggered()), this, SLOT(addPlayCommnad()));
    action = menu->addAction(trUtf8("Cue command"));
    connect(action, SIGNAL(triggered()), this, SLOT(addCueCommand()));
    action = menu->addAction(trUtf8("Return command"));
    connect(action, SIGNAL(triggered()), this, SLOT(addReturnCommand()));
    menu->popup(QCursor::pos());

}

void MainWindow::onDeleteCommandButtonPushed()
{
    m_modelForCommands->deleteCommand(ui->treeView->currentIndex());
    updateInfoWidget();
}

void MainWindow::addPlayCommnad()
{
    AddCommandDialog dlg(AddCommandDialog::playCommand, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_modelForCommands->addPlayCommand(
                    dlg.getChannel(),
                    dlg.getFilename(),
                    dlg.getTimeCodeId(),
                    dlg.getDuration(),
                    dlg.isLoop());
    }
    updateInfoWidget();
}

void MainWindow::addReturnCommand()
{
    AddCommandDialog dlg(AddCommandDialog::returnCommand, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_modelForCommands->addReturnCommand(
                    dlg.getChannel());
    }
    updateInfoWidget();
}

void MainWindow::addCueCommand()
{
    AddCommandDialog dlg(AddCommandDialog::cueCommand, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_modelForCommands->addCueCommand(
                    dlg.getChannel(),
                    dlg.getFilename());
    }
    updateInfoWidget();
}

void MainWindow::onCurrentRowChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous)
    ui->pushButtonRemove->setEnabled(current.isValid());
}

void MainWindow::onTextChanged(const QString &text)
{
    Q_UNUSED(text)
    updateInfoWidget();
}

void MainWindow::onLogRecivied(const QString &text)
{
    ui->plainTextEditLog->appendPlainText(trUtf8("-- %0 -  %1")
                                          .arg(QTime::currentTime().toString("hh:mm:ss"))
                                          .arg(text));
}

void MainWindow::updateInfoWidget()
{
    m_modelForCommands->setServerId(ui->lineEditServerId->text());
    QByteArray xmlPart = m_modelForCommands->getXmlPart();
    QByteArray ncsHeaderPart = m_modelForCommands->getNcsHeader();
    NCS_HEADER header;
    memcpy((void*)&header, (void*)ncsHeaderPart.data(), ncsHeaderPart.length());
    ui->plainTextEditInfo->clear();
    ui->plainTextEditInfo->appendPlainText(trUtf8("struct _NCS_HEADER {"));
    ui->plainTextEditInfo->appendPlainText(trUtf8("__int64 iChannelID; // %0 (0x%1)")
                                           .arg(header.iChannelID)
                                           .arg(header.iChannelID, 16, 16, QChar('0')));
    ui->plainTextEditInfo->appendPlainText(trUtf8("unsigned __int64 iData[0]; // %0 (0x%1)")
                                           .arg(header.iData[0])
            .arg(header.iData[0], 16, 16, QChar('0')));
    ui->plainTextEditInfo->appendPlainText(trUtf8("unsigned __int64 iData[1]; // %0 (0x%1)")
                                           .arg(header.iData[1])
            .arg(header.iData[1], 16, 16, QChar('0')));

    ui->plainTextEditInfo->appendPlainText(trUtf8("DWORD   ID; // %0 (0x%1)")
                                           .arg(header.ID)
            .arg(header.ID, 8, 16, QChar('0')));

    ui->plainTextEditInfo->appendPlainText(trUtf8("DWORD   ID; // %0 (0x%1)")
                                           .arg(header.CMD)
            .arg(header.CMD, 8, 16, QChar('0')));
    ui->plainTextEditInfo->appendPlainText(trUtf8("}\n\nXML часть:\n"));
    ui->plainTextEditInfo->appendPlainText(QString::fromUtf16(reinterpret_cast<const ushort*>(xmlPart.data()), xmlPart.length() / 2));

}


ItemModelForCommands::ItemModelForCommands(QObject *parent) :
    QStandardItemModel(parent),
    m_isChanged(true)
{
    setHorizontalHeaderLabels({trUtf8("Комманда"), trUtf8("Данные")});
    connect(&m_vplayManager, SIGNAL(log(QString)), this, SIGNAL(log(QString)));
}

void ItemModelForCommands::addReturnCommand(const QString &channel)
{
    QStandardItem *commandItem = new QStandardItem("return");
    QStandardItem *dataItem = new QStandardItem(trUtf8("Channel: %0").arg(channel));
    commandItem->setEditable(false);
    dataItem->setToolTip(dataItem->text());
    dataItem->setEditable(false);
    setItemsData(commandItem, dataItem, (int)VPlayManager::c_return, commandRole);
    setItemsData(commandItem, dataItem, channel, channelRole);
    appendRow({commandItem, dataItem});
    m_isChanged = true;
}

void ItemModelForCommands::addPlayCommand(const QString &channel, const QString &filename, const QTime &timeIn, const QTime &duration, const bool &loop)
{
    QStandardItem *commandItem = new QStandardItem("play");
    QStandardItem *dataItem = new QStandardItem();
    commandItem->setEditable(false);
    dataItem->setEditable(false);
    QString dataText = trUtf8("Channel: %0, file: %1").arg(channel).arg(filename);
    if (!timeIn.isNull()){
        dataText.append(trUtf8(", timeCodeIn: %0").arg(timeIn.toString("hh:mm:ss")));
    }
    if (!duration.isNull()) {
        dataText.append(trUtf8(", duration: %0").arg(timeIn.toString("hh:mm:ss")));
    }
    dataText.append(trUtf8(", loop: %0").arg(loop ? "true" : "false"));
    dataItem->setText(dataText);
    dataItem->setToolTip(dataItem->text());
    setItemsData(commandItem, dataItem, (int)VPlayManager::c_play, commandRole);
    setItemsData(commandItem, dataItem, channel, channelRole);
    setItemsData(commandItem, dataItem, timeIn, timeInRole);
    setItemsData(commandItem, dataItem, duration, durationRole);
    setItemsData(commandItem, dataItem, loop, loopRole);
    setItemsData(commandItem, dataItem, filename, filenameRole);
    appendRow({commandItem, dataItem});
    m_isChanged = true;
}

void ItemModelForCommands::addCueCommand(const QString &channel, const QString &filename)
{
    QStandardItem *commandItem = new QStandardItem("cue");
    QStandardItem *dataItem = new QStandardItem(trUtf8("Channel: %0, file: %1")
                                                .arg(channel)
                                                .arg(filename));
    dataItem->setToolTip(dataItem->text());
    commandItem->setEditable(false);
    dataItem->setEditable(false);
    setItemsData(commandItem, dataItem, (int)VPlayManager::c_cue, commandRole);
    setItemsData(commandItem, dataItem, channel, channelRole);
    setItemsData(commandItem, dataItem, filename, filenameRole);
    appendRow({commandItem, dataItem});
    m_isChanged = true;
}

void ItemModelForCommands::setServerId(const QString &serverId)
{
    m_isChanged = true;
    m_serverId = serverId;
}

void ItemModelForCommands::setServerPort(const int &serverPort)
{
    m_vplayManager.setServerPort(serverPort);
}

void ItemModelForCommands::setServerAddress(const QString &serverAddress)
{
    m_vplayManager.setServerName(serverAddress);
}

void ItemModelForCommands::deleteCommand(const QModelIndex &index)
{

    QStandardItem *si = itemFromIndex(index);
    if (si) {
        m_isChanged = true;
        removeRow(si->row());
    }
}

void ItemModelForCommands::sendCommands()
{

    prepareSendCommands();
    m_vplayManager.sendCommandsToServer(QUuid::createUuid());
}

QByteArray ItemModelForCommands::getFullPacket()
{
    if (m_isChanged){
        prepareSendCommands();
        m_packetHash = m_vplayManager.getPacket();
        m_isChanged = false;
    }
    return m_packetHash;
}

QByteArray ItemModelForCommands::getNcsHeader()
{
    return getFullPacket().left(sizeof(NCS_HEADER));
}

QByteArray ItemModelForCommands::getXmlPart()
{
    QByteArray packet = getFullPacket();
    return packet.right(packet.length() - sizeof(NCS_HEADER));
}

void ItemModelForCommands::setItemsData(QStandardItem * const item1, QStandardItem * const item2, QVariant value, const int role)
{
    item1->setData(value, role);
    item2->setData(value, role);
}

void ItemModelForCommands::prepareSendCommands()
{
    m_vplayManager.beginGenerateQuery(getServerId());
    for (int row = 0; row < rowCount(); ++row) {
        QStandardItem *si = item(row);
        if (si) {
            int command = si->data(commandRole).toInt();
            QString file = si->data(filenameRole).toString();
            bool loop = si->data(loopRole).toBool();
            QTime timeIn = si->data(timeInRole).toTime();
            QTime duration = si->data(durationRole).toTime();
            QString channel = si->data(channelRole).toString();
            switch (command) {
            case VPlayManager::c_play:
            {
                m_vplayManager.addPlayAction(channel, file, timeIn, duration, loop);
            }
                break;
            case VPlayManager::c_return:
            {
                m_vplayManager.addReturnToPlaylistAction(channel);
            }
                break;
            case VPlayManager::c_cue:
            {
                m_vplayManager.addCueAction(channel, file);
            }
                break;

            default:
                break;
            }
        }
    }
}
