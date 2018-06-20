#ifndef vtkVusionPainter_H
#define vtkVusionPainter_H

#include "vtkImageData.h"
#include "vtkSmartPointer.h"
#include "vtkMath.h"
#include "vtkPoints.h"

template<typename LabelImagePixelType>
class vtkVusionPainter
{
public:
	//typedef unsigned char LabelImagePixelType;
	vtkVusionPainter() {
		m_PainterRadius = 5;//unit in pixel or mm
		m_PainterValue = 0;
		m_LabelImage = NULL;
		m_BackgroundImage = NULL;
		for (int i = 0; i < 3; i++)
		{
			m_WorldPosition[i] = 0;
			m_ImageIndex[i] = 0;
		}
		m_PainterShape = 3;
		m_PainterSliceOrientation = 0;
	}

	~vtkVusionPainter()
	{
		//if (m_LabelImage) m_LabelImage = NULL;
	}

	void SetPainterDiameter(int diameter)
	{
		m_PainterRadius = diameter / 2;
	}

	int GetPainterRadius()
	{
		return this->m_PainterRadius;
	}

	void SetPainterValue(LabelImagePixelType value)
	{
		m_PainterValue = value;
	}

	LabelImagePixelType GetPainterValue()
	{
		return this->m_PainterValue;
	}

	void SetPainterSliceOrientation(int ori)
	{
		m_PainterSliceOrientation = ori;
	}

	void SetLabelImage(vtkImageData *labelImage)
	{
		m_LabelImage = labelImage;
	}

	void SetBackgroundImage(vtkImageData *backgroundImage)
	{
		m_BackgroundImage = backgroundImage;
	}

	void SetPainterShape(int shape)
	{
		m_PainterShape = shape;
	}

	void SetPainterWorldPostion(double *worldPos)
	{
		for (int i = 0; i < 3; i++)
		{
			m_WorldPosition[i] = worldPos[i];
		}
	}

	/*void SetPainterWorldPositions(std::vector<double>)
	{
		for (int i = 0; i < 3; i++)
		{
			m_ImageIndex[i] = imageIndex[i];
		}
	}*/

	int GetImageIndexFromWorldPosition(double *worldPos, vtkImageData * imageData, int *imageIndex)
	{
		if (!imageData) return 0;

		//vtkIdType ptId = imageData->FindPoint(worldPos);

		//if (ptId == -1)
		//{
		//	return 0;
		//}

		//double closestPt[3];
		//imageData->GetPoint(ptId, closestPt);

		//double origin[3];
		//imageData->GetOrigin(origin);
		//double spacing[3];
		//imageData->GetSpacing(spacing);
		//int extent[6];
		//imageData->GetExtent(extent);

		////int index[3];
		//int tempIndex;
		//for (int i = 0; i < 3; i++)
		//{
		//	// compute world to image coords
		//	tempIndex = vtkMath::Round((closestPt[i] - origin[i]) / spacing[i]);

		//	// we have a valid pick already, just enforce bounds check
		//	imageIndex[i] = (tempIndex < extent[2 * i]) ? extent[2 * i] : ((tempIndex > extent[2 * i + 1]) ? extent[2 * i + 1] : tempIndex);

		//	//std::cout << "imageIndex[" << i << "] = " << imageIndex[i] << std::endl;
		//	// update world position
		//	worldPos[i] = imageIndex[i] * spacing[i] + origin[i];
		//}

		double origin[3];
		imageData->GetOrigin(origin);
		double spacing[3];
		imageData->GetSpacing(spacing);
		int extent[6];
		imageData->GetExtent(extent);

		for (int i = 0; i < 3; i++)
		{
			imageIndex[i] = vtkMath::Round((worldPos[i] - origin[i]) / spacing[i]) - extent[2*i];
		}
		for (int i = 0; i < 3; i++)
		{
			//std::cout << "Painter: imageIndex[" << i << "] = " << imageIndex[i] << std::endl;
		}
		return 1;
	}

	void PaintPixel()
	{
		if (m_WorldPosition[0] == 0 && m_WorldPosition[1] == 0 && m_WorldPosition[2] == 0)
		{
			return;
		}

		if (!m_BackgroundImage)
		{
			std::cout << "NULL background image in painter" << std::endl;
			return;
		}





		if (!m_LabelImage)
		{
			/*m_LabelImage = vtkSmartPointer<vtkImageData>::New();
			m_LabelImage->SetDimensions(m_BackgroundImage->GetDimensions());
			m_LabelImage->SetSpacing(m_BackgroundImage->GetSpacing());
			m_LabelImage->SetOrigin(m_BackgroundImage->GetOrigin());
			m_LabelImage->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
			memset((LabelImagePixelType *)(m_LabelImage->GetScalarPointer()), 0, m_LabelImage->GetScalarSize()*m_LabelImage->GetNumberOfPoints());*/

			return;
		}

		if (!this->GetImageIndexFromWorldPosition(m_WorldPosition, m_LabelImage, m_ImageIndex))
		{
			return;
		}

		//std::cout << "-----------are we here---------------painter ?" << std::endl;

		int DIMS[3];
		double SPACING[3];
		m_LabelImage->GetDimensions(DIMS);
		m_LabelImage->GetSpacing(SPACING);
		LabelImagePixelType *pPixel = static_cast<LabelImagePixelType*>(m_LabelImage->GetScalarPointer());
		const int numInplaneElements = DIMS[0] * DIMS[1];//Caution only works for one component image data.

		//Begin drawing
		int oriCoeffitients[3];
		switch (this->m_PainterSliceOrientation)
		{
		case 0://SLICE_ORIENTATION_YZ
			oriCoeffitients[0] = 0;
			oriCoeffitients[1] = 1;
			oriCoeffitients[2] = 1;
			break;
		case 1://SLICE_ORIENTATION_XZ
			oriCoeffitients[0] = 1;
			oriCoeffitients[1] = 0;
			oriCoeffitients[2] = 1;
			break;
		case 2://SLICE_ORIENTATION_XY
			oriCoeffitients[0] = 1;
			oriCoeffitients[1] = 1;
			oriCoeffitients[2] = 0;
			break;
		default:
			break;
		}

		//std::cout << "m_imageindex = " << m_ImageIndex[0] << ", " << m_ImageIndex[1] << ", " << m_ImageIndex[2] << std::endl;


		switch (this->m_PainterShape)
		{
		case 0://Circle			
		{
			for (int i = m_ImageIndex[0] - oriCoeffitients[0] * m_PainterRadius; i <= m_ImageIndex[0] + oriCoeffitients[0] * m_PainterRadius; i++)
				for (int j = m_ImageIndex[1] - oriCoeffitients[1] * m_PainterRadius; j <= m_ImageIndex[1] + oriCoeffitients[1] * m_PainterRadius; j++)
					for (int k = m_ImageIndex[2] - oriCoeffitients[2] * m_PainterRadius; k <= m_ImageIndex[2] + oriCoeffitients[2] * m_PainterRadius; k++)
					{
						if ((i < DIMS[0]) && (i >= 0))
						{
							if ((j < DIMS[1]) && (j >= 0))
							{
								if ((k < DIMS[2]) && (k >= 0))
								{
									double distance = sqrt(1.0*(i - m_ImageIndex[0]) * (i - m_ImageIndex[0]) + 1.0*(j - m_ImageIndex[1])*(j - m_ImageIndex[1]) + 1.0*(k - m_ImageIndex[2])*(k - m_ImageIndex[2]));
									if (distance < m_PainterRadius)
									{
										//LabelImagePixelType *pVal = static_cast<LabelImagePixelType*>(m_LabelImage->GetScalarPointer(i, j, k));
										//*pVal = LabelImagePixelType(m_PainterValue);
										pPixel[i + j*DIMS[0] + k*numInplaneElements] = m_PainterValue;
										//std::cout << "i, j , k = " << i << ", " << j << ", " << k << std::endl;
									}
								}
							}
						}
					}			
		}
		break;
		case 1://Square
			for (int i = m_ImageIndex[0] - m_PainterRadius; i <= m_ImageIndex[0] + m_PainterRadius; i++)
				for (int j = m_ImageIndex[1] - m_PainterRadius; j <= m_ImageIndex[1] + m_PainterRadius; j++)
					for (int k = m_ImageIndex[2] - m_PainterRadius; k <= m_ImageIndex[2] + m_PainterRadius; k++)
					{
						//int k = m_ImageIndex[2];
						if (i < DIMS[0] && i >= 0)
						{
							if (j < DIMS[1] && j >= 0)
							{
								if ((k < DIMS[2]) && (k >= 0))
								{
									/*LabelImagePixelType *pVal = static_cast<LabelImagePixelType*>(m_LabelImage->GetScalarPointer(i, j, k));
									*pVal = LabelImagePixelType(m_PainterValue);*/
									pPixel[i + j*DIMS[0] + k*numInplaneElements] = m_PainterValue;
									//std::cout << "i, j , k = " << i << ", " << j << ", " << k << std::endl;
								}
							}
						}
					}
			break;
		case 2://Sphere
			for (int i = m_ImageIndex[0] - m_PainterRadius; i <= m_ImageIndex[0] + m_PainterRadius; i++)
				for (int j = m_ImageIndex[1] - m_PainterRadius; j <= m_ImageIndex[1] + m_PainterRadius; j++)
					for (int k = m_ImageIndex[2] - m_PainterRadius; k <= m_ImageIndex[2] + m_PainterRadius; k++)
					{
						if ((i < DIMS[0]) && (i >= 0))
						{
							if ((j < DIMS[1]) && (j >= 0))
							{
								if ((k < DIMS[2]) && (k >= 0))
								{
									double distance = sqrt(1.0*(i - m_ImageIndex[0]) * (i - m_ImageIndex[0]) + 1.0*(j - m_ImageIndex[1])*(j - m_ImageIndex[1]) + 1.0*(k - m_ImageIndex[2])*(k - m_ImageIndex[2]));
									if (distance < m_PainterRadius)
									{
										LabelImagePixelType *pVal = static_cast<LabelImagePixelType*>(m_LabelImage->GetScalarPointer(i, j, k));
										*pVal = LabelImagePixelType(m_PainterValue);
									}
								}
							}
						}
					}
			break;
			case 3://Circle in mm		
			{
				int xRange[2] = { std::round(m_ImageIndex[0] - oriCoeffitients[0] * m_PainterRadius / SPACING[0]), std::round(m_ImageIndex[0] + oriCoeffitients[0] * m_PainterRadius / SPACING[0]) };
				int yRange[2] = { std::round(m_ImageIndex[1] - oriCoeffitients[1] * m_PainterRadius / SPACING[1]), std::round(m_ImageIndex[1] + oriCoeffitients[1] * m_PainterRadius / SPACING[1]) };
				int zRange[2] = { std::round(m_ImageIndex[2] - oriCoeffitients[2] * m_PainterRadius / SPACING[2]), std::round(m_ImageIndex[2] + oriCoeffitients[2] * m_PainterRadius / SPACING[2]) };

				for (int i = xRange[0]; i <= xRange[1]; i++)
					for (int j = yRange[0]; j <= yRange[1]; j++)
						for (int k = zRange[0]; k <= zRange[1]; k++)
						{
							if ((i < DIMS[0]) && (i >= 0))
							{
								if ((j < DIMS[1]) && (j >= 0))
								{
									if ((k < DIMS[2]) && (k >= 0))
									{
										double distance = sqrt(1.0*(i - m_ImageIndex[0])*SPACING[0]*SPACING[0] * (i - m_ImageIndex[0]) + 1.0*(j - m_ImageIndex[1])*(j - m_ImageIndex[1]) *SPACING[1]*SPACING[1]+ 1.0*(k - m_ImageIndex[2])*(k - m_ImageIndex[2])*SPACING[2]*SPACING[2]);
										if (distance < m_PainterRadius)
										{
											//LabelImagePixelType *pVal = static_cast<LabelImagePixelType*>(m_LabelImage->GetScalarPointer(i, j, k));
											//*pVal = LabelImagePixelType(m_PainterValue);
											pPixel[i + j*DIMS[0] + k*numInplaneElements] = m_PainterValue;
											//std::cout << "i, j , k = " << i << ", " << j << ", " << k << std::endl;
										}
									}
								}
							}
						}
			}
			break;
		default:
			break;
		}

		this->m_LabelImage->Modified();
	}

	void PaintPixels()
	{

	}

	vtkImageData * GetLabelImage()
	{
		return m_LabelImage;
	}

	int GetPainterSliceOrientation()
	{
		return m_PainterSliceOrientation;
	}

	double *GetWorldPostion()
	{
		return m_WorldPosition;
	}
	int *GetImageIndex()
	{
		return m_ImageIndex;
	}


protected:
	int m_PainterRadius;
	LabelImagePixelType m_PainterValue;
	vtkImageData* m_LabelImage;//do not hold label image
	vtkImageData *m_BackgroundImage;//do not hold background image
	double m_WorldPosition[3];
	int m_ImageIndex[3];
	int m_PainterShape;
	int m_PainterSliceOrientation;
};

#endif
