#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QDate>
#include <QDialog>
#include <QSettings>

namespace Ui {
class SettingsDlg;
}

namespace UTILS
{
    QString md5(QString plain);
}

class SettingsWdg : public QDialog
{
    Q_OBJECT

private:
    QString m_kaddress, m_username, m_ktenantDomain, m_kpwd, m_kpwdPrivacy;
    QString m_bpath;
    QDate m_fromDate;
    qulonglong m_lastCallId, m_reorderCallId;
    qulonglong m_interval;
    uint m_kvers;
    bool m_excl_Local, m_excl_Incoming, m_excl_Outcoming;

    void createTrayIcon();
    void createActions();

    void persistSettings();

public:
    explicit SettingsWdg(QWidget *parent = 0);
    ~SettingsWdg();

    uint getKvers() const;
    bool getLocalExcludeFlag() const;
    bool getIncomingExcludeFlag() const;
    bool getOutcomingExcludeFlag() const;

    QString getKaddress() const;
    QString getUsername() const;
    QString getKtenantDomain() const;
    QString getKpwd() const;
    QString getKpwdPrivacy() const;

    QString getBluesTxtPath() const;
    qulonglong getInterval() const;
    qulonglong getLastCallId() const;
    qulonglong getReorderCallId() const;
    QDate getFromDate() const;

    QStringList req_type={"Outgoing","External","All"};

    void persistLastUpdateInfo(qulonglong callId, qulonglong reorderFrom, QDate fromDate);
    void updateGui();

private:
    Ui::SettingsDlg *ui;
    
private slots:
    void changeSettings();
    void onCancelRequested();
    void searchTxtDirs();
    void onKversChanged();
};
#endif // SETTINGS_H
