/*!
 * The VTK_MODULE_INIT is definitely required. Without it NULL is returned to
 * ::New() type calls
 */

#ifndef VTKINCLUDES_H
#define VTKINCLUDES_H

#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2);   //! VTK was built with vtkRenderingOpenGL2
VTK_MODULE_INIT(vtkRenderingFreeType);  //!
VTK_MODULE_INIT(vtkInteractionStyle);   //!
#include <cmath>
#include <unordered_map>
#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkAppendPolyData.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkColorTransferFunction.h>
#include <vtkCommand.h>
#include <vtkConeSource.h>
#include <vtkContourFilter.h>
#include <vtkCoordinate.h>
#include <vtkCubeSource.h>
#include <vtkCylinderSource.h>
#include <vtkDataSetMapper.h>
#include <vtkDelaunay3D.h>
#include <vtkFloatArray.h>
#include <vtkGeometryFilter.h>
#include <vtkGlyph3D.h>
#include <vtkImageData.h>
#include <vtkJPEGWriter.h>
#include <vtkLine.h>
#include <vtkLineSource.h>
#include <vtkMath.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkObject.h>
#include <vtkOutputWindow.h>
#include <vtkParametricFunctionSource.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkPolygon.h>
#include <vtkProgrammableSource.h>
#include <vtkProperty.h>
#include <vtkProperty2D.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkReverseSense.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkSurfaceReconstructionFilter.h>
#include <vtkTextActor.h>
#include <vtkTextMapper.h>
#include <vtkTextProperty.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTubeFilter.h>
#include <vtkUnsignedCharArray.h>
#include <vtkVersion.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkWarpTo.h>
#include <vtkWindowToImageFilter.h>
#include <vtkXMLPolyDataWriter.h>

#endif