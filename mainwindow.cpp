#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTextStream>
#include <QStringList>
#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QCursor>
#include <QMessageBox>
#include <QHostInfo>

#include "mainwindow.h"
#include "restapiutils.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QString address, QString username, QString pwd, qulonglong interval, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_requestTimer(NULL)
    , kbc_logger(NULL)
{
    m_lastCallId = 0;
    m_totLinesCount = 0;

    m_restApiUtils = new RestApiUtils();
    m_requestTimer = new QTimer(this);

    // store passed address, credentials and interval
    m_kalliopeIpAddr = address;
    m_username = username;
    m_adminPwd = pwd;
    m_secsInterval = interval;

    m_settingsDialog = new SettingsWdg(this);

    // use default values from settings whether address, credentials and/or interval was not passed
    if (m_kalliopeIpAddr.isEmpty())
    {
        m_kalliopeIpAddr = m_settingsDialog->getKaddress();
    }
    if (m_username.isEmpty())
    {
        m_username = m_settingsDialog->getUsername();
    }
    if (m_adminPwd.isEmpty())
    {
        m_adminPwd = m_settingsDialog->getKpwd();
    }
    m_privacyPwd = m_settingsDialog->getKpwdPrivacy();

    if (m_secsInterval == 0)
    {
        m_secsInterval = m_settingsDialog->getInterval();
    }

    m_Bpath = m_settingsDialog->getBluesTxtPath();

    m_fromDate = m_settingsDialog->getFromDate();
    m_lastCallId = m_settingsDialog->getLastCallId();
    m_reorderCallId = m_settingsDialog->getReorderCallId();
    m_excl_Local= m_settingsDialog->getLocalExcludeFlag();
    m_excl_Incoming= m_settingsDialog->getIncomingExcludeFlag();
    m_excl_Outcoming= m_settingsDialog->getOutcomingExcludeFlag();

    m_tmpFileName = QDir::tempPath() + "/bluesCdr.tmp";

    initLogger();

    // setup UI
    ui->setupUi(this);
    ui->lblStatus->setText(tr("Non connesso"));
    createActions();
    createTrayIcon();
    this->setContextMenuPolicy( Qt::CustomContextMenu );
    updateGui();
    m_TrayIcon->show();

    bool b = true;
    b &= (bool) connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(openContextMenu(const QPoint &)));
    b &= (bool) connect(&m_RestApiNetworkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(handleRestApiResponse(QNetworkReply*)), Qt::QueuedConnection);
    b &= (bool) connect(m_requestTimer, SIGNAL(timeout()), this, SLOT(sendRestApiCommand()));
    b &= (bool) connect(m_TrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    Q_ASSERT(b);

    if (m_kalliopeIpAddr.isEmpty() || m_username.isEmpty() || m_adminPwd.isEmpty() || m_secsInterval < 1 || m_Bpath.isEmpty())
    {
        openConfigurationPanel();
    }
    else
    {
        QTimer::singleShot(500, this, SLOT(showMinimized()));
        sendRestApiCommand();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
    ui = NULL;
}

void 
MainWindow::logMessage(QString logMsg, QString funcInfo, UNQL::LogMessagePriorityType verbosity)
{
    if (kbc_logger)
    {
        *kbc_logger << verbosity << funcInfo << logMsg << UNQL::EOM;
    }
}

void
MainWindow::sendRestApiCommand()
{
    if (!m_requestTimer->isActive())
    {
        startRefreshTimer();
    }

    if (m_settingsDialog->getKvers() == 4)
    {
        QHostAddress kIpAddr(m_kalliopeIpAddr);
        if (kIpAddr.isNull())
        {
            QHostInfo info = QHostInfo::fromName(m_kalliopeIpAddr);
            if (info.error() == QHostInfo::NoError)
            {
                kIpAddr = info.addresses().at(0);
            }
            else
            {
                //TODO - log some error...
            }
        }

        QString restUrl = QString("http://%1/rest/cdr/blues").arg(m_kalliopeIpAddr);

        qDebug() << "Requesting cdr: " << restUrl << endl;

        QNetworkRequest request;
        request.setRawHeader("Content-Type", "application/xml");
        request.setUrl(QUrl(restUrl));

        m_restApiUtils->addAuthHeaderToRequest(&request, m_settingsDialog->getKtenantDomain(), kIpAddr, m_username, m_adminPwd);

        msgToSend= "<?xml version=\"1.0\"?><kpbx_request><cdr>";

        if (m_fromDate.isValid())
        {
            msgToSend += "<begin>" + m_fromDate.toString("yyyy-MM-dd") + " 00:00:00</begin>";
        }

        if (m_excl_Local)
        {
            msgToSend += "<bluesExcludeLocal>true</bluesExcludeLocal>";
        }
        else
        {
            msgToSend += "<bluesExcludeLocal>false</bluesExcludeLocal>";
        }

        if (m_excl_Incoming)
        {
            msgToSend += "<bluesExcludeIn>true</bluesExcludeIn>";
        }
        else
        {
            msgToSend += "<bluesExcludeIn>false</bluesExcludeIn>";
        }

        if (m_excl_Outcoming)
        {
            msgToSend += "<bluesExcludeOut>true</bluesExcludeOut>";
        }
        else
        {
            msgToSend += "<bluesExcludeOut>false</bluesExcludeOut>";
        }

        msgToSend += "</cdr></kpbx_request>";

        m_RestApiNetworkManager.post(request, msgToSend.toLatin1());

        logMessage(QString("Sending to %1 V4 RestApi Url: %2 ExclInternal: %3 ExclIncoming: %4 ExclOutcoming: %5").arg(m_kalliopeIpAddr).arg(restUrl).arg(m_excl_Local).arg(m_excl_Incoming).arg(m_excl_Outcoming), Q_FUNC_INFO, UNQL::LOG_INFO);
    }
    else if (m_settingsDialog->getKvers() == 3)
    {
        msgToSend= "<?xml version=\"1.0\"?><kpbx_request><cdr>";
        if (!m_privacyPwd.isEmpty())
        {
            msgToSend += "<privacy_password>" + UTILS::md5(m_privacyPwd) + "</privacy_password>";
        }
        if (m_fromDate.isValid())
        {
            msgToSend += "<begin>" + m_fromDate.toString("yyyy-MM-dd") + " 00:00:00</begin>";
        }
        msgToSend += "</cdr></kpbx_request>";

        QString restUrl = QString("http://%1/rest/cdr/blues/out").arg(m_kalliopeIpAddr);
        qDebug() << "Requesting cdr: " << restUrl << endl;

        QNetworkRequest request;
        request.setRawHeader("Authorization", "Basic " + QString("%1:%2").arg("admin").arg(UTILS::md5(m_adminPwd)).toLatin1().toBase64());
        request.setRawHeader("Content-Type", "application/xml");
        request.setUrl(QUrl(restUrl));

        m_RestApiNetworkManager.post(request, msgToSend.toLatin1());
        logMessage(QString("Sending to %1 V3 RestApi Url: %2").arg(m_kalliopeIpAddr).arg(restUrl), Q_FUNC_INFO, UNQL::LOG_INFO);
    }
}

void
MainWindow::handleRestApiResponse(QNetworkReply* reply)
{
    qulonglong callId = 0;
    qulonglong addedLines = 0;

    if (reply->error() != QNetworkReply::NoError)
    {
        logMessage(QString("REST API FAILED: %1").arg(reply->errorString()), Q_FUNC_INFO, UNQL::LOG_CRITICAL);
        m_TrayIcon->setToolTip(tr("Ultimo agg. (%1) NON riuscito").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
        ui->lblStatus->setText(tr("Ultimo agg. (%1) NON riuscito:\n%2 (errore %3)").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")).arg(reply->errorString()).arg(QString::number(reply->error())));
        return;
    }

    logMessage("API response is correct", Q_FUNC_INFO, UNQL::LOG_INFO);

    QFile outputFile(m_tmpFileName);
    if (!outputFile.open((QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)))
    {
        logMessage(QString("Opening bluesCdr.tmp in write mode FAILED: %1").arg(outputFile.errorString()), Q_FUNC_INFO, UNQL::LOG_CRITICAL);
        m_TrayIcon->setToolTip(tr("Ultimo agg. (%1) NON riuscito").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
        ui->lblStatus->setText(tr("Ultimo agg. (%1) NON riuscito:\n%2 (errore %3)").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")).arg(outputFile.errorString()).arg(QString::number(outputFile.error())));
        return;
    }

    logMessage("bluesCdr.tmp successfully opened in write mode.", Q_FUNC_INFO, UNQL::LOG_DBG);
    QTextStream out(&outputFile);

    QTextStream in(reply);
    while (!in.atEnd())
    {
        QString line = in.readLine();
        if (!line.startsWith("KPBX "))
        {
            logMessage("Got unexpected API response, i.e. not starting with KPBX prefix!", Q_FUNC_INFO, UNQL::LOG_WARNING);
            continue;
        }

        QStringList lineFields = line.simplified().split(" ");
        Q_ASSERT(lineFields.count() > 4);
        if (lineFields.count() <= 4)
        {
            logMessage(QString("Got invalid line, it has %1 fields!").arg(lineFields.count()), Q_FUNC_INFO, UNQL::LOG_WARNING);
            continue;
        }

        callId = lineFields.at(1).toULongLong();
        QString lastDateString = lineFields.at(3);
        QStringList tmpLineData = lastDateString.split("/");
        QDate lastDate = QDate(tmpLineData.last().toInt(), tmpLineData.at(1).toInt(), tmpLineData.first().toInt()).addDays(-1);

        if (lastDate.year() > m_fromDate.year() ||
                (lastDate.year() == m_fromDate.year() && lastDate.month() > m_fromDate.month()))
        {
            /* Month changed, reset reorderCallId.
             *
             * No need to reset also m_lastCallId since the new callId is always
             * greater than m_lastCallId (it's first 6 chars are yyyyMM).
             */
            logMessage(QString("Month changed from %1 to %2, resetting m_reorderCallId.").arg(m_fromDate.toString("yyyy-MM")).arg(lastDate.toString("yyyy-MM")), Q_FUNC_INFO, UNQL::LOG_WARNING);
            Q_ASSERT(callId > m_lastCallId);
            m_reorderCallId = 0;
        }

        if (callId <= m_lastCallId)
        {
            logMessage(QString("Skipping call having callId less then last... callId: %1 - lastCallId: %2").arg(callId).arg(m_lastCallId), Q_FUNC_INFO, UNQL::LOG_DBG);
            continue;
        }

        // check for reordering enabled
        if (m_reorderCallId == 0)
        {
            // do not reorder
            out << line << "\n";
        }
        else
        {
            // reorder callIds
            out << line.replace(QString(" %1 ").arg(callId), QString(" %1 ").arg(++m_reorderCallId)) << "\n";
        }

        m_lastCallId = callId;
        m_fromDate = lastDate;
        m_totLinesCount++;
        updateGui();

        addedLines++;
    }

    outputFile.close();
    logMessage("BluesCdr.tmp closed.", Q_FUNC_INFO, UNQL::LOG_DBG);

    if (addedLines > 0)
    {
        logMessage(QString("%1 lines successfully written. Let's copy the temporary file in the targer folder.").arg(QString::number(addedLines)), Q_FUNC_INFO, UNQL::LOG_INFO);
        copyFile();

        // update lastCallId and fromDate
        m_settingsDialog->persistLastUpdateInfo(callId, m_reorderCallId, m_fromDate);
    }
    else
    {
        logMessage("No new lines to write.", Q_FUNC_INFO, UNQL::LOG_INFO);
    }

    m_TrayIcon->setToolTip(tr("Ultimo agg. (%1) riuscito").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
    ui->lblStatus->setText(tr("Ultimo agg. (%1) riuscito: %2 linee importate").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")).arg(QString::number(addedLines)));
}

void MainWindow::createTrayIcon()
{
    m_menuTrayIcon = new QMenu(this);
    m_menuTrayIcon->addAction(m_settingsAction);
    m_menuTrayIcon->addSeparator();
    m_menuTrayIcon->addAction(m_QuitAction);
    m_TrayIcon = new QSystemTrayIcon(this);
    m_TrayIcon->setIcon(QIcon(":/images/money.png"));
    m_TrayIcon->setContextMenu(m_menuTrayIcon);
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
        this->showNormal();
        this->activateWindow();
        break;
    case QSystemTrayIcon::MiddleClick:
    default:
        ;
    }
}

void MainWindow::createActions()
{
    m_settingsAction = new QAction(tr("&Impostazioni"),this);
    m_QuitAction = new QAction(tr("&Esci"), this);
    connect(m_settingsAction, SIGNAL(triggered()), this, SLOT(openConfigurationPanel()));
    connect(m_QuitAction, SIGNAL(triggered()), this, SLOT(close()));
}

void MainWindow::changeEvent(QEvent *_event)
{
    if (_event->type() == QEvent::WindowStateChange)
    {
        if (isMinimized() == true)
        {
#ifdef WIN32
            QTimer::singleShot(100, this, SLOT(hide()));
#endif
        }
        else //if (isMaximized())
        {

#ifdef WIN32
            QTimer::singleShot(100, this, SLOT(show()));
#endif
        }
    }
    else
    {
        QMainWindow::changeEvent(_event);
    }
}

void
MainWindow::openConfigurationPanel()
{
    m_settingsDialog->updateGui();
    if (m_settingsDialog->exec())
    {
        m_requestTimer->stop();
        logMessage("Refresh timer stopped", Q_FUNC_INFO, UNQL::LOG_INFO);

        // get new settings
        m_kalliopeIpAddr = m_settingsDialog->getKaddress();
        m_username = m_settingsDialog->getUsername();
        m_adminPwd = m_settingsDialog->getKpwd();
        m_secsInterval = m_settingsDialog->getInterval();
        m_Bpath = m_settingsDialog->getBluesTxtPath();
        m_fromDate = m_settingsDialog->getFromDate();
        m_lastCallId = m_settingsDialog->getLastCallId();
        m_reorderCallId = m_settingsDialog->getReorderCallId();
        m_excl_Local = m_settingsDialog->getLocalExcludeFlag();
        m_excl_Incoming = m_settingsDialog->getIncomingExcludeFlag();
        m_excl_Outcoming = m_settingsDialog->getOutcomingExcludeFlag();

        updateGui();

        logMessage("Settings updated --------", Q_FUNC_INFO, UNQL::LOG_INFO);
        logMessage("-> KalliopePBX IP:" + m_kalliopeIpAddr, Q_FUNC_INFO, UNQL::LOG_INFO);
        logMessage("-> KalliopePBX Username: " + m_username, Q_FUNC_INFO, UNQL::LOG_INFO);
        logMessage("-> Refresh rate (secs): " + QString::number(m_secsInterval), Q_FUNC_INFO, UNQL::LOG_INFO);
        logMessage("-> Blues file path: " + m_Bpath, Q_FUNC_INFO, UNQL::LOG_INFO);
        logMessage("-> From date: " + m_fromDate.toString("yyyy-MM-dd"), Q_FUNC_INFO, UNQL::LOG_INFO);
        logMessage("-> Last CallID: " + QString::number(m_lastCallId), Q_FUNC_INFO, UNQL::LOG_INFO);
        logMessage("-> Reorder CallID from: " + m_reorderCallId == 0 ? "NO" : QString::number(m_reorderCallId), Q_FUNC_INFO, UNQL::LOG_INFO);
        logMessage("-> ExclInternal: " + QString::number(m_excl_Local), Q_FUNC_INFO, UNQL::LOG_INFO);
        logMessage("-> ExclIncoming: " + QString::number(m_excl_Incoming), Q_FUNC_INFO, UNQL::LOG_INFO);
        logMessage("-> ExclOutcoming: " + QString::number(m_excl_Outcoming), Q_FUNC_INFO, UNQL::LOG_INFO);

        sendRestApiCommand();
    }
}

void
MainWindow::copyFile()
{
    QFile destFile(m_Bpath + "/bluesCdr.txt");
    QFile sourceFile(m_tmpFileName);
    if (destFile.open(QIODevice::WriteOnly | QIODevice::Append) && sourceFile.open(QIODevice::ReadOnly))
    {
        QByteArray sourceContents = sourceFile.readAll();
        sourceFile.close();
        destFile.write(sourceContents);
        destFile.close();
        if (QFile::remove(m_tmpFileName))
        {            
            logMessage("File bluesCdr.tmp succesfully removed", Q_FUNC_INFO, UNQL::LOG_DBG);
        }
        else
        {
            logMessage("Deleting bluesCdr.tmp FAILED", Q_FUNC_INFO, UNQL::LOG_WARNING);
        }
    }
    else
    {
        int retryTimeoutSecs = 5;
        logMessage(QString("ERROR in copying bluesCdr.tmp in %1/bluesCdr.txt. Trying again in %1 secs...").arg(m_Bpath).arg(retryTimeoutSecs), Q_FUNC_INFO, UNQL::LOG_WARNING);
        QTimer::singleShot(retryTimeoutSecs * 1000, this, SLOT(copyFile()));
    }
}

void
MainWindow::updateGui()
{
    ui->lbl_interval->setText(QString::number(m_secsInterval/60));
    ui->lbl_Kaddress->setText(m_kalliopeIpAddr);
    ui->lbl_LinesCount->setText(QString::number(m_totLinesCount));
    ui->lblLastCallID->setText(QString::number(m_lastCallId));
    ui->lblReorderCallIDFrom->setText(m_reorderCallId == 0 ? "non attivo" : QString("attivo: %1").arg(m_reorderCallId+1));
}

void MainWindow::initLogger()
{
    if (kbc_logger)
    {
        // already initialized
        return;
    }

    UniqLogger *ul = UniqLogger::instance();
    ul->setEncasingChars('[',']');
    ul->setSpacingChar(' ');

    WriterConfig wconf;
    wconf.maxMessageNum = 10000;
    wconf.writerFlushSecs = 1;
    wconf.maxFileSize = 5;
    wconf.maxFileNum = 2;

    kbc_logger = ul->createFileLogger("KBC", "kbc_log.txt", wconf);
    kbc_logger->setVerbosityAcceptedLevel(UNQL::LOG_DBG);
    kbc_logger->setTimeStampFormat("yyyy-MM-dd hh:mm:ss.zzz");

    logMessage("Logger Started", Q_FUNC_INFO, UNQL::LOG_INFO);
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    if (QMessageBox::Yes == QMessageBox::question(this, tr("Conferma la chiusura dell'applicazione"), tr("Si \xc3\xa8 sicuri di chiudere l'applicazione?"), QMessageBox::Yes|QMessageBox::No))
    {
        e->accept();
    }
    else
    {
        e->ignore();
    }
}

void MainWindow::startRefreshTimer()
{
    m_requestTimer->start(m_secsInterval*1000);
    logMessage(QString("Refresh timer started, it will tick every %1 secs").arg(m_secsInterval), Q_FUNC_INFO, UNQL::LOG_INFO);
}

void MainWindow::openContextMenu(const QPoint &)
{
    m_menuTrayIcon->exec(QCursor::pos());
}
