/*=========================================================================

  Program: DICOM for VTK

  Copyright (c) 2012-2015 David Gobbi
  All rights reserved.
  See Copyright.txt or http://dgobbi.github.io/bsd3.txt for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDICOMVUSIONGenerator.h"
//#include "vtkDICOMMetaData.h"
#include "vtkObjectFactory.h"
//#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkDICOMMetaDataAdapter.h"
#include "vtkDICOMMetaData.h"
#include "vtkIntArray.h"
#include "vtkImageData.h"

#include "vtkDICOMDictionary.h"

vtkStandardNewMacro(vtkDICOMVUSIONGenerator);

//----------------------------------------------------------------------------
vtkDICOMVUSIONGenerator::vtkDICOMVUSIONGenerator()
{
	m_UseSourceInstansePrivateTag = false;
	m_AnonymizingDicom = false;
	m_SoureImageData = NULL;
}

//----------------------------------------------------------------------------
vtkDICOMVUSIONGenerator::~vtkDICOMVUSIONGenerator()
{
}

bool vtkDICOMVUSIONGenerator::GenerateInstance(vtkInformation *info)
{
	if (this->MultiFrame)
	{
		vtkErrorMacro("Enhanced Multi-Frame MR is not yet supported.");
		return false;
	}

	return ((this->GenerateMRInstance(info) && (this->GenerateVUSIONModule(this->SourceMetaData))));
}

bool vtkDICOMVUSIONGenerator::GenerateVUSIONModule(vtkDICOMMetaData *source) {

	//Copy all source meta data tags: private and public
	this->CopySourceInstanceTags(source);

	if (m_AnonymizingDicom)
	{
		this->AnonymizingDicomTag();
	}

	if (m_SoureImageData && m_DiffusionBValue.size() > 0)
	{
		this->WriteDiffusionBValueTag();
	}

	if (m_SoureImageData && m_GradientDirection.size() > 0)
	{
		this->WriteDiffusionGradientOrientationTag();
	}
	////How to write some useful tags
	//metaData->SetAttributeValue(DC::StudyTime, std::to_string(resclaeSlope))
	//metaData->SetAttributeValue(m_DicomHelper->ScaleSlop, vtkDICOMValue(vtkDICOMVR::DS, 1.1));

	////mandatory tags required by Vusion
	//vtkDICOMMetaData *meta = this->MetaData;
	//meta->SetAttributeValue(DC::Modality, "MR");

	////copy Vusion specific tags
	static const DC::EnumType required[] = {
		//DC::SeriesDescription, // 1C
		//DC::ProtocolName,
		//DC::Manufacturer,
		DC::RealWorldValueIntercept,
		DC::RealWorldValueSlope,
		//DC::ImageOrientationPatient,//somehow the generator computed image orientation is wrong, find out later
		//DC::DiffusionModelCodeSequence,
		//DC::DiffusionDirectionality,
		//DC::DiffusionBValue,
		//DC::DiffusionGradientOrientation,
		DC::ItemDelimitationItem
	};

	return this->CopyRequiredAttributes(required, source);
}

void vtkDICOMVUSIONGenerator::CopySourceInstanceTags(vtkDICOMMetaData *sourceMetaData)
{
	if (!sourceMetaData) return;

	vtkDICOMMetaData *meta = this->MetaData;

	//enhanced dicom meta data
	if (sourceMetaData->HasAttribute(DC::PerFrameFunctionalGroupsSequence))
	{
		std::cout << "Enhanced Multi-Frame MR Meta Data is not copied."<< std::endl;
		return;
	}
	//classic dicom metadata
	else
	{
		vtkDICOMDictEntry entry;

		for (vtkDICOMDataElementIterator iter = sourceMetaData->Begin(); iter != sourceMetaData->End(); ++iter)
		{
			vtkDICOMTag tag = iter->GetTag();

			//UID specific tag, do not copy
			if (tag == vtkDICOMTag(0x0002, 0x0002) || tag == vtkDICOMTag(0x0002, 0x0003) ||
				tag == vtkDICOMTag(0x0002, 0x0010) || tag == vtkDICOMTag(0x0002, 0x0012) ||
				tag == vtkDICOMTag(0x0008, 0x0016) || tag == vtkDICOMTag(0x0008, 0x0018) ||
				tag == vtkDICOMTag(0x0008, 0x1150) || tag == vtkDICOMTag(0x0008, 0x1155) ||
				tag == vtkDICOMTag(0x0020, 0x000D) || tag == vtkDICOMTag(0x0020, 0x000E) ||
				tag == vtkDICOMTag(0x0020, 0x0052) || tag == vtkDICOMTag(0x0020, 0x000E)
				)
			{
				continue;
			}

			if (!iter->IsPerInstance())
			{
				meta->SetAttributeValue(tag, vtkDICOMValue(iter->GetVR(), iter->GetValue().AsString()));//Set or insert a tag					
			}
			else 
			{
				bool isPrivateTag = false;
				entry = vtkDICOMDictionary::FindDictEntry(tag);
				//check if entry was found in dictionary
				if (!entry.IsValid())
				{
					isPrivateTag = true;
				}

				if (m_UseSourceInstansePrivateTag && isPrivateTag)
				{
					if (sourceMetaData->GetFileIndexArray())
					{
						vtkIntArray * fileMap = sourceMetaData->GetFileIndexArray();
						int components = fileMap->GetNumberOfComponents();
						int slices = fileMap->GetNumberOfTuples();
						int cnt = 0;
						for (int sliceIndex = 0; sliceIndex < slices; sliceIndex++)
						{
							for (int componentIndex = 0; componentIndex < components; componentIndex++)
							{
								int fileIndex = fileMap->GetComponent(sliceIndex, componentIndex);
								meta->SetAttributeValue(cnt, tag, vtkDICOMValue(iter->GetVR(), iter->GetValue(fileIndex).AsString()));
								cnt++;
							}
						}
					}
				}
			}
		}
	}
}

void vtkDICOMVUSIONGenerator::AnonymizingDicomTag()
{
	vtkDICOMMetaData *meta = this->MetaData;

	std::string seriesName = "VusionDemoData";

	//meta->SetAttributeValue(DC::SeriesDescription, seriesName);
	//meta->SetAttributeValue(DC::ProtocolName, seriesName);
	//meta->SetAttributeValue(DC::Manufacturer, seriesName); //mannually overwrite in soure meta data if needed
	meta->SetAttributeValue(DC::PatientBirthName, seriesName);
	meta->SetAttributeValue(DC::InstitutionName, seriesName);//Hospital
	meta->SetAttributeValue(DC::InstitutionAddress, seriesName);//Hospital
	meta->SetAttributeValue(DC::Modality, seriesName);
	meta->SetAttributeValue(vtkDICOMTag(0x0010, 0x0010), seriesName);//Patient Name
	meta->SetAttributeValue(DC::PatientAge, seriesName);
	meta->SetAttributeValue(DC::PatientBirthDate, seriesName);
	meta->SetAttributeValue(vtkDICOMTag(0x0010, 0x0040), seriesName);//Patient Gender
	meta->SetAttributeValue(vtkDICOMTag(0x0010, 0x1030), seriesName);//Patient Weight
	meta->SetAttributeValue(vtkDICOMTag(0x0002, 0x0013), seriesName);//implementation version name 
}

void vtkDICOMVUSIONGenerator::SetDiffusionBValueTag(vtkImageData * imageData,const std::vector<float> & diffusionBValueList)
{
	//Fang dai first
	if (this->MultiFrame)
	{
		std::cout << "Enhanced Multi-Frame MR Meta Data is not copied." << std::endl;
		return;
	}

	if (!imageData) return;
	if (diffusionBValueList.size() < 1) return;

	if (imageData->GetNumberOfScalarComponents() != diffusionBValueList.size()) return;

	m_SoureImageData = imageData;
	m_DiffusionBValue = diffusionBValueList;
	//vtkDICOMMetaData *meta = this->MetaData;

	//////check if file index array exists
	////if (!sourceMetaData->GetFileIndexArray())
	////{
	////	sourceMetaData->SetNumberOfInstances(imageDataComponents*imageDataSlices);

	////	vtkIntArray * fileArray = vtkIntArray::New();
	////	fileArray->SetNumberOfComponents(imageDataComponents);
	////	fileArray->SetNumberOfTuples(imageDataSlices);
	////	for (int i = 0; i < imageDataSlices; i++)
	////	{
	////		for (int j = 0; j < imageDataComponents; j++)
	////		{
	////			fileArray->SetComponent(i, j, i*imageDataComponents + j);
	////		}
	////	}
	////	sourceMetaData->SetFileIndexArray(fileArray);
	////	fileArray->Delete();
	////}

	////for (int sliceIndex = 0; sliceIndex < imageDataSlices; sliceIndex++)
	////{
	////	vtkIntArray * fileMap = sourceMetaData->GetFileIndexArray();
	////	for (int componentIndex = 0; componentIndex < imageDataComponents; componentIndex++)
	////	{
	////		int fileIndex = fileMap->GetComponent(sliceIndex, componentIndex);
	////		sourceMetaData->SetAttributeValue(fileIndex, DC::DiffusionBValue, vtkDICOMValue(vtkDICOMVR::FD, diffusionBValueList.at(componentIndex)));
	////		//std::cout << "After set attribute, file index= " << fileIndex << std::endl;
	////	}
	////}
}

void vtkDICOMVUSIONGenerator::WriteDiffusionBValueTag()
{
	//Fang dai has been handled in setdiffusionBValueTag function
	vtkDICOMMetaData *meta = this->MetaData;

	int imageDataSlices = m_SoureImageData->GetDimensions()[2];
	int imageDataComponents = m_SoureImageData->GetNumberOfScalarComponents();

	//diffusion tag as non per instance
	if (imageDataComponents == 1)
	{
		meta->SetAttributeValue(DC::DiffusionBValue, vtkDICOMValue(vtkDICOMVR::FD, m_DiffusionBValue.at(imageDataComponents - 1)));
		return;
	}

	int cnt = 0;
	for (int sliceIndex = 0; sliceIndex < imageDataSlices; sliceIndex++)
	{
		for (int componentIndex = 0; componentIndex < imageDataComponents; componentIndex++)
		{			
			meta->SetAttributeValue(cnt, DC::DiffusionBValue, vtkDICOMValue(vtkDICOMVR::FD, m_DiffusionBValue.at(componentIndex)));
			cnt++;
			//std::cout << "writing b value tag, cnt = " << cnt  << std::endl;
		}
	}
}

void vtkDICOMVUSIONGenerator::SetDiffusionGradientOrientationTag(vtkImageData * imageData, const std::vector<std::array<float, 3>> & gradientDirection)
{
	//Fang dai first
	if (this->MultiFrame)
	{
		std::cout << "Enhanced Multi-Frame MR Meta Data is not copied." << std::endl;
		return;
	}

	if (!imageData) return;

	if (gradientDirection.size() < 1) return;

	if (imageData->GetNumberOfScalarComponents() != gradientDirection.size()) return;

	//if m_SoureImageData hasn't been set yet, set it from here
	//else use prior settings, namely, bValue.
	if (!m_SoureImageData)
	{
		m_SoureImageData = imageData;
	}

	m_GradientDirection = gradientDirection;

	//	if (imageDataSlices < 1 || imageDataComponents < 1) return;
	//
	//	if (sourceMetaData->HasAttribute(DC::PerFrameFunctionalGroupsSequence))
	//	{
	//		std::cout << "Enhanced dicom not supported yet, please use vtkDICOMMetaDataAdapter to convert from enhanced dicom to classic dicom first" << std::endl;
	//		return;
	//	}
	//
	//	if (imageDataComponents == 1)
	//	{
	//		float orientation[3]; 
	//		orientation[0] = gradientDirection.at(0).at(0);
	//		orientation[1] = gradientDirection.at(0).at(1);
	//		orientation[2] = gradientDirection.at(0).at(2);
	//		//orientation = gradientDirection.at(0);
	//		sourceMetaData->SetAttributeValue(DC::DiffusionGradientOrientation, vtkDICOMValue(vtkDICOMVR::FD, orientation, 3));
	//		return;
	//	}
	//
	//	//check if file index array exists
	//	if (!sourceMetaData->GetFileIndexArray())
	//	{
	//		sourceMetaData->SetNumberOfInstances(imageDataComponents*imageDataSlices);
	//
	//		vtkIntArray * fileArray = vtkIntArray::New();
	//		fileArray->SetNumberOfComponents(imageDataComponents);
	//		fileArray->SetNumberOfTuples(imageDataSlices);
	//		for (int i = 0; i < imageDataSlices; i++)
	//		{
	//			for (int j = 0; j < imageDataComponents; j++)
	//			{
	//				fileArray->SetComponent(i, j, i*imageDataComponents + j);
	//			}
	//		}
	//		sourceMetaData->SetFileIndexArray(fileArray);
	//		fileArray->Delete();
	//	}
	//
	//	//Set tag
	//	for (int sliceIndex = 0; sliceIndex < imageDataSlices; sliceIndex++)
	//	{
	//		vtkIntArray * fileMap = sourceMetaData->GetFileIndexArray();
	//		for (int componentIndex = 0; componentIndex < imageDataComponents; componentIndex++)
	//		{
	//			float orientation[3];
	//			orientation[0] = gradientDirection.at(componentIndex).at(0);
	//			orientation[1] = gradientDirection.at(componentIndex).at(1);
	//			orientation[2] = gradientDirection.at(componentIndex).at(2);
	//			int fileIndex = fileMap->GetComponent(sliceIndex, componentIndex);
	//			sourceMetaData->SetAttributeValue(fileIndex, DC::DiffusionGradientOrientation, vtkDICOMValue(vtkDICOMVR::FD, orientation,3));
	//		}
	//	}
}

void vtkDICOMVUSIONGenerator::WriteDiffusionGradientOrientationTag()
{
	//Fang dai has been handled in SetdiffusionGradientOrientationTag function
	vtkDICOMMetaData *meta = this->MetaData;

	int imageDataSlices = m_SoureImageData->GetDimensions()[2];
	int imageDataComponents = m_SoureImageData->GetNumberOfScalarComponents();

	//diffusion tag as non per instance
	if (imageDataComponents == 1)
	{
		float orientation[3]; 
		orientation[0] = m_GradientDirection.at(0).at(0);
		orientation[1] = m_GradientDirection.at(0).at(1);
		orientation[2] = m_GradientDirection.at(0).at(2);
		meta->SetAttributeValue(DC::DiffusionGradientOrientation, vtkDICOMValue(vtkDICOMVR::FD, orientation, 3));
		return;
	}

	//Set multi-component tag
	int cnt = 0;
	for (int sliceIndex = 0; sliceIndex < imageDataSlices; sliceIndex++)
	{
		for (int componentIndex = 0; componentIndex < imageDataComponents; componentIndex++)
		{
			float orientation[3];
			orientation[0] = m_GradientDirection.at(componentIndex).at(0);
			orientation[1] = m_GradientDirection.at(componentIndex).at(1);
			orientation[2] = m_GradientDirection.at(componentIndex).at(2);
			meta->SetAttributeValue(cnt, DC::DiffusionGradientOrientation, vtkDICOMValue(vtkDICOMVR::FD, orientation,3));
			cnt++;
		}
	}
}
//void vtkDICOMVUSIONGenerator::CustomDiffusionBValueMetaData(vtkDICOMMetaData *sourceMetaData, int imageDataSlices, int imageDataComponents, const std::vector<float> & diffusionBValueList)
//{
//	if (imageDataSlices < 1 || imageDataComponents < 1) return;
//	if (sourceMetaData->HasAttribute(DC::PerFrameFunctionalGroupsSequence))
//	{
//		std::cout << "Enhanced dicom not supported yet, please use vtkDICOMMetaDataAdapter to convert from enhanced dicom to classic dicom first" << std::endl;
//		return;
//	}
//	
//	//diffusion tag as non per instance
//	if (imageDataComponents == 1) 
//	{
//		sourceMetaData->SetAttributeValue(DC::DiffusionBValue, vtkDICOMValue(vtkDICOMVR::FD, diffusionBValueList.at(imageDataComponents - 1)));
//		return;
//	}
//
//	//check if file index array exists
//	if (!sourceMetaData->GetFileIndexArray())
//	{
//		sourceMetaData->SetNumberOfInstances(imageDataComponents*imageDataSlices);
//
//		vtkIntArray * fileArray = vtkIntArray::New();
//		fileArray->SetNumberOfComponents(imageDataComponents);
//		fileArray->SetNumberOfTuples(imageDataSlices);
//		for (int i = 0; i < imageDataSlices; i++)
//		{
//			for (int j = 0; j < imageDataComponents; j++)
//			{
//				fileArray->SetComponent(i, j, i*imageDataComponents + j);
//			}
//		}
//		sourceMetaData->SetFileIndexArray(fileArray);
//		fileArray->Delete();
//	}
//
//	for (int sliceIndex = 0; sliceIndex < imageDataSlices; sliceIndex++)
//	{
//		vtkIntArray * fileMap = sourceMetaData->GetFileIndexArray();
//		for (int componentIndex = 0; componentIndex < imageDataComponents; componentIndex++)
//		{
//			int fileIndex = fileMap->GetComponent(sliceIndex, componentIndex);
//			sourceMetaData->SetAttributeValue(fileIndex, DC::DiffusionBValue, vtkDICOMValue(vtkDICOMVR::FD, diffusionBValueList.at(componentIndex)));
//			//std::cout << "After set attribute, file index= " << fileIndex << std::endl;
//		}
//	}
//
//	//debug use only
//	bool printBValue = false;
//	if (printBValue)
//	{
//		for (vtkDICOMDataElementIterator iter = sourceMetaData->Begin(); iter != sourceMetaData->End(); ++iter)
//		{
//			vtkDICOMTag tag = iter->GetTag();
//			//std::cout << " tag : " << tag << std::endl;
//			// Crucial step: check for values that vary across the series.
//			if (iter->IsPerInstance())
//			{
//				//if (tag == vtkDICOMTag(0x0018, 0x9087))//DC::DiffusionBValue
//				if (tag == vtkDICOMTag(DC::DiffusionBValue))//DC::DiffusionBValue
//				{
//					int n = iter->GetNumberOfInstances();
//					std::cout << "Printing tag: " << tag << std::endl;
//					for (int i = 0; i < n; i++)
//					{
//						std::cout << " instance " << i << ": " << iter->GetValue(i) << std::endl;
//					}
//				}
//			}
//			//else
//			//{
//			//	// Not PerInstance: value is the same for all files in series.
//			//	std::cout << " all instances : " << iter->GetValue() << std::endl;
//			//}
//		}
//	}
//}
//
//void  vtkDICOMVUSIONGenerator::CustomDiffusionGradientOrientationMetaData(vtkDICOMMetaData *sourceMetaData, int imageDataSlices, int imageDataComponents, const std::vector<std::array<float, 3>> & gradientDirection)
//{
//	if (imageDataSlices < 1 || imageDataComponents < 1) return;
//
//	if (sourceMetaData->HasAttribute(DC::PerFrameFunctionalGroupsSequence))
//	{
//		std::cout << "Enhanced dicom not supported yet, please use vtkDICOMMetaDataAdapter to convert from enhanced dicom to classic dicom first" << std::endl;
//		return;
//	}
//
//	if (imageDataComponents == 1)
//	{
//		float orientation[3]; 
//		orientation[0] = gradientDirection.at(0).at(0);
//		orientation[1] = gradientDirection.at(0).at(1);
//		orientation[2] = gradientDirection.at(0).at(2);
//		//orientation = gradientDirection.at(0);
//		sourceMetaData->SetAttributeValue(DC::DiffusionGradientOrientation, vtkDICOMValue(vtkDICOMVR::FD, orientation, 3));
//		return;
//	}
//
//	//check if file index array exists
//	if (!sourceMetaData->GetFileIndexArray())
//	{
//		sourceMetaData->SetNumberOfInstances(imageDataComponents*imageDataSlices);
//
//		vtkIntArray * fileArray = vtkIntArray::New();
//		fileArray->SetNumberOfComponents(imageDataComponents);
//		fileArray->SetNumberOfTuples(imageDataSlices);
//		for (int i = 0; i < imageDataSlices; i++)
//		{
//			for (int j = 0; j < imageDataComponents; j++)
//			{
//				fileArray->SetComponent(i, j, i*imageDataComponents + j);
//			}
//		}
//		sourceMetaData->SetFileIndexArray(fileArray);
//		fileArray->Delete();
//	}
//
//	//Set tag
//	for (int sliceIndex = 0; sliceIndex < imageDataSlices; sliceIndex++)
//	{
//		vtkIntArray * fileMap = sourceMetaData->GetFileIndexArray();
//		for (int componentIndex = 0; componentIndex < imageDataComponents; componentIndex++)
//		{
//			float orientation[3];
//			orientation[0] = gradientDirection.at(componentIndex).at(0);
//			orientation[1] = gradientDirection.at(componentIndex).at(1);
//			orientation[2] = gradientDirection.at(componentIndex).at(2);
//			int fileIndex = fileMap->GetComponent(sliceIndex, componentIndex);
//			sourceMetaData->SetAttributeValue(fileIndex, DC::DiffusionGradientOrientation, vtkDICOMValue(vtkDICOMVR::FD, orientation,3));
//		}
//	}
//}

//----------------------------------------------------------------------------
void vtkDICOMVUSIONGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


//How to iterate metaData and get value
//iter->GetNumberOfInstances();
////iter->GetValue()
//iter->GetVR();
//std::cout << "Printing tag = " << tag << std::endl;
//std::cout << "			GetNumberOfInstances() = " << iter->GetNumberOfInstances() << "--" << std::endl;
//std::cout << "			isPerInstance = " << iter->IsPerInstance() << "--" << std::endl;
//std::cout << "			GetVR() = " << iter->GetVR() << "--" << std::endl;
//std::cout << "			GetValue().AsString() = " << iter->GetValue().AsString() << "--" << std::endl;
//if (iter->GetNumberOfInstances() > 1)
//{
//	std::cout << "			GetValue().GetDouble(0) = " << iter->GetValue().GetDouble(0) << std::endl;
//	//std::cout << "			GetValue().GetDouble(0) = " << vtkDICOMValue(vtkDICOMVR::FD, orientation, iter->GetNumberOfInstances()) << std::endl;
//}
//else if(iter->GetNumberOfInstances() == 1)//bug for some tag. e.g ivim data 0008,1111
//{
//	if (!iter->IsPerInstance())
//	{
//		std::cout << "			GetValue() = " << iter->GetValue().AsString() << std::endl;
//	}
//	else
//	{
//		std::cout << "			GetValue() = " << iter->GetValue(0) << std::endl;
//	}
//}
//else
//{
//	std::cout << "			number of instances = 0 "<< std::endl;
//}
