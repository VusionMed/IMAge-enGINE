// Qt includes

// SlicerApp includes
#include "vusionAboutDialog.h"
#include "ui_vusionAboutDialog.h"

//-----------------------------------------------------------------------------
class vusionAboutDialogPrivate: public Ui_vusionAboutDialog
{
public:
};

//-----------------------------------------------------------------------------
// vusionAboutDialogPrivate methods


//-----------------------------------------------------------------------------
// vusionAboutDialog methods
vusionAboutDialog::vusionAboutDialog(QWidget* parentWidget)
 :QDialog(parentWidget)
  , d_ptr(new vusionAboutDialogPrivate)
{
  Q_D(vusionAboutDialog);
  d->setupUi(this);
  //d->CreditsTextBrowser->setFontPointSize(25);
  //d->CreditsTextBrowser->append("ImageEngine By VUSION TECH");
  //d->CreditsTextBrowser->setFontPointSize(11);
  //d->CreditsTextBrowser->append("");
  //d->CreditsTextBrowser->append("Ver. 0.0.1");
  //d->CreditsTextBrowser->append("");
  //d->CreditsTextBrowser->append("");
  //d->CreditsTextBrowser->insertHtml("<a href=\"http://www.vusion.com/\">¹«Ë¾ÍøÕ¾</a> <br />");
  //d->CreditsTextBrowser->append("");
  ////d->CreditsTextBrowser->append(QString::fromLocal"");
  //d->CreditsTextBrowser->insertHtml("");
  //d->CreditsTextBrowser->insertHtml("");
  ////d->SlicerLinksTable->setIndexWidget(QModelIndex(), new QTextBrowser);
  //d->SlicerLinksTextBrowser->insertHtml("");
  //d->CreditsTextBrowser->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);
  setWindowTitle(tr("License Information"));
  connect(d->ButtonBox, SIGNAL(rejected()), this, SLOT(close()));
}

//-----------------------------------------------------------------------------
vusionAboutDialog::~vusionAboutDialog()
{

}
