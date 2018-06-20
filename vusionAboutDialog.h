
#ifndef __vusionAboutDialog_h
#define __vusionAboutDialog_h

// Qt includes
#include <QDialog>

// CTK includes
#include <ctkPimpl.h>

class vusionAboutDialogPrivate;

/// Pre-request that a vusionlication has been instanced
class vusionAboutDialog : public QDialog
{
  Q_OBJECT
public:
  vusionAboutDialog(QWidget *parentWidget = 0);
  virtual ~vusionAboutDialog();

protected:
  QScopedPointer<vusionAboutDialogPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(vusionAboutDialog);
  Q_DISABLE_COPY(vusionAboutDialog);
};

#endif
