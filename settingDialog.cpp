#include <settingDialog.h>
#include "ui_settingDialog.h"
#include <QSettings>
#include <qdebug.h>

settingDialog::settingDialog(QWidget* parentWidget)
	:QDialog(parentWidget)
{
	qDebug() << "[Setting] init page";
	this->ui = new Ui::settingDialog;
	this->ui->setupUi(this);

	setWindowTitle(tr("Setting"));

	QSettings setting("MR_Config.ini", QSettings::IniFormat);

	if (setting.value("Language") == "IMAGEN_zh_CN.qm")
	{
		this->ui->langCombo->setCurrentIndex(1);
	}
	else
	{
		this->ui->langCombo->setCurrentIndex(0);
	}

	if (setting.value("MASK_OPT"," ") == "")
	{
		this->ui->maskCombo->setCurrentIndex(1);
	}
	else
	{
		this->ui->maskCombo->setCurrentIndex(setting.value("MASK_OPT").toInt());
	}

	if (setting.value("IVIM_NUM_B", " ") == "")
	{
		this->ui->ivimB->setValue(4);
	}
	else
	{
		this->ui->ivimB->setValue(setting.value("IVIM_NUM_B").toInt());
	}

	if (setting.value("Dstar_BVAL_THRESH", " ") == "")
	{
		this->ui->dstarTh->setValue(200);
	}
	else
	{
		this->ui->dstarTh->setValue(setting.value("Dstar_BVAL_THRESH").toInt());
	}

	if (setting.value("DTI_NUM_GRAD_DIR", " ") == "")
	{
		this->ui->dtiMinDir->setValue(6);
	}
	else
	{
		this->ui->dtiMinDir->setValue(setting.value("DTI_NUM_GRAD_DIR").toInt());
	}

	if (setting.value("DKI_NUM_GRAD_DIR", " ") == "")
	{
		this->ui->dkiMinDir->setValue(10);
	}
	else
	{
		this->ui->dkiMinDir->setValue(setting.value("DKI_NUM_GRAD_DIR").toInt());
	}

	if (setting.value("NODDI_MED_B", " ") == "")
	{
		this->ui->NoddiMedThresh->setValue(1500);
	}
	else
	{
		this->ui->NoddiMedThresh->setValue(setting.value("NODDI_MED_B").toInt());
	}

	if (setting.value("SDKI_LOW_B_MIN", " ") == "")
	{
		this->ui->sdkiLowBu->setValue(500);
	}
	else
	{
		this->ui->sdkiLowBu->setValue(setting.value("SDKI_LOW_B_MIN").toInt());
	}

	if (setting.value("SDKI_LOW_B_MAX", " ") == "")
	{
		this->ui->sdkiLowBT->setValue(1000);
	}
	else
	{
		this->ui->sdkiLowBT->setValue(setting.value("SDKI_LOW_B_MAX").toInt());
	}

	if (setting.value("SDKI_HIGH_B_MIN", " ") == "")
	{
		this->ui->sdkiHighBu->setValue(2000);
	}
	else
	{
		this->ui->sdkiHighBu->setValue(setting.value("SDKI_HIGH_B_MIN").toInt());
	}

	if (setting.value("SDKI_HIGH_B_MAX", " ") == "")
	{
		this->ui->sdkiHighBt->setValue(3000);
	}
	else
	{
		this->ui->sdkiHighBt->setValue(setting.value("SDKI_HIGH_B_MAX").toInt());
	}

	connect(this->ui->buttonBox, SIGNAL(accepted()), this, SLOT(onAccept()));
}

//-----------------------------------------------------------------------------
settingDialog::~settingDialog()
{

}

void settingDialog::onAccept()
{
	this -> ui->infoLabel->setText(tr("Re-open the software and the setting will be valid. "));
	QSettings setting("MR_Config.ini", QSettings::IniFormat);

	if(this->ui->langCombo->currentIndex() == 0)
	{
		setting.setValue("Language", "IMAGEN_en_CN.qm");
	}
	else
	{
		setting.setValue("Language", "IMAGEN_zh_CN.qm");
	}
	setting.setValue("MASK_OPT", this->ui->maskCombo->currentIndex());

	setting.setValue("IVIM_NUM_B", this->ui->ivimB->value());
	setting.setValue("Dstar_BVAL_THRESH", this->ui->dstarTh->value());
	setting.setValue("DTI_NUM_GRAD_DIR", this->ui->dtiMinDir->value());
	setting.setValue("DKI_NUM_GRAD_DIR", this->ui->dkiMinDir->value());

	setting.setValue("NODDI_MED_B", this->ui->NoddiMedThresh->value());

	setting.setValue("SDKI_LOW_B_MIN", this->ui->sdkiLowBu->value());
	setting.setValue("SDKI_LOW_B_MAX", this->ui->sdkiLowBT->value());

	setting.setValue("SDKI_HIGH_B_MIN", this->ui->sdkiHighBu->value());
	setting.setValue("SDKI_HIGH_B_MAX", this->ui->sdkiHighBt->value());
}