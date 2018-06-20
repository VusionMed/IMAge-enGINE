/*=========================================================================
Program:   Vusion ImageEngine
Module:    vtkVUsionMultiView
=========================================================================*/

// .NAME vtkVusionMultiView - Display Multiple images in one window.
// .Note vtkVusionMultiView is a adapter class of vtkImageViewer2

#ifndef vtkVusionMultiView_h
#define vtkVusionMultiView_h

//#include "vtkInteractionImageModule.h" // For export macro
#include "vtkObject.h"
#include "vtkSmartPointer.h"
//#include "vtkImageMapToWindowLevelColors.h"
#include "vtkImageActor.h"
#include "vtkImageMapper3D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkVusionImageMapToWindowLevelColors.h"


class vtkAlgorithm;
class vtkAlgorithmOutput;
//class vtkImageActor;
class vtkImageData;
//class vtkImageMapToWindowLevelColors;
class vtkInformation;
class vtkInteractorStyleImage;
//class vtkRenderWindow;
//class vtkRenderer;
//class vtkRenderWindowInteractor;
class vtkLookupTable;

class vtkVusionMultiView : public vtkObject
{
public:
	static vtkVusionMultiView *New();
	vtkTypeMacro(vtkVusionMultiView, vtkObject);
	void PrintSelf(ostream& os, vtkIndent indent);

	// Description:
	// Get the name of rendering window.
	//virtual const char *GetWindowName();

	// Description:
	// Render the resulting image.
	virtual void Render(void);
	
	// Description:
	// Set/Get the input image to the viewer.
	enum
	{
		IMAGELAYER_BACKGROUND = 0,
		IMAGELAYER_FOREROUND = 1,
		IMAGELAYER_LABEL = 2
	};

	virtual void SetInputConnection(int layer, vtkAlgorithmOutput* input);
	virtual void SetImageLayerInputData(int layer, vtkImageData * image);
	virtual void SetBackgroundImage(vtkImageData *in) { this->SetImageLayerInputData(vtkVusionMultiView::IMAGELAYER_BACKGROUND,in); }
	virtual void SetForegroundImage(vtkImageData *in) { this->SetImageLayerInputData(vtkVusionMultiView::IMAGELAYER_FOREROUND, in); }
	//Label image is forced to use unsigned char pixel type
	virtual void SetLabelImage(vtkImageData *in) { this->SetImageLayerInputData(vtkVusionMultiView::IMAGELAYER_LABEL, in); }

	virtual vtkImageData *  GetImageLayerInputData(int layer);
	virtual vtkImageData *  GetBackgroundImage() { return this->GetImageLayerInputData(vtkVusionMultiView::IMAGELAYER_BACKGROUND); }
	virtual vtkImageData *  GetForegroundImage() { return this->GetImageLayerInputData(vtkVusionMultiView::IMAGELAYER_FOREROUND); }
	virtual vtkImageData * GetLabelImage() { return this->GetImageLayerInputData(vtkVusionMultiView::IMAGELAYER_LABEL); }

	virtual void SetImageLayerOpacity(int layer, double opacity);

	// Description:
	// Set/get the slice orientation
	enum
	{
		SLICE_ORIENTATION_YZ = 0,
		SLICE_ORIENTATION_XZ = 1,
		SLICE_ORIENTATION_XY = 2
	};
	vtkGetMacro(SliceOrientation, int);
	virtual void SetSliceOrientation(int orientation);
	virtual void SetSliceOrientationToXY()
	{
		this->SetSliceOrientation(vtkVusionMultiView::SLICE_ORIENTATION_XY);
	};
	virtual void SetSliceOrientationToYZ()
	{
		this->SetSliceOrientation(vtkVusionMultiView::SLICE_ORIENTATION_YZ);
	};
	virtual void SetSliceOrientationToXZ()
	{
		this->SetSliceOrientation(vtkVusionMultiView::SLICE_ORIENTATION_XZ);
	};

	// Description:
	// Set/Get the current slice to display (depending on the orientation
	// this can be in X, Y or Z).
	//vtkGetMacro(Slice, int);
	virtual void SetSlice(int layer, int slice);
	virtual int GetSlice(int layer)
	{
		if (layer < 0 || layer > 2) return -1;
		return this->Slice[layer];
	}

	// Description:
	// Update the display extent manually so that the proper slice for the
	// given orientation is displayed. It will also try to set a
	// reasonable camera clipping range.
	// This method is called automatically when the Input is changed, but
	// most of the time the input of this class is likely to remain the same,
	// i.e. connected to the output of a filter, or an image reader. When the
	// input of this filter or reader itself is changed, an error message might
	// be displayed since the current display extent is probably outside
	// the new whole extent. Calling this method will ensure that the display
	// extent is reset properly.
	virtual void UpdateDisplayExtent(int layer);

	// Description:
	// Return the minimum and maximum slice values (depending on the orientation
	// this can be in X, Y or Z).
	virtual int* GetSliceRange(int layer);

	// Description:
	// Set window and level for mapping pixels to colors.
	//virtual double GetColorWindow();
	//virtual double GetColorLevel();
	//virtual void SetColorWindow(double s);
	//virtual void SetColorLevel(double s);

	// Description:
	// These are here when using a Tk window.
	//virtual void SetDisplayId(void *a);
	//virtual void SetWindowId(void *a);
	//virtual void SetParentId(void *a);

	// Description:
	// Set/Get the position in screen coordinates of the rendering window.
	//virtual int* GetPosition();
	//virtual void SetPosition(int a, int b);
	//virtual void SetPosition(int a[2]) { this->SetPosition(a[0], a[1]); }

	// Description:
	// Set/Get the size of the window in screen coordinates in pixels.
	virtual int* GetSize();
	virtual void SetSize(int a, int b);
	virtual void SetSize(int a[2]) { this->SetSize(a[0], a[1]); }

	// Description:
	// Get the internal render window, renderer, image actor, and
	// image map instances.
	vtkGetObjectMacro(RenderWindow, vtkRenderWindow);
	vtkGetObjectMacro(Renderer, vtkRenderer);
	//vtkGetObjectMacro(BackgroundImageActor, vtkImageActor);
	//vtkGetObjectMacro(ForegroundImageActor, vtkImageActor);
	//vtkGetObjectMacro(LabelImageActor, vtkImageActor);
	//vtkGetObjectMacro(WindowLevel, vtkImageMapToWindowLevelColors);
	//vtkGetObjectMacro(InteractorStyle, vtkInteractorStyleImage);
	virtual vtkVusionImageMapToWindowLevelColors* GetWindowLevel(int i) {
		if (i < 0 || i > 2) return NULL;
		return this->WindowLevel[i];
	}
	virtual vtkImageActor* GetImageActor(int i) {
		if (i < 0 || i > 2) return NULL;
		return this->ImageActor[i];
	}
	// Description:
	// Set your own renderwindow and renderer
	virtual void SetRenderWindow(vtkRenderWindow *arg);
	virtual void SetRenderer(vtkRenderer *arg);

	// Description:
	// Attach an interactor for the internal render window.
	virtual void SetupInteractor(vtkRenderWindowInteractor*);

	virtual void SetViewModeToGrayScale(int imageLayerType);

	virtual void SetViewModeToColorMap(int imageLayerType);

	vtkSmartPointer <vtkLookupTable> BuildRainBowLookupTable(int imageLayerType);

	virtual void SetInteractorStyleToCameraMode();

	virtual void SetInteractorStyleToActorMode();

	virtual void SetInteractorStyleToTrackBallCamera();

	//virtual void SetInteractorStyleToPaintBrushMode();

	//virtual void SetInteractorStyleToActor();
	//virtual void SetInteractorStyleToCameraMode();

	//virtual void SetInteractorStyleToActorMode();

	//Test brush mode for now
	virtual void SetLookupTable(int layer, vtkLookupTable *lookupTable);
	virtual void SetLookupTableToLinear(int layer);

	// Description:
	// Create a window in memory instead of on the screen. This may not
	// be supported for every type of window and on some windows you may
	// need to invoke this prior to the first render.
	//virtual void SetOffScreenRendering(int);
	//virtual int GetOffScreenRendering();
	//vtkBooleanMacro(OffScreenRendering, int);
	int GetImageIndexFromWorldPoint(double *worldPos, vtkImageData * imageData, int *imageIndex);

	//Funtion: Fill the whole image window
	virtual void UpdateCameraScale();

	//Funciton: place the image Actors in correct order: Label Image, Foreground Image, BackgroundImage (front to back)...
	virtual void UpdateCameraClippingRange();

	virtual void CreateDefaultLookupTableForLabelImage();

	virtual void ImageAutoFillWindow();

protected:
	vtkVusionMultiView();
	~vtkVusionMultiView();

	virtual void InstallPipeline();
	virtual void UnInstallPipeline();

	vtkVusionImageMapToWindowLevelColors  *WindowLevel[3];
	vtkRenderWindow                 *RenderWindow;
	vtkRenderer                     *Renderer;
	vtkImageActor                   *ImageActor[3];
	vtkRenderWindowInteractor       *Interactor;
	vtkInteractorStyleImage         *InteractorStyle;
	vtkLookupTable					*LookupTable[3];

	int SliceOrientation;
	int Slice[3];
	//double initalPosition[3];

	virtual void UpdateOrientation();

	virtual vtkInformation* GetInputInformation(int layer);
	virtual vtkAlgorithm* GetInputAlgorithm(int layer);

	

private:
	vtkVusionMultiView(const vtkVusionMultiView&);  // Not implemented.
	void operator=(const vtkVusionMultiView&);  // Not implemented.
};

#endif
