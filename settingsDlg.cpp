#include <QPushButton>
#include <QButtonGroup>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QByteArray>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include "simpleobfuscator.h"
#include "settingsDlg.h"
#include "ui_settingsdlg.h"


namespace UTILS
{
    QString md5(QString plain)
    {
        return QString::fromUtf8(QCryptographicHash::hash(plain.toLatin1(),QCryptographicHash::Md5).toHex());
    }
}

SettingsWdg::SettingsWdg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDlg)
{
    ui->setupUi(this);

    m_kvers = 4;
    m_reorderCallId = 0;
    m_excl_Local = false;
    m_excl_Incoming = false;
    m_excl_Outcoming = false;

    m_lastCallId = 0;
    m_interval = 300;
    m_ktenantDomain = "default";

    //QRegExp rx("[0-9]{1,3}(.[0-9]{1,3}){3,3}"); //RegEp for IP address
    //QValidator *validator = new QRegExpValidator(rx, this);
    //ui->txtKAddress->setValidator(validator);

    //QRegExp numbers("^([5-9]|[1-8][0-9]|9[0-9]|[1-8][0-9]{2}|9[0-8][0-9]|99[0-9]|[1-4][0-9]{3}|5000)$");
    QValidator *numValidator = new QIntValidator(5, 5000, ui->txtInterval);
    ui->txtInterval->setValidator(numValidator);

    QValidator *lastCallIdValidator = new QRegExpValidator(QRegExp("\\d{14}"), ui->txtLastCallId);
    ui->txtLastCallId->setValidator(lastCallIdValidator);

    QValidator *reorderFromValidator = new QRegExpValidator(QRegExp("\\d{14}"), ui->txtReorderFrom);
    ui->txtReorderFrom->setValidator(reorderFromValidator);

    ui->cdrDateFrom->setDate(QDate(2018, 1, 1));

    connect(ui->pbSave, SIGNAL(clicked()), this, SLOT(changeSettings()));
    connect(ui->pbCancel, SIGNAL(clicked()), this, SLOT(onCancelRequested()));
    connect(ui->pbBrowse, SIGNAL(clicked()), this, SLOT(searchTxtDirs()));
    connect(ui->rbV3, SIGNAL(clicked()), this, SLOT(onKversChanged()));
    connect(ui->rbV4, SIGNAL(clicked()), this, SLOT(onKversChanged()));

    // update displayed fields
    onKversChanged();

    if (QFile::exists("kbc.ini")) {
    
        QSettings ksett("kbc.ini", QSettings::IniFormat);
        ksett.beginGroup("KBC");

        m_kvers         = ksett.value("Kvers", 3).toUInt();
        m_excl_Local       = ksett.value("exclLocal", 0).toUInt()?true:false;
        m_excl_Incoming      = ksett.value("exclIncoming", 0).toUInt()?true:false;
        m_excl_Outcoming       = ksett.value("exclOutcoming", 0).toUInt()?true:false;

        m_kaddress      = ksett.value("kaddress", "").toString();
        m_username      = ksett.value("username", "").toString();
        m_ktenantDomain = ksett.value("ktenantDomain", "default").toString();
        m_kpwdPrivacy   = SimpleObfuscator::reveal(ksett.value("pwdPrivacy", "").toString());
        m_kpwd          = SimpleObfuscator::reveal(ksett.value("pwd", "").toString());

        // interval is minimum 300
        m_interval      = ksett.value("updateInterval", 300).toULongLong()*60;
        m_interval      = qMax(m_interval, static_cast<qulonglong>(300));

        m_bpath         = ksett.value("bluesTxtPath", "").toString();

        m_fromDate      = QDate::fromString(ksett.value("fromDate", "").toString(), "yyyy-MM-dd");

        m_lastCallId    = ksett.value("lastCallId", 0).toULongLong();
        m_reorderCallId = ksett.value("reorderCallId", 0).toULongLong();

        ksett.endGroup();

        updateGui();
    }
}

SettingsWdg::~SettingsWdg()
{
    delete ui;
    ui = NULL;
}

void
SettingsWdg::changeSettings()
{
    if (ui->txtPath->text().isEmpty())
    {
        QMessageBox::warning(this, "Informazione mancante", "Il campo \"Path di salvataggio log\" \xc3\xa8 obbligatorio");
        return;
    }
    if (ui->txtUsername->text().isEmpty())
    {
        QMessageBox::warning(this, "Informazione mancante", "Il campo \"Username\" \xc3\xa8 obbligatorio");
        return;
    }
    if (ui->txtPwd->text().isEmpty())
    {
        QMessageBox::warning(this, "Informazione mancante", "Il campo \"Password\" \xc3\xa8 obbligatorio");
        return;
    }
    if (ui->txtKAddress->text().isEmpty())
    {
        QMessageBox::warning(this, "Informazione mancante", "L'indirizzo IP del KalliopePBX non \xc3\xa8 valido");
        return;
    }
    if (ui->chkReorderFrom->isChecked() && ui->txtReorderFrom->text().size() != 14)
    {
        QMessageBox::warning(this, "Informazione non corretta", "Il campo \"Riordina CallId da\" deve essere un CallId valido");
        return;
    }

    if (ui->chkExclLocal->isChecked() && ui->chkExclIncoming->isChecked() && ui->chkExclOutcoming->isChecked())
    {
        QMessageBox::warning(this, "Informazione non corretta", "Non possono essere escluse tutte le tipologie di chiamata!");
        return;
    }

    m_kaddress = ui->txtKAddress->text();

    if (ui->rbV3->isChecked())
    {
        m_kvers = 3;
    }
    else
    {
        m_kvers = 4;
    }

    m_excl_Local = ui->chkExclLocal->isChecked();
    m_excl_Incoming = ui->chkExclIncoming->isChecked();
    m_excl_Outcoming = ui->chkExclOutcoming->isChecked();

    m_ktenantDomain = ui->txtTenantDomain->text();

    m_username = ui->txtUsername->text();
    if (!ui->txtPwd->text().isEmpty())
    {
        m_kpwd = ui->txtPwd->text();
    }
    if (!ui->txtPwdPrivacy->text().isEmpty())
    {
        m_kpwdPrivacy = ui->txtPwdPrivacy->text();
    }

    qulonglong interval = ui->txtInterval->text().toULongLong()*60;
    if (interval < 300)
    {
        interval = 300;
        ui->txtInterval->setText(QString::number(interval/60));
    }
    m_interval = ui->txtInterval->text().toULongLong()*60;

    m_bpath = ui->txtPath->text();

    // reset lastCallId
    m_lastCallId = 0;

    if (ui->chkReorderFrom->isChecked())
    {
        m_reorderCallId = ui->txtReorderFrom->text().toULongLong();
    }
    else
    {
        m_reorderCallId = 0;
    }

    m_fromDate = ui->cdrDateFrom->date();

    persistSettings();
    updateGui();

    this->accept();
}

void
SettingsWdg::persistSettings()
{
    QSettings ksett("kbc.ini", QSettings::IniFormat);
    ksett.beginGroup("KBC");

    ksett.setValue("Kvers", m_kvers);

    ksett.setValue("exclLocal", m_excl_Local?1:0);
    ksett.setValue("exclIncoming", m_excl_Incoming?1:0);
    ksett.setValue("exclOutcoming", m_excl_Outcoming?1:0);

    ksett.setValue("username", m_username);
    ksett.setValue("pwd", SimpleObfuscator::hide(ui->txtPwd->text().toLatin1()));
    ksett.setValue("pwdPrivacy", SimpleObfuscator::hide(ui->txtPwdPrivacy->text().toLatin1()));
    ksett.setValue("kaddress", m_kaddress);
    ksett.setValue("ktenantDomain", ui->txtTenantDomain->text());

    ksett.setValue("bluesTxtPath", m_bpath);
    ksett.setValue("updateInterval", m_interval/60);
    ksett.setValue("fromDate", m_fromDate.toString("yyyy-MM-dd"));

    ksett.setValue("lastCallId", m_lastCallId);
    ksett.setValue("reorderCallId", m_reorderCallId);

    ksett.endGroup();
}

void
SettingsWdg::onCancelRequested()
{
    updateGui();
    this->reject();
}

void
SettingsWdg::onKversChanged()
{
    m_kvers = ui->rbV3->isChecked() ? 3 : 4;

    ui->lblTenantDomain->setVisible(m_kvers == 4);
    ui->txtTenantDomain->setVisible(m_kvers == 4);
    ui->txtTenantDomain->setEnabled(m_kvers == 4);
    ui->chkExclIncoming->setEnabled(m_kvers == 4);
    ui->chkExclOutcoming->setEnabled(m_kvers == 4);
    ui->chkExclLocal->setEnabled(m_kvers == 4);

    if(m_kvers == 3)
    {
        ui->txtTenantDomain->setText("default");
        ui->chkExclLocal->setChecked(false);
        ui->chkExclOutcoming->setChecked(false);
        ui->chkExclIncoming->setChecked(false);
    }

    ui->lblPwdPrivacy->setVisible(m_kvers == 3);
    ui->txtPwdPrivacy->setVisible(m_kvers == 3);
    ui->txtPwdPrivacy->setEnabled(m_kvers == 3);
}

uint
SettingsWdg::getKvers() const
{
    return m_kvers;
}

bool
SettingsWdg::getLocalExcludeFlag() const
{
    return m_excl_Local;
}

bool
SettingsWdg::getIncomingExcludeFlag() const
{
    return m_excl_Incoming;
}

bool
SettingsWdg::getOutcomingExcludeFlag() const
{
    return m_excl_Outcoming;
}

QString
SettingsWdg::getKaddress() const
{
    return m_kaddress;
}

QString
SettingsWdg::getKtenantDomain() const
{
    return m_ktenantDomain;
}

QString
SettingsWdg::getKpwd() const
{
    return m_kpwd;
}

QString
SettingsWdg::getUsername() const
{
    return m_username;
}

QString
SettingsWdg::getKpwdPrivacy() const
{
    return m_kpwdPrivacy;
}

QString
SettingsWdg::getBluesTxtPath() const
{
    return m_bpath;
}

qulonglong
SettingsWdg::getInterval() const
{
    return m_interval;
}

qulonglong
SettingsWdg::getLastCallId() const
{
    return m_lastCallId;
}

qulonglong
SettingsWdg::getReorderCallId() const
{
    return m_reorderCallId;
}

QDate
SettingsWdg::getFromDate() const
{
    return m_fromDate;
}

void
SettingsWdg::persistLastUpdateInfo(qulonglong callId, qulonglong reorderFrom, QDate fromDate)
{
    m_lastCallId = callId;
    m_reorderCallId = reorderFrom;
    m_fromDate = fromDate;

    persistSettings();
}

void
SettingsWdg::searchTxtDirs()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"), QDir::homePath(), QFileDialog::ShowDirsOnly);
    if (!dir.isEmpty())
    {
        ui->txtPath->setText(dir);
    }
}

void
SettingsWdg::updateGui()
{
    if (m_kvers == 3)
    {
        ui->rbV3->setChecked(true);

        ui->lblTenantDomain->setVisible(false);
        ui->txtTenantDomain->setVisible(false);
        ui->txtTenantDomain->setEnabled(false);
        ui->chkExclLocal->setEnabled(false);
        ui->chkExclIncoming->setEnabled(false);
        ui->chkExclOutcoming->setEnabled(false);

        ui->lblPwdPrivacy->setVisible(true);
        ui->txtPwdPrivacy->setVisible(true);
        ui->txtPwdPrivacy->setEnabled(true);
    }
    else if (m_kvers == 4)
    {
        ui->rbV4->setChecked(true);
    }

    ui->chkExclLocal->setChecked(m_excl_Local);
    ui->chkExclIncoming->setChecked(m_excl_Incoming);
    ui->chkExclOutcoming->setChecked(m_excl_Outcoming);

    ui->txtKAddress->setText(m_kaddress);
    ui->txtUsername->setText(m_username);
    ui->txtTenantDomain->setText(m_ktenantDomain);
    ui->txtPwdPrivacy->setText(m_kpwdPrivacy);
    ui->txtPwd->setText(m_kpwd);

    ui->txtInterval->setText(QString::number(m_interval/60));
    ui->txtPath->setText(m_bpath);

    ui->txtLastCallId->setText(QString::number(m_lastCallId));
    ui->cdrDateFrom->setDate(m_fromDate);

    ui->chkReorderFrom->setChecked(m_reorderCallId != 0);
    ui->txtReorderFrom->setText(QString::number(m_reorderCallId));
}
