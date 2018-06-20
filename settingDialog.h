#ifndef __settingDialog_h
#define __settingDialog_h

// Qt includes
#include <QDialog>

namespace Ui {
	class settingDialog;
}
/// Pre-request that a vusionlication has been instanced
class settingDialog : public QDialog
{
	Q_OBJECT
public:
	settingDialog(QWidget *parentWidget = 0);
	virtual ~settingDialog();

	private slots:
	void onAccept();

protected:


private:
	Ui::settingDialog *ui;
};

#endif