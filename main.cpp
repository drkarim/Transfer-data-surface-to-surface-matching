// STL
#include <iostream>
#include <string>

// VTK
#include <vtkXMLPolyDataWriter.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkMatrix4x4.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTransform.h>
#include <vtkLandmarkTransform.h>
#include <vtkIterativeClosestPointTransform.h>
#include <vtkPolyDataWriter.h>
#include <vtkPolyDataReader.h>
#include <vtkPointLocator.h>
#include <vtkFloatArray.h>
#include <vtkGenericCell.h>
#include <vtkPointData.h>
#include <vtkFileOutputWindow.h>

using namespace std;

// Prints the stats for how many points were plotted 
void PrintHitFrequency(int* hit_frequency, int length)
{
	ofstream out; 
	out.open("hit_frequency.txt"); 
	out << "Point ID\t\tFrequency" << endl; 
	out << "========\t\t=========" << endl;
	for (int pointID=0;pointID<length;pointID++)
	{
		if (hit_frequency[pointID] > 0)
			out << pointID << "\t\t" << hit_frequency[pointID] << endl;
	}

	out.close(); 
}



/*
*	poly_s - polydata with scalars
*	poly_ns - polydata with no scalars
*	poly_o - polydata output
*/
void TransferScalars(vtkPolyData* poly_s, vtkPolyData* poly_ns, bool isDirectCopy) {
	
	double scalar; 
	double xyz[3];
	int closestPointID=-1;
	vtkSmartPointer<vtkPointLocator> point_locator = vtkSmartPointer<vtkPointLocator>::New();
	point_locator->SetDataSet(poly_s); 
	point_locator->AutomaticOn(); 
	point_locator->BuildLocator();
	poly_s->BuildLinks();
	
	//vtkSmartPointer<vtkFloatArray> poly_s_scalars = vtkFloatArray::SafeDownCast(poly_s->GetPointData()->GetScalars());
	vtkSmartPointer<vtkFloatArray> poly_ns_scalars = vtkSmartPointer<vtkFloatArray>::New(); 

	for (int i=0;i<poly_s->GetNumberOfPoints();i++){
		poly_ns_scalars->InsertNextTuple1(-1); 
	}

	int* hit_frequency = new int[poly_s->GetNumberOfPoints()];

	// record hit frequency 
	for (int i=0;i<poly_s->GetNumberOfPoints();i++)
	{
		hit_frequency[i] = 0; 
	}

	//cout << "Finished reading scalars, now transferring .. " << endl;
	for (int i=0;i<poly_ns->GetNumberOfPoints();i++) 
	{	
		poly_ns->GetPoint(i, xyz);

		

		if (!isDirectCopy) {
			closestPointID = point_locator->FindClosestPoint(xyz); 
			
			if (closestPointID != -1) {
				scalar = poly_s->GetPointData()->GetScalars()->GetTuple1(closestPointID);
				hit_frequency[closestPointID]++; 

				poly_ns_scalars->SetTuple1(i, scalar);
			}
		}
		
		else {
			scalar = poly_s->GetPointData()->GetScalars()->GetTuple1(i);
			poly_ns_scalars->SetTuple1(i, scalar); 
		}
	}

	poly_ns->GetPointData()->SetScalars(poly_ns_scalars);

	PrintHitFrequency(hit_frequency, poly_s->GetNumberOfPoints());
	
}


int main(int argc, char *argv[])
{
	bool isDirectCopy = false, foundArgs1=false;
	std::string fn1, fn2, fn3; 
	if (argc < 6)
	{
		std::cerr << "Required parameters: \n\t-i1 source \n\t-i2 target \n\t-o output \n\t--directcopy xx (optional)" << std::endl;
		return EXIT_FAILURE;
	}

	if (argc >= 1)
	{
		for (int i = 1; i < argc; i++) {
			if (i + 1 != argc) {
				if (string(argv[i]) == "-i1") {
					fn1 = argv[i + 1];
					foundArgs1 = true;
				}
				else if (string(argv[i]) == "-i2") {
					fn2 = argv[i + 1];
					foundArgs1 = true;
				}
				else if (string(argv[i]) == "-o") {
					fn3 = argv[i + 1];
					foundArgs1 = true;
				}
			}
			if (string(argv[i]) == "--directcopy") {
				isDirectCopy = true;
				
			}
		}
	}

	
	vtkSmartPointer<vtkPolyDataReader> reader1 = vtkSmartPointer<vtkPolyDataReader>::New();
	reader1->SetFileName(fn1.c_str());
	reader1->Update();
	
	vtkSmartPointer<vtkPolyDataReader> reader2 = vtkSmartPointer<vtkPolyDataReader>::New();
	reader2->SetFileName(fn2.c_str());
	reader2->Update();


	vtkSmartPointer<vtkPolyData> poly_with_scalar = vtkSmartPointer<vtkPolyData>::New(); 
	poly_with_scalar = reader1->GetOutput();

	vtkSmartPointer<vtkPolyData> poly_no_scalar = vtkSmartPointer<vtkPolyData>::New();
	poly_no_scalar = reader2->GetOutput();

	
	vtkSmartPointer<vtkFileOutputWindow> fileOutputWindow = vtkSmartPointer<vtkFileOutputWindow>::New();
	fileOutputWindow->SetFileName("vtk_error.txt");

	// Note that the SetInstance function is a static member of vtkOutputWindow.
	vtkOutputWindow::SetInstance(fileOutputWindow);

	
	TransferScalars(poly_with_scalar, poly_no_scalar, isDirectCopy);

	
	vtkSmartPointer<vtkPolyDataWriter> writer = vtkSmartPointer<vtkPolyDataWriter>::New();
	writer->SetInputData(poly_no_scalar);
	writer->SetFileName(fn3.c_str());
	writer->Update();

	return EXIT_SUCCESS;
}