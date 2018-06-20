/*=========================================================================



=========================================================================*/
// .NAME vtkVusionMultiView - Display Multiple images in one window.
// .Note vtkVusionMultiView is a adapter and extension class of vtkImageViewer2

#include "vtkVusionMultiView.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
//#include "vtkImageActor.h"
#include "vtkImageData.h"
//#include "vtkImageMapper3D.h"
//#include "vtkImageMapToWindowLevelColors.h" //Moved to header
#include "vtkInformation.h"
//#include "vtkInteractorStyleImage.h"
#include "vtkObjectFactory.h"
//#include "vtkRenderWindow.h"
//#include "vtkRenderWindowInteractor.h"
//#include "vtkRenderer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
//merged from vusion sliceview
#include "vtkLookupTable.h"
#include "vtkImageShiftScale.h"
#include "vtkSmartPointer.h"
#include "vtkImageAccumulate.h"
#include "vtkImageExtractComponents.h"
#include "vtkMultiViewInteractorStyleImage.h"
#include "vtkInteractorStyleTrackballActor.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkExtractVOI.h"
#include "vtkImageCast.h"

//#include "vtkMultiViewInteractorStyleImagePaint.h"

//Test
#include <vtkWorldPointPicker.h>
//#include <vtkCullerCollection.h>
//#include <vtkCuller.h>

vtkStandardNewMacro(vtkVusionMultiView);

#define LookupTableHistogramSize 500
#define LookupTableHistogramCoverage 0.98


class vtkMultiViewCallback : public vtkCommand
{
public:
	static vtkMultiViewCallback *New() { return new vtkMultiViewCallback; }

	void Execute(vtkObject *caller,
		unsigned long event,
		void *vtkNotUsed(callData))
	{
		if ((this->MultiViewer == NULL) || (this->MultiViewer->GetImageLayerInputData(vtkVusionMultiView::IMAGELAYER_BACKGROUND) == NULL))
		{
			return;
		}

		if (event == vtkCommand::ResetWindowLevelEvent)
		{
			//use gray scale for now. 
			this->MultiViewer->SetViewModeToGrayScale(vtkVusionMultiView::IMAGELAYER_BACKGROUND);
		}

		////===============Test brush mode begin=========
		//Test brush mode
		//if (event == vtkCommand::WindowLevelEvent)
		////if(event == vtkCommand::RightButtonPressEvent)
		//{
		//	vtkRenderWindowInteractor *rwiBrush = static_cast<vtkRenderWindowInteractor *>(this->MultiViewer->GetRenderWindow()->GetInteractor());
		//	if (!rwiBrush) return;
		//	//std::cout << "display postion: " << rwiBrush->GetEventPosition()[0] << " " << rwiBrush->GetEventPosition()[1] << std::endl;
		//	rwiBrush->GetPicker()->Pick(rwiBrush->GetEventPosition()[0],
		//		rwiBrush->GetEventPosition()[1],
		//		0,  // always zero.
		//		this->MultiViewer->GetRenderer());
		//	double picked[3];
		//	rwiBrush->GetPicker()->GetPickPosition(picked);
		//	//std::cout << "World point position: " << picked[0] << " " << picked[1] << " " << picked[2] << std::endl;

		//	//Off image, return. Use imageIndex is better.
		//	if (picked[0] == 0 && picked[1] == 0 && picked[2] == 0)
		//	{
		//		return;
		//	}

		//	vtkVusionPainter<unsigned char> *painter = new vtkVusionPainter<unsigned char>();
		//	painter->SetBackgroundImage(this->MultiViewer->GetBackgroundImage());
		//	painter->SetLabelImage(this->MultiViewer->GetLabelImage());
		//	painter->SetPainterWorldPostion(picked);
		//	painter->SetPainterSliceOrientation(this->MultiViewer->GetSliceOrientation());
		//	
		//	painter->Paint();
		//	
		//	this->MultiViewer->SetLabelImage(painter->GetLabelImage());
		//	this->MultiViewer->SetSlice(2, this->MultiViewer->GetSlice(0));
		//	this->MultiViewer->SetImageLayerOpacity(2, 0.6);
		//	this->MultiViewer->SetLookupTableToLinear(2);

		//	if (painter)
		//	{
		//		delete painter;
		//		painter = nullptr;
		//	}
		//}
		//============Brush mode end====================




		// handle Window Level event
		if (event != vtkCommand::WindowLevelEvent) return;

		vtkRenderWindowInteractor *rwi = static_cast<vtkRenderWindowInteractor *>(this->MultiViewer->GetRenderWindow()->GetInteractor());
		if (!rwi) return;

		vtkInteractorStyleImage *isi = static_cast<vtkInteractorStyleImage *>(caller);
		if (!isi) return;

		isi->GetWindowLevelCurrentPosition()[0] = rwi->GetEventPosition()[0];
		isi->GetWindowLevelCurrentPosition()[1] = rwi->GetEventPosition()[1];
		

		//double computedWorldPoint[4];
		//isi->ComputeDisplayToWorld(rwi->GetEventPosition()[0], rwi->GetEventPosition()[1], this->MultiViewer->GetSlice(0), computedWorldPoint)
		//std::cout << "display to world: " <<  << std::endl;
		// Forward events
		//vtkInteractorStyleTrackballCamera::OnLeftButtonDown();

		if (this->MultiViewer)
		{
			int *size = this->MultiViewer->GetRenderer()->GetSize();
			double window = this->MultiViewer->GetWindowLevel(vtkVusionMultiView::IMAGELAYER_BACKGROUND)->GetWindow();
			double level = this->MultiViewer->GetWindowLevel(vtkVusionMultiView::IMAGELAYER_BACKGROUND)->GetLevel();

			// Compute normalized delta
			double dx = (isi->GetWindowLevelCurrentPosition()[0] -
				isi->GetWindowLevelStartPosition()[0]) * 0.0005;//modify 
			double dy = (isi->GetWindowLevelCurrentPosition()[1] -
				isi->GetWindowLevelStartPosition()[1]) * 0.0005; // / size[1];//jiangli modify
		
			// Scale by current values
			if (fabs(window) > 0.01)
			{
				dx = dx * window;
			}
			else
			{
				dx = dx * (window < 0 ? -0.01 : 0.01);
			}
			if (fabs(level) > 0.01)
			{
				dy = dy * level;
			}
			else
			{
				dy = dy * (level < 0 ? -0.01 : 0.01);
			}
		
			// Abs so that direction does not flip
		
			if (window < 0.0)
			{
				dx = -1 * dx;
			}
			if (level < 0.0)
			{
				dy = -1 * dy;
			}
		
			// Compute new window level
		
			//double newWindow = dx + window;
			//double newLevel = level - dy;
			double newWindowLevel[2]; 
			newWindowLevel[0] = dx + window;
			newWindowLevel[1] = level - dy;
		
			//if (newWindowLevel[0] < 0.01)
			//{
			//	newWindowLevel[0] = 0.01;
			//}
			//newWindowLevel[1] = newWindowLevel[1] < 0.01 ? 0.01 : newWindowLevel[1];
		
			this->MultiViewer->GetWindowLevel(vtkVusionMultiView::IMAGELAYER_BACKGROUND)->SetWindow((newWindowLevel[0]));
			this->MultiViewer->GetWindowLevel(vtkVusionMultiView::IMAGELAYER_BACKGROUND)->SetLevel((newWindowLevel[1]));
			this->MultiViewer->Render();
		}
	}
	vtkVusionMultiView *MultiViewer = NULL;
};

//----------------------------------------------------------------------------
vtkVusionMultiView::vtkVusionMultiView()
{
	this->RenderWindow = NULL;
	this->Renderer = NULL;
	for (int i = 0; i< 3; i++)
	{
		this->ImageActor[i] = NULL;// vtkImageActor::New();
		this->WindowLevel[i] = NULL;// vtkImageMapToWindowLevelColors::New();
		this->LookupTable[i] = NULL;
	}

	this->Interactor = NULL;
	this->InteractorStyle = NULL;

	for (int i = 0; i< 3; i++)
	{ 
		this->Slice[i] = 0;
	}

	//this->FirstRender = 1;
	this->SliceOrientation = vtkVusionMultiView::SLICE_ORIENTATION_XY;

	// Setup the pipeline
	vtkRenderWindow *renwin = vtkRenderWindow::New();
	this->SetRenderWindow(renwin);
	renwin->Delete();

	vtkRenderer *ren = vtkRenderer::New();
	this->SetRenderer(ren);
	ren->Delete();

	this->InstallPipeline();
}
//
//----------------------------------------------------------------------------
vtkVusionMultiView::~vtkVusionMultiView()
{
	for (int i = 0; i < 3; i++)
	{
		if (this->WindowLevel[i])
		{
			this->WindowLevel[i]->Delete();
			this->WindowLevel[i] = NULL;
		}

		if (this->ImageActor[i])
		{
			this->ImageActor[i]->Delete();
			this->ImageActor[i] = NULL;
		}

		if (this->LookupTable[i])
		{
			this->LookupTable[i]->Delete(); 
			this->LookupTable[i] = NULL;
		}
	}

	if (this->Renderer)
	{
		this->Renderer->Delete();
		this->Renderer = NULL;
	}

	if (this->RenderWindow)
	{
		this->RenderWindow->Delete();
		this->RenderWindow = NULL;
	}

	if (this->Interactor)
	{
		this->Interactor->Delete();
		this->Interactor = NULL;
	}

	//if (this->InteractorStyle)
	//{
	//	this->InteractorStyle->Delete();
	//	this->InteractorStyle = NULL;
	//}
}

//----------------------------------------------------------------------------
void vtkVusionMultiView::SetupInteractor(vtkRenderWindowInteractor *arg)
{
	if (this->Interactor == arg)
	{
		return;
	}

	this->UnInstallPipeline();

	if (this->Interactor)
	{
		this->Interactor->UnRegister(this);
	}

	this->Interactor = arg;

	if (this->Interactor)
	{
		this->Interactor->Register(this);
	}

	this->InstallPipeline();

	if (this->Renderer)
	{
		this->Renderer->GetActiveCamera()->ParallelProjectionOn();
	}
}

//----------------------------------------------------------------------------
void vtkVusionMultiView::SetRenderWindow(vtkRenderWindow *arg)
{
	if (this->RenderWindow == arg)
	{
		return;
	}

	this->UnInstallPipeline();

	if (this->RenderWindow)
	{
		this->RenderWindow->UnRegister(this);
	}

	this->RenderWindow = arg;

	if (this->RenderWindow)
	{
		this->RenderWindow->Register(this);
	}

	this->InstallPipeline();
}

//----------------------------------------------------------------------------
void vtkVusionMultiView::SetRenderer(vtkRenderer *arg)
{
	if (this->Renderer == arg)
	{
		return;
	}

	this->UnInstallPipeline();

	if (this->Renderer)
	{
		this->Renderer->UnRegister(this);
	}

	this->Renderer = arg;

	if (this->Renderer)
	{
		this->Renderer->Register(this);
	}

	this->InstallPipeline();
	this->UpdateOrientation();
}

//----------------------------------------------------------------------------
void vtkVusionMultiView::SetSize(int a, int b)
{
	this->RenderWindow->SetSize(a, b);
}

//----------------------------------------------------------------------------
int* vtkVusionMultiView::GetSize()
{
	return this->RenderWindow->GetSize();
}

//----------------------------------------------------------------------------
//void vtkVusionMultiView::GetSliceRange(int &min, int &max)
//{
//	vtkAlgorithm *input = this->GetInputAlgorithm();
//	if (input)
//	{
//		input->UpdateInformation();
//		int *w_ext = input->GetOutputInformation(0)->Get(
//			vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
//		min = w_ext[this->SliceOrientation * 2];
//		max = w_ext[this->SliceOrientation * 2 + 1];
//	}
//}

//----------------------------------------------------------------------------
int* vtkVusionMultiView::GetSliceRange(int layer)
{
	vtkAlgorithm *input = this->GetInputAlgorithm(layer);
	if (input)
	{
		input->UpdateInformation();
		return input->GetOutputInformation(0)->Get(
			vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()) +
			this->SliceOrientation * 2;
	}
	return NULL;
}

//----------------------------------------------------------------------------
void vtkVusionMultiView::SetSlice(int layer, int slice)
{
	//if (this->Slice[layer] == slice) return;

	int *sliceRange = this->GetSliceRange(layer);

	if (!sliceRange) return;

	if (slice < sliceRange[0])
	{
		slice = static_cast<int>(sliceRange[0]);
	}
	if (slice > sliceRange[1])
	{
		slice = static_cast<int>(sliceRange[1]);
	}

	this->Slice[layer] = slice;
	this->Modified();

	this->UpdateDisplayExtent(layer);
	this->UpdateCameraClippingRange();
}

//
//----------------------------------------------------------------------------
void vtkVusionMultiView::SetSliceOrientation(int orientation)
{
	if (orientation < vtkVusionMultiView::SLICE_ORIENTATION_YZ ||
		orientation > vtkVusionMultiView::SLICE_ORIENTATION_XY)
	{
		vtkErrorMacro("Error - invalid slice orientation " << orientation);
		return;
	}

	//if (this->SliceOrientation == orientation)
	//{
	//	return;
	//}

	this->SliceOrientation = orientation;

	this->UpdateOrientation();
	this->UpdateDisplayExtent(0);
	this->UpdateDisplayExtent(1);
	this->UpdateDisplayExtent(2);

	//if (this->Renderer && this->GetInput())

	if (this->Renderer)
	{
		//double scale = this->Renderer->GetActiveCamera()->GetParallelScale();
		//Bug report: slice = 200 (any large enough number), scoll to slice = 0. camera positon lies between two clipping planes!!
		//Sovle: Clear the slice cache before reset camera
		//only clear background image slice cache for now
		int tmpSlice = this->Slice[0];
		this->SetSlice(0, 0);
		this->Renderer->ResetCamera();
		this->SetSlice(0, tmpSlice);
		//this->Renderer->GetActiveCamera()->SetParallelScale(scale);
		this->ImageAutoFillWindow();
	}
	//this->Render();
}

//----------------------------------------------------------------------------
void vtkVusionMultiView::UpdateOrientation()
{
	// Set the camera position
	vtkCamera *cam = this->Renderer ? this->Renderer->GetActiveCamera() : NULL;
	if (cam)
	{
		switch (this->SliceOrientation)
		{
		case vtkVusionMultiView::SLICE_ORIENTATION_XY:
			//cam->SetFocalPoint(0, 0, 0);
			//cam->SetPosition(0, 0, 1); // -1 if medical ?
			//cam->SetViewUp(0, 1, 0);
			cam->SetFocalPoint(0, 0, 0);
			cam->SetPosition(0, 0, -1);
			cam->SetViewUp(0, -1, 0);
			break;

		case vtkVusionMultiView::SLICE_ORIENTATION_XZ:
			cam->SetFocalPoint(0, 0, 0);
			cam->SetPosition(0, -1, 0); // 1 if medical ?
			cam->SetViewUp(0, 0, 1);
			break;

		case vtkVusionMultiView::SLICE_ORIENTATION_YZ:
			cam->SetFocalPoint(0, 0, 0);
			cam->SetPosition(1, 0, 0); // -1 if medical ?
			cam->SetViewUp(0, 0, 1);
			break;
		}
	}
}

//----------------------------------------------------------------------------
void vtkVusionMultiView::UpdateDisplayExtent(int layer)
{
	vtkAlgorithm *input = this->GetInputAlgorithm(layer);
	if (!input || !this->ImageActor[layer])
	{
		//std::cout <<"image layer: " << layer << " is empty" << std::endl;
		return;
	}

	input->UpdateInformation();
	vtkInformation* outInfo = input->GetOutputInformation(0);
	int *w_ext = outInfo->Get(
		vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

	// Is the slice in range ? If not, fix it
	int slice_min = w_ext[this->SliceOrientation * 2];
	int slice_max = w_ext[this->SliceOrientation * 2 + 1];
	if (this->Slice[layer] < slice_min || this->Slice[layer] > slice_max)
	{
		this->Slice[layer] = static_cast<int>((slice_min + slice_max) * 0.5);
	}
	
	// Set the image actor
	switch (this->SliceOrientation)
	{
	case vtkVusionMultiView::SLICE_ORIENTATION_XY:
		this->ImageActor[layer]->SetDisplayExtent(
			w_ext[0], w_ext[1], w_ext[2], w_ext[3], this->Slice[layer], this->Slice[layer]);
		break;

	case vtkVusionMultiView::SLICE_ORIENTATION_XZ:
		this->ImageActor[layer]->SetDisplayExtent(
			w_ext[0], w_ext[1], this->Slice[layer], this->Slice[layer], w_ext[4], w_ext[5]);
		break;

	case vtkVusionMultiView::SLICE_ORIENTATION_YZ:
		this->ImageActor[layer]->SetDisplayExtent(
			this->Slice[layer], this->Slice[layer], w_ext[2], w_ext[3], w_ext[4], w_ext[5]);
		break;
	}	
}

void vtkVusionMultiView::UpdateCameraScale()
{
	/*if (this->Renderer)
		this->Renderer->ResetCamera();*/
}

void vtkVusionMultiView::UpdateCameraClippingRange()
{
	if (this->Renderer)
	{
		//this->Renderer->ResetCameraClippingRange();

		//this->Renderer->ResetCamera();//no longer needed, because we'er using parrallel projection and camera was resetted during  set slice orientation

		//Comment below for VLung project
		{
			vtkCamera *cam = this->Renderer->GetActiveCamera();
			if (cam)
			{
				//Figure out the correct clipping range according to Label Image/Foreground Image/BackGround Image (priority from high to low)
				int layer = -1;
				if (this->ImageActor[2])
				{
					layer = 2;
				}
				else if (this->ImageActor[1])
				{
					layer = 1;
				}
				else if (this->ImageActor[0])
				{
					layer = 0;
				}

				if (layer == -1) return;

				double bounds[6];
				this->ImageActor[layer]->GetBounds(bounds);
				double *spacing = this->GetInputAlgorithm(layer)->GetOutputInformation(0)->Get(vtkDataObject::SPACING());
				double spos = bounds[this->SliceOrientation * 2];
				double cpos = cam->GetPosition()[this->SliceOrientation];
				double range = fabs(spos - cpos);
				//double avg_spacing = (spacing[0] + spacing[1] + spacing[2]) / 3.0;
				double avg_spacing = spacing[this->SliceOrientation];
				cam->SetClippingRange(
					range - avg_spacing * 0.5, range + avg_spacing * 0.5);
				//std::cout << "Multi View get camera Range: " << cam->GetClippingRange()[0] << " " << cam->GetClippingRange()[1] << std::endl;
				
			}
		}
	}
}

////----------------------------------------------------------------------------
//double vtkVusionMultiView::GetColorWindow()
//{
//	return this->WindowLevel->GetWindow();
//}
//
////----------------------------------------------------------------------------
//double vtkVusionMultiView::GetColorLevel()
//{
//	return this->WindowLevel->GetLevel();
//}
//
////----------------------------------------------------------------------------
//void vtkVusionMultiView::SetColorWindow(double s)
//{
//	this->WindowLevel->SetWindow(s);
//}
//
////----------------------------------------------------------------------------
//void vtkVusionMultiView::SetColorLevel(double s)
//{
//	this->WindowLevel->SetLevel(s);
//}
//


//----------------------------------------------------------------------------
void vtkVusionMultiView::InstallPipeline()
{
	if (this->RenderWindow && this->Renderer)
	{
		this->RenderWindow->AddRenderer(this->Renderer);
	}

	if (this->Interactor)
	{
		//no interactorstyle provided for now
		this->Interactor->SetRenderWindow(this->RenderWindow);
		//this->Interactor->SetInteractorStyle(this->InteractorStyle);
	}

	if (this->Renderer)
	{
		for (int i = 0; i < 3; i++)
		{
			if (this->ImageActor[i])
			{
				this->Renderer->AddViewProp(this->ImageActor[i]);
				//this->Renderer->getpro
			}
		}

		//force to use parrallel projection for medical image view
		this->Renderer->GetActiveCamera()->ParallelProjectionOn();
	}

	for (int i = 0; i < 3; i++)
	{
		if (this->ImageActor[i] && this->WindowLevel[i])
		{
			this->ImageActor[i]->GetMapper()->SetInputConnection(
				this->WindowLevel[i]->GetOutputPort());
		}
	}
}

//----------------------------------------------------------------------------
void vtkVusionMultiView::UnInstallPipeline()
{
	for (int i = 0; i < 3; i++)
	{
		//Do not clear this->WindowLevel[i]
		/*if (this->WindowLevel[i])
		{
			this->WindowLevel[i]->SetInputConnection(NULL);
			this->WindowLevel[i]->SetLookupTable(NULL);
		}*/

		if (this->ImageActor[i])
		{
			this->ImageActor[i]->GetMapper()->SetInputConnection(NULL);
		}
	}
	
	if (this->Renderer)
	{
		this->Renderer->RemoveAllViewProps();
	}

	if (this->RenderWindow && this->Renderer)
	{
		this->RenderWindow->RemoveRenderer(this->Renderer);
	}

	if (this->Interactor)
	{
		this->Interactor->SetInteractorStyle(NULL);
		this->Interactor->SetRenderWindow(NULL);
	}
}

//----------------------------------------------------------------------------
void vtkVusionMultiView::Render()
{
	//see two ways provided by vtkTextbook Chapter 3 Computer graphics primer, section z-buffer.
	//we used a custom way by manipunating the position.

	//Method 1: doesn't seem to work. But the picking order works.
	//Note: This will only solve render depth for objects that are in the same z-depth. Objects in front or behind will draw normally despite having a render depth.
	/*
	The Logic

		We will edit the draw order in the vtkRenderer's viewProps object. All vtkObjects with a render depth of null will be drawn first followed by a descending order of render depths.

		Example:
	Unsorted render depths
		viewProps = [NULL, 3, NULL, 1, 2, 4, NULL]

		Sorted render depths
		viewProps = [NULL, NULL, NULL, 4, 3, 2, 1]*/

	//vtkPropCollection * props = this->Renderer->GetViewProps();
	//props->InitTraversal();
	//
	//for (int i = 0; i < props->GetNumberOfItems(); i++) 
	//{
	//	vtkProp * prop = props->GetNextProp();

	//	std::cout << "prop " << i << " = " << prop << std::endl;
	//	//vtkProp * propB = static_cast<vtkProp *>(propArray->GetItemAsObject(i + 1));
	//	std::cout << "image actor " << i << " = " << this->ImageActor[i] << std::endl;
	//	/*if (propA->GetRenderDepth() == NULL || propB->GetRenderDepth() == NULL) {
	//		if (propA->GetRenderDepth() != NULL && propB->GetRenderDepth() == NULL) {
	//			propArray->ReplaceItem(i, propB);
	//			propArray->ReplaceItem(i + 1, propA);
	//			isUnsorted = true;
	//		}
	//	}
	//	else if (propA->GetRenderDepth() < propB->GetRenderDepth()) {
	//		propArray->ReplaceItem(i, propB);
	//		propArray->ReplaceItem(i + 1, propA);
	//		isUnsorted = true;*/

	//	
	//	//props->item
	//}
	//props->ReplaceItem(0, this->ImageActor[0]);
	//props->ReplaceItem(1, this->ImageActor[1]);
	//props->ReplaceItem(2, this->ImageActor[2]);
	

	//Method 2:
	//Adjust position based on background image actor
	//if (this->Renderer)
	//{
	//	//this->Renderer->GetCullers()->GetLastItem()->SetSortingStyleToBackToFront();
	//	//this->Renderer->GetViewProps()->
	//	if (this->ImageActor[0])
	//	{
	//		vtkCamera *cam = this->Renderer->GetActiveCamera();
	//		//cam->GetDirectionOfProjection();
	//		if (cam)
	//		{
	//			double motion_vector[3];
	//			cam->GetDirectionOfProjection(motion_vector);
	//			for (int i = 0; i < 3; i++)
	//			{
	//				motion_vector[i] *= -0.001;//0.001 should be small enough.use 0.01*spacing is the serious way, but why bother.
	//			}
	//			//std::cout << "motion_vector =  " << motion_vector[0] << "x" << motion_vector[1] << "x" << motion_vector[2] << std::endl;

	//			if (this->ImageActor[1])
	//			{
	//				this->ImageActor[1]->SetPosition(this->ImageActor[0]->GetPosition());
	//				this->ImageActor[1]->AddPosition(motion_vector);
	//			}

	//			if (this->ImageActor[2])
	//			{
	//				this->ImageActor[2]->SetPosition(this->ImageActor[0]->GetPosition());
	//				this->ImageActor[2]->AddPosition(motion_vector);
	//				this->ImageActor[2]->AddPosition(motion_vector);
	//			}

	//			/*for (int i = 0; i < 3; i++)
	//			{
	//				double position[3];
	//				this->GetImageActor(i)->GetPosition(position);
	//				std::cout << "position: " << position[0] << "x" << position[1] << "x" << position[2] << endl;

	//				double bounds[6];
	//				this->GetImageActor(i)->GetBounds(bounds);
	//				std::cout << "bounds: " << bounds[0] << "x" << bounds[1] << "x" << bounds[2] << "x" << bounds[3] << "x" << bounds[4] << "x" << bounds[5] << endl;
	//			}*/
	//		}
	//	}
	//}

	//To support Rendering of multiple actors. not so general
	if (this->Renderer)
	{
		if (!this->ImageActor[0])
		{
			this->RenderWindow->Render();
			return;
		}

		vtkCamera *cam = this->Renderer->GetActiveCamera();
		if (cam)
		{
			double motion_vector[3];
			cam->GetDirectionOfProjection(motion_vector);
			for (int i = 0; i < 3; i++)
			{
				motion_vector[i] *= 0.005;//changed from -0.001 to 0.001;//0.001 should be small enough.use 0.01*spacing is the serious way, but why bother.
			}

			for (int i = 0; i < 3; i++)
			{
				if (this->ImageActor[i])
				{
					double actorPos[3]; 
					this->ImageActor[i]->GetPosition(actorPos);
					if (actorPos[0] == 0 && actorPos[1] == 0 && actorPos[2] == 0)
					{
						for (int j = 3 - i; j > 0; j--)
						{
							this->ImageActor[i]->AddPosition(motion_vector);
						}
					}
				}
			}

			//if (this->ImageActor[0])
			//{
			//	/*double position[3];
			//	this->GetImageActor(0)->GetPosition(position);
			//	double bounds[6];
			//	this->GetImageActor(0)->GetBounds(bounds);	
			//	std::cout << "bounds: " << bounds[0] << "x" << bounds[1] << "x" << bounds[2] << "x" << bounds[3] << "x" << bounds[4] << "x" << bounds[5] << endl;
			//	this->GetImageActor(0)->GetPosition(position);
			//	std::cout << "position: " << position[0] << "x" << position[1] << "x" << position[2] << endl;*/

			//	
			//	this->ImageActor[0]->SetPosition(0,0,0);//Assuming 0, 0, 0 is the default positon. cache it using an array later.
			//	this->ImageActor[0]->AddPosition(motion_vector);
			//	this->ImageActor[0]->AddPosition(motion_vector);
			//	this->ImageActor[0]->AddPosition(motion_vector);
			//}

			//if (this->ImageActor[1])
			//{
			//	this->ImageActor[1]->SetPosition(0, 0, 0);//Assuming 0, 0, 0 is the default positon. cache it using an array later.
			//	this->ImageActor[1]->AddPosition(motion_vector);
			//	this->ImageActor[1]->AddPosition(motion_vector);
			//}

			//if (this->ImageActor[2])
			//{
			//	this->ImageActor[2]->SetPosition(0, 0, 0);//Assuming 0, 0, 0 is the default positon. cache it using an array later.
			//	this->ImageActor[2]->AddPosition(motion_vector);
			//}			
		}
		this->RenderWindow->Render();
	}
}

void vtkVusionMultiView::SetViewModeToGrayScale(int imageLayerType)
{
	if (!this->GetImageLayerInputData(imageLayerType)) return;

	//if (!this->WindowLevel->GetLookupTable()) return;

	if (this->WindowLevel[imageLayerType])
	{
		this->WindowLevel[imageLayerType]->SetLookupTable(NULL);

		//Reset window and level: To discuss, use current slice data instead of 3D input data or not
		{
			//find current slice data
			/*int w_ext[6];
			this->GetImageLayerInputData(imageLayerType)->GetExtent(w_ext);

			vtkSmartPointer <vtkExtractVOI> ExtractVOI = vtkSmartPointer <vtkExtractVOI>::New();
			ExtractVOI->SetInputData(this->GetImageLayerInputData(imageLayerType));
			switch (this->SliceOrientation)
			{
			case vtkVusionMultiView::SLICE_ORIENTATION_XY:
				ExtractVOI->SetVOI(w_ext[0], w_ext[1], w_ext[2], w_ext[3], this->Slice[imageLayerType], this->Slice[imageLayerType]);
				break;

			case vtkVusionMultiView::SLICE_ORIENTATION_XZ:
				ExtractVOI->SetVOI(w_ext[0], w_ext[1], this->Slice[imageLayerType], this->Slice[imageLayerType], w_ext[4], w_ext[5]);
				break;

			case vtkVusionMultiView::SLICE_ORIENTATION_YZ:
				ExtractVOI->SetVOI(this->Slice[imageLayerType], this->Slice[imageLayerType], w_ext[2], w_ext[3], w_ext[4], w_ext[5]);
				break;
			}
			ExtractVOI->Update();*/

			//find range
			double range[2];
			//ExtractVOI->GetOutput()->GetScalarRange(range);
			this->GetImageLayerInputData(imageLayerType)->GetScalarRange(range);
			double window = (range[1] - range[0]);
			double level = 0.5 * (range[1] + range[0]);

			this->WindowLevel[imageLayerType]->SetWindow(window);
			this->WindowLevel[imageLayerType]->SetLevel(level);
		}

		this->Modified();
		//this->ImageAutoFillWindow();
		this->Render();
	}
}

void vtkVusionMultiView::SetViewModeToColorMap(int imageLayerType)
{
	if (!this->GetImageLayerInputData(imageLayerType)) return;

	if (this->WindowLevel[imageLayerType])
	{	
		if (!LookupTable[imageLayerType])
		{
			vtkSmartPointer<vtkLookupTable> rainBowTable = this->BuildRainBowLookupTable(imageLayerType);
			this->SetLookupTable(imageLayerType, rainBowTable.GetPointer());
		}
		
		//this->WindowLevel[imageLayerType]->Update();
		this->WindowLevel[imageLayerType]->SetLookupTable(LookupTable[imageLayerType]);
		//this->WindowLevel[imageLayerType]->PassAlphaToOutputOn();
		//std::cout << "------------alpha to output on ? " << this->WindowLevel[imageLayerType]->GetPassAlphaToOutput() << std::endl;
		//Reset color window level
		{
			//double range[2];
			////this->WindowLevel[imageLayerType]->GetOutput()->GetScalarRange(range);
			////this->LookupTable[imageLayerType]->GetTableRange(range);
			//this->GetImageLayerInputData(imageLayerType)->GetScalarRange(range);

			//double window = (range[1] - range[0]);
			//double level = 0.5 * (range[1] + range[0]);

			//this->WindowLevel[imageLayerType]->SetWindow(//1348);
			//	window);
			//this->WindowLevel[imageLayerType]->SetLevel(//674);
			//	level);

			//maybe we should apply this to all color map view mode.
			//adjust window level for color doesn't make sense.
			//if (imageLayerType == 2)
			{
				this->WindowLevel[imageLayerType]->SetWindow(//1348);
					1);
				this->WindowLevel[imageLayerType]->SetLevel(//674);
					0.5);
			}
		}

		//double startRender = clock();
		
		//this->Render();
		
		//double endRender = clock();
		//std::cout << (endRender - startRender) / CLOCKS_PER_SEC \
		//	<< " seconds (RenderInside!)" << std::endl;
	}
}

vtkSmartPointer<vtkLookupTable> vtkVusionMultiView::BuildRainBowLookupTable(int imageLayerType)
{
	if (!this->GetImageLayerInputData(imageLayerType)) return NULL;

	//compute interal look up table range according to the 1st component data
	vtkSmartPointer <vtkImageExtractComponents> scalarComponent = vtkSmartPointer <vtkImageExtractComponents>::New();
	scalarComponent->SetInputData(this->GetImageLayerInputData(imageLayerType));
	scalarComponent->SetComponents(0);
	scalarComponent->Update();

	double inputDataRange[2];
	scalarComponent->GetOutput()->GetScalarRange(inputDataRange);
	double spacing = (inputDataRange[1] - inputDataRange[0]) / LookupTableHistogramSize;
	spacing = spacing < 0 ? 1 : spacing;

	vtkSmartPointer<vtkImageAccumulate> imageAccumulate = vtkSmartPointer<vtkImageAccumulate>::New();
	imageAccumulate->SetInputData(scalarComponent->GetOutput());
	imageAccumulate->SetComponentExtent(0, LookupTableHistogramSize - 1, 0, 0, 0, 0);
	imageAccumulate->SetComponentOrigin(inputDataRange[0], 0, 0);
	imageAccumulate->SetComponentSpacing(spacing, 0, 0); // This will count exactly the number of pixels of each color. Use (10,0,0) to make bins of width 10 instead.
	imageAccumulate->Update();

	int histogramBins[LookupTableHistogramSize];
	for (vtkIdType bin = 0; bin < LookupTableHistogramSize; ++bin)
	{
		histogramBins[bin] = *(static_cast<int*>(imageAccumulate->GetOutput()->GetScalarPointer(bin, 0, 0)));
	}
	//Discard edge points
	unsigned int pixelsInHistogram = 0;//do not count min values
	for (unsigned int i = 1; i < LookupTableHistogramSize; i++)
	{
		pixelsInHistogram += histogramBins[i];
	}

	if (!pixelsInHistogram)	return NULL;

	double computedRange[2];
	unsigned int nrPixelsToIgnore = (unsigned int)((1.0 - LookupTableHistogramCoverage)*pixelsInHistogram);
	unsigned int upperIndex = LookupTableHistogramSize - 1;
	unsigned int lowerIndex = 1;// do not count background pixels
	unsigned int nrPixelsIgnored = 0;

	while ((nrPixelsIgnored < nrPixelsToIgnore) && (upperIndex - lowerIndex > 2))
	{
		if (histogramBins[lowerIndex] < histogramBins[upperIndex])
		{
			nrPixelsIgnored += histogramBins[lowerIndex];
			if (nrPixelsIgnored < nrPixelsToIgnore)
			{
				lowerIndex++;
			}
		}
		else
		{
			nrPixelsIgnored += histogramBins[upperIndex];
			if (nrPixelsIgnored < nrPixelsToIgnore)
			{
				upperIndex--;
			}
		}
	}

	//Now compute the new extremes.
	double range = (double)(inputDataRange[1] - inputDataRange[0]);
	double cutOff = (double)(lowerIndex) / LookupTableHistogramSize;

	computedRange[0] = cutOff*range + inputDataRange[0];
	cutOff = (double)(upperIndex + 1) / LookupTableHistogramSize;
	computedRange[1] = cutOff*range + inputDataRange[0];

	int numberofValues = int(computedRange[1]) - int(computedRange[0]) + 1;

	vtkSmartPointer<vtkLookupTable> internalRainBowTable = vtkSmartPointer<vtkLookupTable>::New();
	internalRainBowTable->SetNumberOfTableValues(numberofValues);//try below color range
	internalRainBowTable->SetTableRange(computedRange[0], computedRange[1]);//try below color range
	internalRainBowTable->SetHueRange(0.66667, 0.0);//rainbow color map: from blue to red
	internalRainBowTable->UseBelowRangeColorOn();
	internalRainBowTable->SetBelowRangeColor(0.0, 0.0, 0.0, 1.0);
	internalRainBowTable->Build();

	return internalRainBowTable;
}

////----------------------------------------------------------------------------
//const char* vtkVusionMultiView::GetWindowName()
//{
//	return this->RenderWindow->GetWindowName();
//}
//
////----------------------------------------------------------------------------
//void vtkVusionMultiView::SetOffScreenRendering(int i)
//{
//	this->RenderWindow->SetOffScreenRendering(i);
//}
//
////----------------------------------------------------------------------------
//int vtkVusionMultiView::GetOffScreenRendering()
//{
//	return this->RenderWindow->GetOffScreenRendering();
//}
//
////----------------------------------------------------------------------------
//void vtkVusionMultiView::SetInputData(vtkImageData *in)
//{
//	this->WindowLevel->SetInputData(in);
//	this->UpdateDisplayExtent();
//}

//----------------------------------------------------------------------------
void vtkVusionMultiView::SetImageLayerInputData(int layer, vtkImageData * image)
{
	if (layer > 2 || layer < 0) return;
	
	//initialize actor and window level if already there
	if (this->WindowLevel[layer])
	{
		this->WindowLevel[layer]->SetInputConnection(NULL);
		this->WindowLevel[layer]->SetLookupTable(NULL);
	}

	if (this->ImageActor[layer])
	{
		this->ImageActor[layer]->GetMapper()->SetInputConnection(NULL);

		if (this->Renderer)
		{
			this->Renderer->RemoveViewProp(this->ImageActor[layer]);
		}
	}

	//handle null image, so that we can use it as reset
	if (!image)
	{
		if (this->WindowLevel[layer])
		{
			this->WindowLevel[layer]->Delete();
			this->WindowLevel[layer] = NULL;
		}

		if (this->ImageActor[layer])
		{
			this->ImageActor[layer]->Delete();
			this->ImageActor[layer] = NULL;
		}

		return;
	}


	//create if not there
	if (!this->WindowLevel[layer])
	{
		this->WindowLevel[layer] = vtkVusionImageMapToWindowLevelColors::New();
	}

	if (!this->ImageActor[layer])
	{
		this->ImageActor[layer] = vtkImageActor::New();
	}
	//Add to pipeline
	if (this->ImageActor[layer] && this->WindowLevel[layer])
	{
		this->ImageActor[layer]->GetMapper()->SetInputConnection(
			this->WindowLevel[layer]->GetOutputPort());
	}

	if (this->Renderer)
	{
		this->Renderer->AddViewProp(this->ImageActor[layer]);
	}

	//Force Label Image to unsigned char type
	if (layer != vtkVusionMultiView::IMAGELAYER_LABEL)
	{
		this->WindowLevel[layer]->SetInputData(image);
	}
	else
	{
		if (image->GetScalarType() == VTK_UNSIGNED_CHAR)
		{
			this->WindowLevel[layer]->SetInputData(image);
			//this->WindowLevel[layer]->Update();
		}else
		{
			vtkSmartPointer <vtkImageCast> castFilter = vtkSmartPointer<vtkImageCast>::New();
			castFilter->SetInputData(image);
			castFilter->SetOutputScalarTypeToUnsignedChar();
			castFilter->Update();
			this->WindowLevel[layer]->SetInputData(castFilter->GetOutput());
		}
	}
	this->UpdateDisplayExtent(layer);// updated multiple times if function get called multiple times
}
//----------------------------------------------------------------------------
vtkImageData* vtkVusionMultiView::GetImageLayerInputData(int layer)
{
	if (layer > 2 || layer < 0) return NULL;

	if (!this->WindowLevel[layer]) return NULL;

	return vtkImageData::SafeDownCast(this->WindowLevel[layer]->GetInput());
}
//----------------------------------------------------------------------------
vtkInformation* vtkVusionMultiView::GetInputInformation(int layer)
{
	if (layer > 2 || layer < 0) return NULL;

	if (!this->WindowLevel[layer]) return NULL;

	return this->WindowLevel[layer]->GetInputInformation();
}
//----------------------------------------------------------------------------
vtkAlgorithm* vtkVusionMultiView::GetInputAlgorithm(int layer)
{
	if (layer > 2 || layer < 0) return NULL;

	if (!this->WindowLevel[layer]) return NULL;

	return this->WindowLevel[layer]->GetInputAlgorithm();
}
//----------------------------------------------------------------------------
void vtkVusionMultiView::SetInputConnection(int layer, vtkAlgorithmOutput* input)
{
	if (layer > 2 || layer < 0) return;

	//initialize actor and window level if already there
	if (this->WindowLevel[layer])
	{
		this->WindowLevel[layer]->SetInputConnection(NULL);
		this->WindowLevel[layer]->SetLookupTable(NULL);
	}

	if (this->ImageActor[layer])
	{
		this->ImageActor[layer]->GetMapper()->SetInputConnection(NULL);

		if (this->Renderer)
		{
			this->Renderer->RemoveViewProp(this->ImageActor[layer]);
		}
	}
	
	//create if not there
	if (!this->WindowLevel[layer])
	{
		this->WindowLevel[layer] = vtkVusionImageMapToWindowLevelColors::New();
	}

	if (!this->ImageActor[layer])
	{
		this->ImageActor[layer] = vtkImageActor::New();
	}

	//Add to pipeline
	if (this->ImageActor[layer] && this->WindowLevel[layer])
	{
		this->ImageActor[layer]->GetMapper()->SetInputConnection(
			this->WindowLevel[layer]->GetOutputPort());
	}

	if (this->Renderer)
	{
		this->Renderer->AddViewProp(this->ImageActor[layer]);
	}

	this->WindowLevel[layer]->SetInputConnection(input);
	this->UpdateDisplayExtent(layer);
}

//----------------------------------------------------------------------------
void vtkVusionMultiView::SetImageLayerOpacity(int layer, double opacity)
{
	if (layer > 2 || layer < 0) return;

	if (this->ImageActor[layer])
	{
		this->ImageActor[layer]->SetOpacity(opacity);
	}
}

//----------------------------------------------------------------------------
void vtkVusionMultiView::SetInteractorStyleToCameraMode()
{
	if (!this->GetRenderWindow()->GetInteractor())
	{
		vtkSmartPointer< vtkRenderWindowInteractor> interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
		this->SetupInteractor(interactor);
	}

	vtkSmartPointer<vtkMultiViewInteractorStyleImage> cameraModeStyle = vtkSmartPointer<vtkMultiViewInteractorStyleImage>::New();
	vtkMultiViewCallback *cbk = vtkMultiViewCallback::New();
	cbk->MultiViewer = this;
	cameraModeStyle->AddObserver(
		vtkCommand::WindowLevelEvent, cbk);

	//cameraModeStyle->AddObserver(
		//vtkCommand::StartWindowLevelEvent, cbk);
	//cameraModeStyle->AddObserver(
	//	vtkCommand::LeftButtonPressEvent, cbk);
	cameraModeStyle->AddObserver(
		vtkCommand::ResetWindowLevelEvent, cbk);
	cbk->Delete();

	this->GetRenderWindow()->GetInteractor()->SetInteractorStyle(cameraModeStyle);
}

//----------------------------------------------------------------------------
void vtkVusionMultiView::SetInteractorStyleToActorMode()
{
	if (!this->GetRenderWindow()->GetInteractor())
	{
		vtkSmartPointer< vtkRenderWindowInteractor> interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
		this->SetupInteractor(interactor);
	}

	vtkSmartPointer<vtkInteractorStyleTrackballActor> actorModeStyle = vtkSmartPointer<vtkInteractorStyleTrackballActor>::New();
	//vtkMultiViewCallback *cbk = vtkMultiViewCallback::New();
	//cbk->MultiViewer = this;
	/*actorModeStyle->AddObserver(
		vtkCommand::RightButtonPressEvent, cbk);*/
	this->GetRenderWindow()->GetInteractor()->SetInteractorStyle(actorModeStyle);
}

//----------------------------------------------------------------------------
void vtkVusionMultiView::SetInteractorStyleToTrackBallCamera()
{
	if (!this->GetRenderWindow()->GetInteractor())
	{
		vtkSmartPointer< vtkRenderWindowInteractor> interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
		this->SetupInteractor(interactor);
	}

	vtkSmartPointer<vtkInteractorStyleTrackballCamera> ballCameraStyle = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
	//vtkMultiViewCallback *cbk = vtkMultiViewCallback::New();
	//cbk->MultiViewer = this;
	/*actorModeStyle->AddObserver(
	vtkCommand::RightButtonPressEvent, cbk);*/
	this->GetRenderWindow()->GetInteractor()->SetInteractorStyle(ballCameraStyle);
}

//----------------------------------------------------------------------------
//void vtkVusionMultiView::SetInteractorStyleToPaintBrushMode()
//{
//	if (!this->GetRenderWindow()->GetInteractor())
//	{
//		vtkSmartPointer< vtkRenderWindowInteractor> interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
//		this->SetupInteractor(interactor);
//	}
//
//	vtkSmartPointer<vtkMultiViewInteractorStyleImagePaint> paintStyle = vtkSmartPointer<vtkMultiViewInteractorStyleImagePaint>::New();
//	this->GetRenderWindow()->GetInteractor()->SetInteractorStyle(paintStyle);
//	paintStyle->SetMultiViewer(this);	
//}


void vtkVusionMultiView::SetLookupTable(int layer, vtkLookupTable *lookupTable)
{
	if (layer > 2 || layer < 0) return;

	if (this->LookupTable[layer] == lookupTable) return;

	if (this->LookupTable[layer])
	{
		this->LookupTable[layer]->UnRegister(this);
	}

	this->LookupTable[layer] = lookupTable;

	if (this->LookupTable[layer])
	{
		this->LookupTable[layer]->Register(this);
	}
}

//specifically designed for label image, though other layer could use it.
void vtkVusionMultiView::SetLookupTableToLinear(int layer)
{
	if (layer > 2 || layer < 0) return;

	if (!this->WindowLevel[layer]) return;

	vtkSmartPointer<vtkLookupTable> linearLookupTable = vtkSmartPointer<vtkLookupTable>::New();
	linearLookupTable->SetNumberOfTableValues(256);//try below color range
	linearLookupTable->SetTableRange(0, 255);//try below color range
	linearLookupTable->SetHueRange(0.66667, 0.0);//rainbow color map: from blue to red
	linearLookupTable->UseBelowRangeColorOn();
	linearLookupTable->SetBelowRangeColor(0.0, 0.0, 0.0, 1.0);
	linearLookupTable->Build();

	
		/*0 background 0 0 0 0
		1 tissue 128 174 128 255
		2 bone 241 214 145 255
		3 skin 177 122 101 255
		4 connective_tissue 111 184 210 255
		5 blood 216 101 79 255*/
	//vtkSmartPointer<vtkLookupTable> labelImageLookupTable = vtkSmartPointer<vtkLookupTable>::New();
	//labelImageLookupTable->SetNumberOfTableValues(6);
	//labelImageLookupTable->SetTableRange(0,5);
	//labelImageLookupTable->UseBelowRangeColorOn();
	//labelImageLookupTable->SetBelowRangeColor(0.0, 0.0, 0.0, 0.0);
	////labelImageLookupTable->UseAboveRangeColorOn();
	////labelImageLookupTable->SetAboveRangeColor(0.0, 0.0, 0.0, 0.5);
	//labelImageLookupTable->SetTableValue(0, 0, 0, 0, 0);
	//labelImageLookupTable->SetTableValue(1, 0.502, 0.682, 0.502, 1);
	//labelImageLookupTable->SetTableValue(2, 0.945, 0.839, 0.569, 1);
	//labelImageLookupTable->SetTableValue(3, 0.694, 0.478, 0.396, 1);
	//labelImageLookupTable->SetTableValue(4, 0.435, 0.722, 0.824, 1);
	//labelImageLookupTable->SetTableValue(5, 0.847, 0.396, 0.310, 1);
	//labelImageLookupTable->Build();

	this->SetLookupTable(layer, linearLookupTable.GetPointer());
	//this->WindowLevel[layer]->SetLookupTable(LookupTable[layer]);
	//this->SetViewModeToColorMap(layer);
	//this->SetLookupTable(layer, NULL);
}

//Replace lookuptable to linear
void vtkVusionMultiView::CreateDefaultLookupTableForLabelImage()
{
	//if (!this->WindowLevel[2]) return;
	/*0 background 0 0 0 0
	0 background 0 0 0 0
	1 tissue 128 174 128 255
	2 bone 241 214 145 255
	3 skin 177 122 101 255
	4 connective_tissue 111 184 210 255
	5 blood 216 101 79 255
	6 organ 221 130 101 255
	7 mass 144 238 144 255
	8 muscle 192 104 88 255
	9 foreign_object 220 245 20 255*/
	vtkSmartPointer<vtkLookupTable> labelImageLookupTable = vtkSmartPointer<vtkLookupTable>::New();
	labelImageLookupTable->SetNumberOfTableValues(10);
	labelImageLookupTable->SetTableRange(0, 9);
	labelImageLookupTable->UseBelowRangeColorOn();
	labelImageLookupTable->SetBelowRangeColor(0.0, 0.0, 0.0, 0.0);
	//labelImageLookupTable->UseAboveRangeColorOn();
	//labelImageLookupTable->SetAboveRangeColor(0.0, 0.0, 0.0, 0.5);
	labelImageLookupTable->SetTableValue(0, 0, 0, 0, 0);
	
	labelImageLookupTable->SetTableValue(1, 128 / 255.0, 174 / 255.0, 128 / 255.0, 1);
	labelImageLookupTable->SetTableValue(2, 241 / 255.0, 214 / 255.0, 145 / 255.0, 1);
	labelImageLookupTable->SetTableValue(3, 177 / 255.0, 122 / 255.0, 101 / 255.0, 1);
	labelImageLookupTable->SetTableValue(4, 111 / 255.0, 184 / 255.0, 210 / 255.0, 1);
	labelImageLookupTable->SetTableValue(5, 216 / 255.0, 101 / 255.0, 79 / 255.0, 1);
	labelImageLookupTable->SetTableValue(6, 221 / 255.0, 130 / 255.0, 101 / 255.0, 1);
	labelImageLookupTable->SetTableValue(7, 144 / 255.0, 238 / 255.0, 144 / 255.0, 1);
	labelImageLookupTable->SetTableValue(8, 192 / 255.0, 104 / 255.0, 88 / 255.0, 1);
	labelImageLookupTable->SetTableValue(9, 220 / 255.0, 245 / 255.0, 20 / 255.0, 1);

	labelImageLookupTable->Build();

	this->SetLookupTable(2, labelImageLookupTable.GetPointer());
}

void vtkVusionMultiView::ImageAutoFillWindow()
{
	if (!this->GetRenderer()->GetActiveCamera()) return;
	int orientation = this->GetSliceOrientation();
	int *size = this->GetRenderWindow()->GetSize();
	double *imageBounds = this->GetRenderer()->ComputeVisiblePropBounds();
	this->GetRenderer()->GetActiveCamera()->ParallelProjectionOn();
	double xFov(0.0), yFov(0.0);
	if (orientation == 0)
	{
		xFov = imageBounds[3] - imageBounds[2];
		yFov = imageBounds[5] - imageBounds[4];
	}
	else if (orientation == 1)
	{
		xFov = imageBounds[1] - imageBounds[0];
		yFov = imageBounds[5] - imageBounds[4];
	}
	else if (orientation == 2)
	{
		xFov = imageBounds[1] - imageBounds[0];
		yFov = imageBounds[3] - imageBounds[2];
	}

	double screenRatio = double(size[1]) / size[0];//height / width;
	double imageRatio = yFov / xFov;
	double parallelScale = imageRatio > screenRatio ? yFov : screenRatio / imageRatio*yFov;
	this->GetRenderer()->GetActiveCamera()->SetParallelScale(0.5*parallelScale);
}

//3D worldPos can be get from picker
int vtkVusionMultiView::GetImageIndexFromWorldPoint(double *worldPos, vtkImageData * imageData, int *imageIndex)
{
	if (!imageData) return 0;

	vtkIdType ptId = imageData->FindPoint(worldPos);

	if (ptId == -1)
	{
		return 0;
	}

	double closestPt[3];
	imageData->GetPoint(ptId, closestPt);

	double origin[3];
	imageData->GetOrigin(origin);
	double spacing[3];
	imageData->GetSpacing(spacing);
	int extent[6];
	imageData->GetExtent(extent);

	int index[3];
	int tempIndex;
	for (int i = 0; i < 3; i++)
	{
		// compute world to image coords
		tempIndex = vtkMath::Round((closestPt[i] - origin[i]) / spacing[i]);

		// we have a valid pick already, just enforce bounds check
		imageIndex[i] = (tempIndex < extent[2 * i]) ? extent[2 * i] : ((tempIndex > extent[2 * i + 1]) ? extent[2 * i + 1] : tempIndex);

		// update world position
		worldPos[i] = imageIndex[i] * spacing[i] + origin[i];
	}

	//Get pixel value
	//double pixelValue = imageData->GetScalarComponentAsDouble(\
	//	static_cast<int>(imageIndex[0]),
	//	static_cast<int>(imageIndex[1]),
	//	static_cast<int>(imageIndex[2]), 0);

	return 1;
}

//----------------------------------------------------------------------------
void vtkVusionMultiView::PrintSelf(ostream& os, vtkIndent indent)
{
	this->Superclass::PrintSelf(os, indent);
}
