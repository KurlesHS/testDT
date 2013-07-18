#ifndef ADDCOMMANDDIALOG_H
#define ADDCOMMANDDIALOG_H

#include <QDialog>

namespace Ui {
class AddCommandDialog;
}

class AddCommandDialog : public QDialog
{
    Q_OBJECT

public:
    enum Commands {
        playCommand,
        cueCommand,
        returnCommand
    };
    
public:
    explicit AddCommandDialog(const Commands &cmd, QWidget *parent = 0);
    ~AddCommandDialog();
    QString getChannel() const;
    QString getFilename() const;
    bool isLoop() const;
    QTime getTimeCodeId() const;
    QTime getDuration() const;

private slots:
    void onSelectFileButtonPushed();
    
private:
    Ui::AddCommandDialog *ui;
    Commands m_currentCommand;
};

#endif // ADDCOMMANDDIALOG_H
