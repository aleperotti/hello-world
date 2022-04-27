#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>
#include "settingsDlg.h"
#include "UniqLogger.h"

class RestApiUtils;

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QString /*address*/, QString /*username*/,QString /*pwd*/, qulonglong interval =300, QWidget *parent = 0);
    ~MainWindow();

private:
    void createTrayIcon();
    void createActions();
    void updateGui();
    void initLogger();
    
private:
    Ui::MainWindow *ui;
    QNetworkAccessManager m_RestApiNetworkManager;
    QString m_adminPwd, m_username, m_kalliopeIpAddr, m_Bpath, m_tmpFileName, m_privacyPwd, msgToSend;
    QDate m_fromDate;
    qulonglong m_lastCallId, m_reorderCallId, m_newCallId, m_newCallIdentifier, m_oldCallId;
    qulonglong m_secsInterval, m_totLinesCount;
    QTimer *m_requestTimer;
    SettingsWdg *m_settingsDialog;
    QSystemTrayIcon *m_TrayIcon;
    QAction *m_QuitAction;
    QAction *m_settingsAction;
    Logger* kbc_logger;
    QMenu *m_menuTrayIcon;
    RestApiUtils *m_restApiUtils;
    bool m_excl_Local, m_excl_Incoming, m_excl_Outcoming, m_bool1;

protected:
    void changeEvent(QEvent *);
    void closeEvent(QCloseEvent *);
    void logMessage(QString /*logMsg*/, QString /*funcInfo*/, UNQL::LogMessagePriorityType /*verbosity*/);

private slots:
    void handleRestApiResponse(QNetworkReply* /*reply*/);
    void sendRestApiCommand();
    void openConfigurationPanel();
    void copyFile();
    void startRefreshTimer();
    void openContextMenu( const QPoint & );
    void trayIconActivated(QSystemTrayIcon::ActivationReason /*reason*/);
};

#endif // MAINWINDOW_H
