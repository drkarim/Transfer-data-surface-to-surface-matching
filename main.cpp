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

	stringstream ss; 
	double points_hit = 0, points_hit_5=0, percent_hit=0; 

	out.open("hit_frequency.csv"); 
	
	for (int pointID=0;pointID<length;pointID++)
	{
		if (hit_frequency[pointID] > 0) {
			ss << pointID << "," << hit_frequency[pointID] << endl;
			points_hit++; 

			if (hit_frequency[pointID] > 5)
				points_hit_5++; 
		}
	}

	percent_hit = 100*(points_hit/(double)length); 
	
	out << "Stats\n=======\n\n" << "Percent points carried over," << percent_hit << endl; 
	out << "Points total," <<  points_hit << endl; 
	out << "Points with more than 5 hits," << points_hit_5 << endl << endl;

	out << "Point ID,Frequency" << endl; 
	out << "========,=========" << endl;
	out << ss.rdbuf() << endl;

	out.close(); 
}

void ICPRegistration(vtkPolyData* source, vtkPolyData* target, vtkPolyData* result)
{
	vtkSmartPointer<vtkIterativeClosestPointTransform> icp = vtkSmartPointer<vtkIterativeClosestPointTransform>::New();
	icp->SetSource(source);
	icp->SetTarget(target);
	icp->GetLandmarkTransform()->SetModeToAffine();
	icp->SetMaximumNumberOfIterations(1000);
	//icp->StartByMatchingCentroidsOn();
	icp->Modified();
	icp->Update();
	vtkSmartPointer<vtkMatrix4x4> m = icp->GetMatrix();
	std::cout << "The resulting matrix after ICP is: " << *m << std::endl;

	vtkSmartPointer<vtkTransformPolyDataFilter> icpFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
	icpFilter->SetInputData(source);
	icpFilter->SetTransform(icp);
	icpFilter->Update();
	result = icpFilter->GetOutput();

	// debug 
	vtkSmartPointer<vtkPolyDataWriter> writer = vtkSmartPointer<vtkPolyDataWriter>::New();
	writer->SetInputData(result);
	writer->SetFileName("post_icp.vtk");
	writer->Update();
}

/*
*	poly_s - polydata with scalars (i.e. source)
*	poly_ns - polydata with no scalars (i.e. target)
*	poly_o - polydata output
* 	copy_method = 1 (index copy), 2 (nearest neighbour), 3 (ICP method)
*/
void TransferScalars(vtkPolyData* poly_s, vtkPolyData* poly_ns, int copy_method) {
	
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

	
	for (int i=0;i<poly_ns->GetNumberOfPoints();i++){
		poly_ns_scalars->InsertNextTuple1(-1); 
	}

	int* hit_frequency = new int[poly_s->GetNumberOfPoints()];

	// record hit frequency 
	for (int i=0;i<poly_s->GetNumberOfPoints();i++)
	{
		hit_frequency[i] = 0; 
	}

	if (copy_method == 3)
	{
		vtkPolyData* result = vtkPolyData::New(); 
		ICPRegistration(poly_s, poly_ns, result);
		poly_s->DeepCopy(result);
	}

	//cout << "Finished reading scalars, now transferring .. " << endl;
	for (int i=0;i<poly_ns->GetNumberOfPoints();i++) 
	{	
		poly_ns->GetPoint(i, xyz);

		
		if (copy_method == 2) {		/*	nearest neighbour		*/ 
			closestPointID = point_locator->FindClosestPoint(xyz); 
			
			if (closestPointID != -1) {
				scalar = poly_s->GetPointData()->GetScalars()->GetTuple1(closestPointID);				
				hit_frequency[closestPointID]++; 
			}
			else 
			{
				scalar = 0 ;
			}
			//cout << scalar << endl;
			poly_ns_scalars->SetTuple1(i, scalar);
		}
		
		else if (copy_method == 1){		/*	index copy, expects equal number of points on source and target		*/
			scalar = poly_s->GetPointData()->GetScalars()->GetTuple1(i);
			poly_ns_scalars->SetTuple1(i, scalar); 
		}
	}

	poly_ns->GetPointData()->SetScalars(poly_ns_scalars);

	PrintHitFrequency(hit_frequency, poly_s->GetNumberOfPoints());
	
}


int main(int argc, char *argv[])
{
	bool isIndexCopy=false, isClosestNeighbourCopy = false, foundArgs1=false, isICP=false, isUnequal=false;
	std::string fn1, fn2, fn3; 
	if (argc < 6)
	{
		std::cerr << "Required parameters: \n\t-source source \n\t-target target \n\t-o output"
		"\n\t--nn (use nearest neighbour)\n\t--index (use vertex index to copy) \n\t--icp (does not work)\n\n"
		"Useful notes:\n\tData is copied from source to target"
		"\n\tThe --nn does not require same number of vertices between source and target"
		"\n\tICP option does not work yet" 
		"\n\tUse NN when both surfaces dont have equal number of points. "
		"\n\tNote this method assumes that the surfaces are aligned approximately and data can be copied over reliably from one to the other" << std::endl;
		return EXIT_FAILURE;
	}

	if (argc >= 1)
	{
		for (int i = 1; i < argc; i++) {
			if (i + 1 != argc) {
				if (string(argv[i]) == "-source") {
					fn1 = argv[i + 1];
					foundArgs1 = true;
				}
				else if (string(argv[i]) == "-target") {
					fn2 = argv[i + 1];
					foundArgs1 = true;
				}
				else if (string(argv[i]) == "-o") {
					fn3 = argv[i + 1];
					foundArgs1 = true;
				}
			}
			if (string(argv[i]) == "--index") {
				isIndexCopy = true;
			}
			else if (string(argv[i]) == "--nn") {
				isClosestNeighbourCopy = true;
			}

			else if (string(argv[i]) == "--icp") {
				isICP = true;
			}
			
		}
	}

	if (isIndexCopy == false && isClosestNeighbourCopy == false && isICP == true)
	{
		cerr << "Missing --index, --nn, --icp switches to indicate which method to use. The program will exit now" << endl; 
		exit(0); 
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


	int copy_method = -1; 
	if (isIndexCopy)
		copy_method = 1; 
	else if (isClosestNeighbourCopy)
		copy_method = 2; 
	else if (isICP)
		copy_method = 3; 

	TransferScalars(poly_with_scalar, poly_no_scalar, copy_method);

	vtkSmartPointer<vtkPolyDataWriter> writer = vtkSmartPointer<vtkPolyDataWriter>::New();
	writer->SetInputData(poly_no_scalar);
	writer->SetFileName(fn3.c_str());
	writer->Update();

	return EXIT_SUCCESS;
}