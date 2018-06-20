#include "vtkMultiViewInteractorStyleImage.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkMultiViewInteractorStyleImage);

vtkMultiViewInteractorStyleImage::vtkMultiViewInteractorStyleImage()
{
	this->MultiViewer = NULL;
}

vtkMultiViewInteractorStyleImage::~vtkMultiViewInteractorStyleImage()
{

}

void vtkMultiViewInteractorStyleImage::OnMouseWheelForward() {


	
}

void vtkMultiViewInteractorStyleImage::OnMouseWheelBackward() {

}

//Get keybord input, only support Reset window level for now
//void vtkMultiViewInteractorStyleImage::OnChar()
//{

//}

//void vtkMultiViewInteractorStyleImage::WindowLevel()
//{
//	//vtkRenderWindowInteractor *rwi = this->Interactor;
//	//if (!rwi) return;
//
//	//this->WindowLevelCurrentPosition[0] = rwi->GetEventPosition()[0];
//	//this->WindowLevelCurrentPosition[1] = rwi->GetEventPosition()[1];
//
//	////if (this->HandleObservers &&
//	////	this->HasObserver(vtkCommand::WindowLevelEvent))
//	////{
//	////	this->InvokeEvent(vtkCommand::WindowLevelEvent, this);
//	////}
//	////else
//	//if (this->MultiViewer)
//	//{
//	//	int *size = this->CurrentRenderer->GetSize();
//	//	double window = this->MultiViewer->GetWindowLevel(0)->GetWindow();
//
//	//	//double window = this->WindowLevelInitial[0];//WindwoLevelInitial doesn't work as expected, replace it
//	//	double level = this->GetImageViewer2()->GetColorLevel();
//	//	// Compute normalized delta
//	//	double dx = (this->WindowLevelCurrentPosition[0] -
//	//		this->WindowLevelStartPosition[0]) * 0.0005;//jiangli modify 
//	//	double dy = (this->WindowLevelStartPosition[1] -
//	//		this->WindowLevelCurrentPosition[1]) * 0.0005; // / size[1];//jiangli modify
//
//	//	// Scale by current values
//	//	if (fabs(window) > 0.01)
//	//	{
//	//		dx = dx * window;
//	//	}
//	//	else
//	//	{
//	//		dx = dx * (window < 0 ? -0.01 : 0.01);
//	//	}
//	//	if (fabs(level) > 0.01)
//	//	{
//	//		dy = dy * level;
//	//	}
//	//	else
//	//	{
//	//		dy = dy * (level < 0 ? -0.01 : 0.01);
//	//	}
//
//	//	// Abs so that direction does not flip
//
//	//	if (window < 0.0)
//	//	{
//	//		dx = -1 * dx;
//	//	}
//	//	if (level < 0.0)
//	//	{
//	//		dy = -1 * dy;
//	//	}
//
//	//	// Compute new window level
//
//	//	//double newWindow = dx + window;
//	//	//double newLevel = level - dy;
//	//	double newWindowLevel[2]; 
//	//	newWindowLevel[0] = dx + window;
//	//	newWindowLevel[1] = level - dy;
//
//	//	if (newWindowLevel[0] < 0.01)
//	//	{
//	//		newWindowLevel[0] = 0.01;
//	//	}
//	//	newWindowLevel[1] = newWindowLevel[1] < 0.01 ? 0.01 : newWindowLevel[1];
//
//	//	//this->CurrentImageProperty->SetColorWindow(newWindowLevel[0]);
//	//	//this->CurrentImageProperty->SetColorLevel(newWindowLevel[1]);
//	//	this->GetImageViewer2()->SetColorWindow(newWindowLevel[0]);
//	//	this->GetImageViewer2()->SetColorLevel(newWindowLevel[1]);
//	//	this->Interactor->Render();
//
//	//	//vtkCornerAnnotation* _Annotation = (vtkCornerAnnotation*)(_currentRender->GetActors2D()->GetItemAsObject(2));
//	//	//_Annotation->SetText(3, "<window>\n<level>");
//	//	//double imageViewer2WindowLevel
//	//	//std::cout << "new window & level " << this->GetImageViewer2()->GetColorWindow() << " x " << this->GetImageViewer2()->GetColorLevel() << std::endl;
//	//	//std::cout << "-----------------------invoke interactor windowlevel event-----------------" << std::endl;
//	//	//this->Interactor->InvokeEvent(vtkCommand::WindowLevelEvent, newWindowLevel);
//	//}
//}
//#endif