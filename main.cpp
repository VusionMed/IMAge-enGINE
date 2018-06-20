#include <QApplication>
#include "mainwindow.h"
#include "qresource.h"
#include "qtextcodec.h"
#include <vtksys/SystemTools.hxx>
#include <windows.h>
#include <vtksys/Encoding.hxx>

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	QString localLoggingTxt;
	QString screenPrinttingTxt;
	switch (type) {
	case QtInfoMsg:
		screenPrinttingTxt = QString("[Info]: %1").arg(msg);
	case QtDebugMsg:
		screenPrinttingTxt = QString("[Debug]: %1").arg(msg);
		break;
	case QtWarningMsg:
		screenPrinttingTxt = QString("[Warning]: %1").arg(msg);
		break;
	case QtCriticalMsg:
		localLoggingTxt = QString("[Critical]: %1").arg(msg);
		break;
	case QtFatalMsg:
		localLoggingTxt = QString("[Fatal]: %1").arg(msg);
		break;
	}

	//QSettings settings;
	//if (settings.value("LogDirectory", "") == "")
	//{
	//	QString directory = QString("./");
	//	settings.setValue("LogDirectory", directory);
	//	settings.sync();
	//}
	//QString LogDirectory = settings.value("LogDirectory").toString();

	QFile outFile("log");
	outFile.open(QIODevice::WriteOnly | QIODevice::Append);
	QTextStream ts(&outFile);
	ts << localLoggingTxt << endl;

	fprintf(stderr, screenPrinttingTxt.toStdString().c_str());
}

int main(int argc, char* argv[])
{
  // QT Stuff
	// Q_INIT_RESOURCE(style);
	qInstallMessageHandler(myMessageOutput);
	QCoreApplication::setOrganizationName("Vusion Tech");
	QCoreApplication::setOrganizationDomain("www.vusion.com.cn");
	QCoreApplication::setApplicationName("ImageEngine-MR_Research");

    QApplication app( argc, argv );
	QTextCodec::setCodecForLocale(QTextCodec::codecForLocale());

	QSettings setting("MR_Config.ini", QSettings::IniFormat);
	QString trans = setting.value("Language").toString();

	qDebug() << "[Language] Loading QM: " << trans << endl;

	QTranslator m_translator;
	app.removeTranslator(&m_translator);
	bool suc = m_translator.load(trans);
	bool suc2 = app.installTranslator(&m_translator);

	//if (!suc || !suc2)
	//{
	//	QString MESSAGE;
	//	//QTextStream(&MESSAGE) << "Please use Shift+Left Click to select at least one window to draw ROI on.";		
	//	//QMessageBox msgBox(QMessageBox::Warning, tr("Please Select Window First"), MESSAGE, 0, this);
	//	//QTextStream(&MESSAGE) << tr("Cannot Load Language Pack");
	//	QMessageBox msgBox(QMessageBox::Warning, "Error", "Cannot Load Language Pack. Use English as Default. ", 0);
	//	msgBox.addButton("OK", QMessageBox::AcceptRole);
	//	msgBox.exec();
	//}

	qDebug() << "[Language] Loading QM successful??  " << suc << " Install OK?" << suc2 << endl;
    MainWindow myUI;
	myUI.show();

    return app.exec();
}

int __stdcall WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nShowCmd)
{
	// QT Stuff
	// Q_INIT_RESOURCE(style);

	qInstallMessageHandler(myMessageOutput);
	QCoreApplication::setOrganizationName("Vusion Tech");
	QCoreApplication::setOrganizationDomain("www.vusion.com.cn");
	QCoreApplication::setApplicationName("ImageEngine-MR_Research");

	int argc;
	LPWSTR* argvStringW = CommandLineToArgvW(GetCommandLineW(), &argc);

	std::vector< const char* > argv(argc); // usual const char** array used in main() functions
	std::vector< std::string > argvString(argc); // this stores the strings that the argv pointers point to
	for (int i = 0; i<argc; i++)
	{
		argvString[i] = vtksys::Encoding::ToNarrow(argvStringW[i]);
		argv[i] = argvString[i].c_str();
	}

	LocalFree(argvStringW);

	QApplication app(argc, const_cast< char** >(&argv[0]));

	QTextCodec::setCodecForLocale(QTextCodec::codecForLocale());

	QSettings setting("MR_Config.ini", QSettings::IniFormat);
	QString trans = setting.value("Language").toString();

	qDebug() << "[Language] Loading QM: " << trans << endl;

	QTranslator m_translator;
	app.removeTranslator(&m_translator);
	bool suc = m_translator.load(trans);
	bool suc2 = app.installTranslator(&m_translator);

	qDebug() << "[Language] Loading QM successful??  " << suc << " Install OK?" << suc2 << endl;

	MainWindow myUI;
	myUI.show();

	return app.exec();
}