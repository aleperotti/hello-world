#include <QButtonGroup>;
#include "settings.h"
#include "ui_settings.h"

SettingsWdg::SettingsWdg(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SettingsDlg)
{
    ui->setupUi(this);
    QRegExp rx("[0-9]{1,3}(.[0-9]{1,3}){3,3}"); //RegEp for IP address
    QValidator *validator = new QRegExpValidator(rx, this);
    ui->txtKAddress->setValidator(validator);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Salva"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Annulla"));
    QRegExp numbers("\d+");
    QValidator * numValidator = new QRegExpValidator(numbers, this);
    ui->txtInterval->setValidator(numValidator);
    connect(ui->buttonBox,SIGNAL(accepted()),this,SLOT(changeSettings()));
    connect(ui->buttonBox,SIGNAL(rejected()),this,SLOT(onCancelRequested()));

    if (!QFile::exists(m_kctiIniFileName)) {
        m_isFirstExecution = true;
        QMessageBox::information(this,"KalliopeCTI",tr("Sembra che sia la prima volta che si esegue KCTI. Prego inserire i dati di connessione al KalliopePBX."));
        this->show();
    }
    else {
        QSettings ksett(m_kctiIniFileName,QSettings::IniFormat);
        ksett.beginGroup("CTI");
        m_oldkaddr              = ksett.value("Kaddress").toString();
        m_oldkusr               = ksett.value("user").toString();
        m_oldkpwd               = ksett.value("password").toString();
}

SettingsWdg::~SettingsWdg()
{
    delete ui;
}

void
SettingsWdg::changeSettings()
{
    ;
}

void
SettingsWdg::onCancelRequested()
{
    this->reject();
}
