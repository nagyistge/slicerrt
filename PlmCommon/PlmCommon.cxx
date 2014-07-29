/*==========================================================================

  Copyright (c) Massachusetts General Hospital, Boston, MA, USA. All Rights Reserved.
 
  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Gregory C. Sharp, Massachusetts General Hospital
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Natural Sciences and Engineering Research Council
  of Canada.

==========================================================================*/

#include "PlmCommon.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkImageData.h>

// SlicerRT includes
#include "SlicerRtCommon.h"

//----------------------------------------------------------------------------
// Utility functions
//----------------------------------------------------------------------------
template<class T> 
static typename itk::Image<T,3>::Pointer
convert_to_itk (vtkMRMLScalarVolumeNode* inVolumeNode, bool applyWorldTransform)
{
  typename itk::Image<T,3>::Pointer image = itk::Image<T,3>::New ();
  SlicerRtCommon::ConvertVolumeNodeToItkImage<T>(
    inVolumeNode, image, applyWorldTransform, true);
  return image;
}

Plm_image::Pointer 
PlmCommon::ConvertVolumeNodeToPlmImage(vtkMRMLScalarVolumeNode* inVolumeNode, bool applyWorldTransform)
{
  Plm_image::Pointer image = Plm_image::New ();

  if (!inVolumeNode || !inVolumeNode->GetImageData())
  {
    std::cerr << "PlmCommon::ConvertVolumeNodeToPlmImage: Invalid input volume node!";
    return image;
  }

  vtkImageData* inVolume = inVolumeNode->GetImageData();
  int vtk_type = inVolume->GetScalarType ();

  switch (vtk_type) {
  case VTK_CHAR:
  case VTK_SIGNED_CHAR:
    image->set_itk (convert_to_itk<char> (inVolumeNode, applyWorldTransform));
    break;
  
  case VTK_UNSIGNED_CHAR:
    image->set_itk (convert_to_itk<unsigned char> (inVolumeNode, applyWorldTransform));
    break;
  
  case VTK_SHORT:
    image->set_itk (convert_to_itk<short> (inVolumeNode, applyWorldTransform));
    break;
  
  case VTK_UNSIGNED_SHORT:
    image->set_itk (convert_to_itk<unsigned short> (inVolumeNode, applyWorldTransform));
    break;
  
#if (CMAKE_SIZEOF_UINT == 4)
  case VTK_INT:
  case VTK_LONG: 
    image->set_itk (convert_to_itk<int> (inVolumeNode, applyWorldTransform));
    break;
  
  case VTK_UNSIGNED_INT:
  case VTK_UNSIGNED_LONG:
    image->set_itk (convert_to_itk<unsigned int> (inVolumeNode, applyWorldTransform));
    break;
#else
  case VTK_INT:
  case VTK_LONG: 
    image->set_itk (convert_to_itk<long> (inVolumeNode, applyWorldTransform));
    break;
  
  case VTK_UNSIGNED_INT:
  case VTK_UNSIGNED_LONG:
    image->set_itk (convert_to_itk<unsigned long> (inVolumeNode, applyWorldTransform));
    break;
#endif
  
  case VTK_FLOAT:
    image->set_itk (convert_to_itk<float> (inVolumeNode, applyWorldTransform));
    break;
  
  case VTK_DOUBLE:
    image->set_itk (convert_to_itk<double> (inVolumeNode, applyWorldTransform));
    break;

  default:
    vtkWarningWithObjectMacro (inVolumeNode, "Got something else\n");
    break;
  }

  return image;
}

Plm_image::Pointer 
PlmCommon::ConvertVolumeNodeToPlmImage(vtkMRMLNode* inNode, bool applyWorldTransform)
{
  return PlmCommon::ConvertVolumeNodeToPlmImage(
    vtkMRMLScalarVolumeNode::SafeDownCast(inNode), applyWorldTransform);
}
