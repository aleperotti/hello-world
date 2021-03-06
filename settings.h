#ifndef SETTINGS_H
#define SETTINGS_H
#include <QWidget>
#include <QSettings>

namespace Ui {
class SettingsDlg;
}
class SettingsWdg : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWdg(QWidget *parent = 0);
    ~SettingsWdg();
    void createTrayIcon();
    void createActions();
	const int int_var = 0;

private:
    Ui::SettingsDlg *ui;
#endif // SETTINGS_H
