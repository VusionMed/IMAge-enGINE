#pragma once

#ifndef vtkUDInteractorStyleImage_h
#define vtkUDInteractorStyleImage_h

#include <vtkInteractorStyleImage.h>
#include <vtkSmartPointer.h>
#include <vtkTextMapper.h>
#include <vtkVusionMultiView.h>
#include <vector>

class vtkImageData;
class vtkImageProperty;
class vtkImageActor;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkRenderer;
//class vtkVusionMultiView;
class vtkContourWidget;


#include <sstream>

class vtkUDInteractorStyleImage : public vtkInteractorStyleImage
{
public:
	static vtkUDInteractorStyleImage* New();
	vtkTypeMacro(vtkUDInteractorStyleImage, vtkInteractorStyleImage);

protected:
	vtkUDInteractorStyleImage();
	~vtkUDInteractorStyleImage() {}
	vtkSmartPointer<vtkVusionMultiView> _ImageViewer;

public:
	void SetImageViewer(vtkSmartPointer< vtkVusionMultiView> imageViewer);
	vtkImageActor* GetImageActor() {return this->_ImageViewer->GetImageActor(0);};
protected:


	virtual void WindowLevel();
	virtual void OnMouseWheelForward() {};
	virtual void OnMouseWheelBackward() {};
private:
	vtkUDInteractorStyleImage(const vtkUDInteractorStyleImage&);  // Not implemented.
	void operator=(const vtkUDInteractorStyleImage&);  // Not implemented.	
	
};

#endif