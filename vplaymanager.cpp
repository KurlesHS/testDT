#include "vplaymanager.h"
#include <QDomDocument>
#include <QEventLoop>
#include <QTcpSocket>
#include <QVariant>
#include <QHostAddress>
#include <QTimer>
#include <QDebug>
#include <QFile>
#include <QAction>


VPlayManager::VPlayManager(QObject *parent) :
    QObject(parent),

    m_currentServerId(SL_COMMAND_PORT),
    m_serverName("localhost")

{
    connect(&m_socket, SIGNAL(connected()), this, SLOT(onSocketConnected()));
    connect(&m_socket, SIGNAL(disconnected()), this, SLOT(onSocketDisconnected()));
    connect(&m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onSocketError(QAbstractSocket::SocketError)));
    connect(&m_socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onStateChanged(QAbstractSocket::SocketState)));
    connect(&m_socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));

    connect(&m_timerForTimeouts, SIGNAL(timeout()), this, SLOT(onTimeout()));

}

void VPlayManager::setServerName(const QString &serverName)
{
    m_serverName = serverName;
}

void VPlayManager::setServerPort(const int &serverPort)
{
    m_serverPort = serverPort;
}

void VPlayManager::beginGenerateQuery(const QString &serverId)
{
    m_currentServerId = serverId;
    m_listOfCommands.clear();
}

void VPlayManager::endGenerateQuery()
{

}

void VPlayManager::addPlayAction(const QString &channel, const QString &filename, const QTime &timeIn, const QTime &duration, bool loop)
{
    command cmd;
    cmd.cmd = c_play;
    cmd.channel = channel;
    cmd.filename = filename;
    cmd.duration = duration;
    cmd.timeIn = timeIn;
    cmd.loop = loop;
    m_listOfCommands.append(cmd);
}

void VPlayManager::addReturnToPlaylistAction(const QString &channel)
{
    command cmd;
    cmd.cmd = c_return;
    cmd.channel = channel;
    m_listOfCommands.append(cmd);
}

void VPlayManager::addCueAction(const QString &channel, const QString &filename)
{
    command cmd;
    cmd.cmd = c_cue;
    cmd.channel = channel;
    cmd.filename = filename;
    m_listOfCommands.append(cmd);
}

void VPlayManager::sendCommandsToServer(const QString &uuid)
{
    m_dataToSend[uuid] = getPacket();
    initiateSendCommands();
}

void VPlayManager::clearSendCommandQueue()
{
    m_dataToSend.clear();
}

QByteArray VPlayManager::getPacket()
{
    /*
    Клиент подсоединяется к порту 5500
    Отправляет заголовок команды в котором указаны признаки команды и количество байт которое нужно прочитать (длина xml в байтах).
    Затем  отправляется содержимое xml.
    */

    /*
    <?xml version="1.0" encoding="utf-8" ?>
    <VPlayChannel ServerID="62661105-f3ce-42af-a30c-a4149dcafb63">
        <CmdList>
            <item Channel = "0" Cmd= "Play"   File="iron_man-tlr1_h480p.mov" TimeCodeIn="00:00:00:000" Duration="00:02:29:205" Loop= "0" />
            <item Channel = "1" Cmd= "Return"/>
            <item Channel = "2" Cmd= "Play" File="iron_man-tlr1_h480p.mov" TimeCodeIn="00:00:00:000" Duration="00:02:29:205" Loop= "0" />
        </CmdList>
    </VPlayChannel>
    */
    NCS_HEADER hdr;
    memset(&hdr, 0, sizeof(NCS_HEADER));
    hdr.ID = SL_ID;
    hdr.CMD = SL_EXT_CMD;
    QDomDocument xmlOut;
    QDomElement root = xmlOut.createElement("VPlayChannel");
    root.setAttribute("ServerID", m_currentServerId);
    xmlOut.appendChild(root);
    QDomElement cmdListElement = xmlOut.createElement("CmdList");
    root.appendChild(cmdListElement);
    foreach (const command &cmd, m_listOfCommands) {
        switch (cmd.cmd) {
        case c_play:
        {
            QDomElement itemElement = xmlOut.createElement("item");
            itemElement.setAttribute("Channel", cmd.channel);
            itemElement.setAttribute("Cmd", "Play");
            itemElement.setAttribute("File", cmd.filename);
            if (!cmd.timeIn.isNull())
                itemElement.setAttribute("TimeCodeIn", cmd.timeIn.toString("HH:mm:ss:000"));
            if (!cmd.duration.isNull()){
                itemElement.setAttribute("Duration", cmd.duration.toString("HH:mm:ss:000"));
                itemElement.setAttribute("TimeCodeOut", cmd.duration.toString("HH:mm:ss:000"));
            }
            itemElement.setAttribute("Loop", cmd.loop ? "1" : "0");
            cmdListElement.appendChild(itemElement);

        }
            break;
        case c_return:
        {
            QDomElement itemElement = xmlOut.createElement("item");
            itemElement.setAttribute("Channel", cmd.channel);
            itemElement.setAttribute("Cmd", "Return");
            cmdListElement.appendChild(itemElement);
        }
            break;
        case c_cue:
        {
            QDomElement itemElement = xmlOut.createElement("item");
            itemElement.setAttribute("Channel", cmd.channel);
            itemElement.setAttribute("Cmd", "Cue");
            itemElement.setAttribute("File", cmd.filename);
            cmdListElement.appendChild(itemElement);
        }
            break;
        default:
            break;
        }
    }
    QString xmlString = QString("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
    xmlString.append(xmlOut.toString());

    QByteArray retValue;
    QByteArray xmlArray = QByteArray(reinterpret_cast<const char*>(xmlString.utf16()), xmlString.length() * 2);
    hdr.iData[0] = xmlArray.length();
    retValue.append(reinterpret_cast<char*>(&hdr), sizeof(NCS_HEADER));
    retValue.append(xmlArray);
    /*
    QFile f("d:/packet.bin");
    QFile f1("d:/packet.xml");
    QFile f2("d:/packet_header.bin");
    f.open(QIODevice::WriteOnly);
    f1.open(QIODevice::WriteOnly);
    f2.open(QIODevice::WriteOnly);
    f.write(retValue);
    f1.write(xmlArray);
    f2.write(reinterpret_cast<char*>(&hdr), sizeof(NCS_HEADER));
    */
    return retValue;
}


void VPlayManager::initiateSendCommands()
{
    // сокет занят - выход, уже посылаем
    if (m_socket.state() != QAbstractSocket::UnconnectedState)
        return;
    // нет данных к отправке - выход.
    if (m_dataToSend.isEmpty())
        return;
    m_currentPacketUuid = m_dataToSend.keys().at(0);
    emit log("trying to connect to the server");
    m_socket.connectToHost(m_serverName, m_serverPort);
    m_currentPacket = m_dataToSend.value(m_currentPacketUuid);
    m_dataToSend.remove(m_currentPacketUuid);
}

void VPlayManager::onSocketConnected()
{
    emit log(trUtf8("connection to the server established, prepare to send packet"));
    QEventLoop lp;
    QTimer t;
    connect(&t, SIGNAL(timeout()), &lp, SLOT(quit()));
    t.start(300);
    //lp.exec();
    emit log(trUtf8("packet prepared, sending..."));

    //m_socket.write(m_currentPacket.left(sizeof(NCS_HEADER)));
    //m_socket.write(m_currentPacket.right(m_currentPacket.length() - sizeof(NCS_HEADER)));
    qDebug() << m_socket.write(m_currentPacket);
    //TODO: wait reply from host
    m_timerForTimeouts.setSingleShot(true);

    // ждем 10 секунд ответа
    m_timerForTimeouts.start(10000);
    emit log(trUtf8("waiting for server response"));
}

void VPlayManager::onSocketDisconnected()
{
    emit log(trUtf8("disconected from the server"));
    initiateSendCommands();
}

void VPlayManager::onSocketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    emit log(trUtf8("socket error"));
    emit sendingIsOver(m_currentPacketUuid, false);
}

void VPlayManager::onStateChanged(QAbstractSocket::SocketState socketState)
{
    Q_UNUSED(socketState)
    //qDebug() << "state changed" << (int)socketState;
}

void VPlayManager::onReadyRead()
{
    m_timerForTimeouts.stop();
    QByteArray data = m_socket.readAll();

    emit log(trUtf8("response from the server received"));
    emit sendingIsOver(m_currentPacketUuid, true);
    m_socket.disconnectFromHost();

    qDebug() << "Response:" << data.length() << QString::fromUtf16(reinterpret_cast<const ushort*>(data.data() + 32));
}

void VPlayManager::onTimeout()
{
    m_timerForTimeouts.stop();
    emit log(trUtf8("Timeout while waiting for a response from the server"));
    m_socket.abort();
    emit sendingIsOver(m_currentPacketUuid, false);
    initiateSendCommands();
}
