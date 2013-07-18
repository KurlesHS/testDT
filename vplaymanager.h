#ifndef VPLAYMANAGER_H
#define VPLAYMANAGER_H

#include <QObject>
#include <QTime>
#include <QList>
#include <QHash>
#include <QMetaType>
#include <QAbstractSocket>
#include <QTcpSocket>
#include <QTimer>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#define SL_COMMAND_PORT 5500
#define SL_ID 0xF4806EC0
#define SL_EXT_CMD 0xECC50B4A


Q_DECLARE_METATYPE(QAbstractSocket::SocketError)
Q_DECLARE_METATYPE(QAbstractSocket::SocketState)

typedef struct _NCS_HEADER{
    __int64 iChannelID;             // Не используется
    unsigned __int64 iData[2]; // iData[0] количество байт которое нужно прочитать. iData[1] Не используется
#ifdef Q_OS_WIN
    DWORD   ID;                      // SL_ID
    DWORD   CMD;                     // SL_EXT_CMD
#else
    unsigned __int32   ID;                          // SL_ID
    unsigned __int32   CMD;                     // SL_EXT_CMD
#endif
}NCS_HEADER, *PNCS_HEADER;

class VPlayManager : public QObject
{
    Q_OBJECT


public:
    enum typeOfCommand {
        c_play,
        c_return,
        c_cue
    };

private:
    struct command {
        typeOfCommand cmd;
        QString channel;
        QString filename;
        QTime timeIn;
        QTime duration;
        bool loop;
    };


public:
    explicit VPlayManager(QObject *parent = 0);
    void setServerName(const QString &serverName);
    void setServerPort(const int &serverPort);
    void beginGenerateQuery(const QString &serverId = QString(""));
    void endGenerateQuery();
    void addPlayAction(const QString &channel, const QString &filename, const QTime &timeIn, const QTime &duration, bool loop = false);
    void addReturnToPlaylistAction(const QString &channel);
    void addCueAction(const QString &channel, const QString &filename);
    void sendCommandsToServer(const QString &uuid);
    void clearSendCommandQueue();


//private:
    QByteArray getPacket();

private:
    void initiateSendCommands();

private Q_SLOTS:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError socketError);
    void onStateChanged (QAbstractSocket::SocketState socketState);
    void onReadyRead();
    void onTimeout();

signals:
    void sendingIsOver(QString uuid, bool status);
    void log(QString text);
    
public slots:

private:
    QTcpSocket m_socket;
    QList<command> m_listOfCommands;
    QString m_currentServerId;
    QString m_serverName;
    QHash<QString, QByteArray> m_dataToSend;
    QString m_currentPacketUuid;
    QByteArray m_currentPacket;
    QTimer m_timerForTimeouts;
    int m_serverPort;

};

#endif // VPLAYMANAGER_H
