#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "vplaymanager.h"
#include <QStandardItemModel>

namespace Ui {
class MainWindow;
}

class ItemModelForCommands : public QStandardItemModel
{
    Q_OBJECT

    enum Roles {
        commandRole = Qt::UserRole + 1,
        channelRole,
        filenameRole,
        timeInRole,
        durationRole,
        loopRole
    };

public:
    ItemModelForCommands(QObject *parent = 0);
    virtual ~ItemModelForCommands() {}
    void addReturnCommand(const QString &channel);
    void addPlayCommand(const QString &channel, const QString &filename, const QTime &timeIn, const QTime &duration, const bool &loop);
    void addCueCommand(const QString &channel, const QString &filename);
    void setServerId(const QString &serverId);
    void setServerPort(const int &serverPort);
    void setServerAddress(const QString &serverAddress);
    void deleteCommand(const QModelIndex &index);
    void sendCommands();
    QString getServerId() const {return m_serverId;}
    QByteArray getFullPacket();
    QByteArray getNcsHeader();
    QByteArray getXmlPart();

signals:
    void log(QString text);
    void sendingIsOver();

private:
    void setItemsData(QStandardItem * const item1, QStandardItem * const item2, QVariant data, const int role = Qt::UserRole + 1);
    void prepareSendCommands();


private:
    QString m_serverId;
    VPlayManager m_vplayManager;
    bool m_isChanged;
    QByteArray m_packetHash;

};

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void onSendCommandsButtonPushed();
    void onAddCommandButtonPushed();
    void onDeleteCommandButtonPushed();
    void addPlayCommnad();
    void addReturnCommand();
    void addCueCommand();
    void onCurrentRowChanged(const QModelIndex &current, const QModelIndex &previous);
    void onTextChanged(const QString &text);
    void onLogRecivied(const QString &text);
    
private:
    void updateInfoWidget();

private:
    Ui::MainWindow *ui;
    VPlayManager vm;
    ItemModelForCommands *m_modelForCommands;

};

#endif // MAINWINDOW_H
