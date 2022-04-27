#include <QApplication>
#include "nrparamparser.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    NRParamParser pp = NRParamParser::instance();
        
    pp.acceptParam("a", "address", true);
    pp.acceptParam("b", "username", true);
    pp.acceptParam("c", "password", true);
    pp.acceptParam("i", "interval", true);
    pp.acceptParam("h", "interval", true);    
    pp.acceptParam("k", "interval", true);
    pp.acceptParam("l", "interval", true);
    pp.acceptParam("m", "interval", true);  
      
    bool b = pp.parse(argc,argv);

    if (!b) {
        exit(100);
    }
    
    QString ad = pp.paramValue("a").toString();
    QString us = pp.paramValue("u").toString();
    QString pt = pp.paramValue("p").toString();
    qulonglong interval = pp.paramValue("i").toULongLong();

    MainWindow w(ad, us, pt, interval);
    w.show();
    
    return a.exec();
}
