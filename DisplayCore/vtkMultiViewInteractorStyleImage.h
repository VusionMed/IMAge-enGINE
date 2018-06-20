#ifndef vtkMultiViewInteractorStyleImage_h
#define vtkMultiViewInteractorStyleImage_h

#include <vtkInteractorStyleImage.h>
#include <vtkSmartPointer.h>
#include "vtkVusionMultiView.h";//vtkSetObjectMacro needed header

class vtkMultiViewInteractorStyleImage : public vtkInteractorStyleImage
{
public:
	static vtkMultiViewInteractorStyleImage* New();
	vtkTypeMacro(vtkMultiViewInteractorStyleImage, vtkInteractorStyleImage);

	vtkSetObjectMacro(MultiViewer, vtkVusionMultiView);
	vtkGetObjectMacro(MultiViewer, vtkVusionMultiView);
	
protected:
	vtkMultiViewInteractorStyleImage();
	~vtkMultiViewInteractorStyleImage();

	virtual void OnMouseWheelForward();//Overwrite

	virtual void OnMouseWheelBackward();//Overwrite

	//virtual void WindowLevel();//Overwrite

	//virtual void OnChar();//Overwrite

protected:
	vtkVusionMultiView * MultiViewer;

private:
	vtkMultiViewInteractorStyleImage(const vtkMultiViewInteractorStyleImage&);  // Not implemented.
	void operator=(const vtkMultiViewInteractorStyleImage&);  // Not implemented.		
};
#endif
