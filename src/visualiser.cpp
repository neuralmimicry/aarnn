#include <iostream>
#include <vector>
#include <memory>
#include "boostincludes.h"
#include "vtkincludes.h"
#include "aarnn.h"
#include "Neuron.h" // Include the necessary headers

int main() {
    // Initialize logger
    Logger logger("errors.log");

    // Initialize VTK
    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderWindow> render_window = vtkSmartPointer<vtkRenderWindow>::New();
    render_window->AddRenderer(renderer);
    vtkSmartPointer<vtkRenderWindowInteractor> render_window_interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    render_window_interactor->SetRenderWindow(render_window);

    // Create a vtkPoints object to hold the neuron positions
    vtkSmartPointer<vtkPoints> definePoints = vtkSmartPointer<vtkPoints>::New();

    // Create a vtkCellArray object to hold the lines
    vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();

    try {
        // Read the database connection configuration and simulation configuration
        std::vector<std::string> config_filenames = {"db_connection.conf", "simulation.conf"};
        auto config = read_config(config_filenames);
        std::string connection_string = build_connection_string(config);

        // Connect to PostgreSQL
        pqxx::connection conn(connection_string);

        // Start a transaction
        pqxx::work txn(conn);

        // Continuously update the visualization
        while (true) {
            // Read data from the database
            std::vector<double> propagationRates;
            std::string query = "SELECT propagation_rate FROM neurons";
            pqxx::result result = txn.exec(query);
            for (const auto& row : result) {
                double propagationRate = row["propagation_rate"].as<double>();
                propagationRates.push_back(propagationRate);
            }

            // Iterate over the neurons and add their positions to the vtkPolyData
            for (const auto& neuron : neurons) {
                const PositionPtr &neuronPosition = neuron->getPosition();

                // Create a vtkIdList to hold the point indices of the line vertices
                vtkSmartPointer<vtkIdList> pointIDs = vtkSmartPointer<vtkIdList>::New();
                pointIDs->InsertNextId(
                        definePoints->InsertNextPoint(neuronPosition->x, neuronPosition->y, neuronPosition->z));

                const PositionPtr &somaPosition = neuron->getSoma()->getPosition();
                // Add the sphere centre point to the vtkPolyData
                vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
                sphere->SetRadius(1.0); // Adjust the sphere radius as needed
                sphere->SetCenter(somaPosition->x, somaPosition->y, somaPosition->z);
                sphere->SetThetaResolution(32); // Set the sphere resolution
                sphere->SetPhiResolution(16);
                sphere->Update();

                // Add the sphere to the renderer
                vtkSmartPointer<vtkPolyDataMapper> sphereMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
                sphereMapper->SetInputData(sphere->GetOutput());
                vtkSmartPointer<vtkActor> sphereActor = vtkSmartPointer<vtkActor>::New();
                sphereActor->SetMapper(sphereMapper);
                renderer->AddActor(sphereActor);

                // Iterate over the dendrite branches and add their positions to the vtkPolyData
                const std::vector<std::shared_ptr<DendriteBranch>> &dendriteBranches = neuron->getSoma()->getDendriteBranches();
                for (const auto &dendriteBranch: dendriteBranches) {
                    addDendriteBranchPositionsToPolyData(renderer, dendriteBranch, definePoints, pointIDs);
                }

                // Add the position of the axon hillock to the vtkPoints and vtkPolyData
                const PositionPtr &axonHillockPosition = neuron->getSoma()->getAxonHillock()->getPosition();
                pointIDs->InsertNextId(definePoints->InsertNextPoint(axonHillockPosition->x, axonHillockPosition->y,
                                                                     axonHillockPosition->z));

                // Add the position of the axon to the vtkPoints and vtkPolyData
                const PositionPtr &axonPosition = neuron->getSoma()->getAxonHillock()->getAxon()->getPosition();
                pointIDs->InsertNextId(definePoints->InsertNextPoint(axonPosition->x, axonPosition->y, axonPosition->z));

                // Add the position of the axon bouton to the vtkPoints and vtkPolyData
                const PositionPtr &axonBoutonPosition = neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getPosition();
                pointIDs->InsertNextId(definePoints->InsertNextPoint(axonBoutonPosition->x, axonBoutonPosition->y,
                                                                     axonBoutonPosition->z));

                // Add the position of the synaptic gap to the vtkPoints and vtkPolyData
                addSynapticGapToRenderer(renderer,
                                         neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap());

                // Iterate over the axon branches and add their positions to the vtkPolyData
                const std::vector<std::shared_ptr<AxonBranch>> &axonBranches = neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBranches();
                for (const auto &axonBranch: axonBranches) {
                    addAxonBranchPositionsToPolyData(renderer, axonBranch, definePoints, pointIDs);
                }

                // Add the line to the cell array
                lines->InsertNextCell(pointIDs);


                // Create a vtkPolyData object and set the points and lines as its data
                vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
                polyData->SetPoints(definePoints);

                vtkSmartPointer<vtkDelaunay3D> delaunay = vtkSmartPointer<vtkDelaunay3D>::New();
                delaunay->SetInputData(polyData);
                delaunay->SetAlpha(0.1); // Adjust for mesh density
                //delaunay->Update();

                // Extract the surface of the Delaunay output using vtkGeometryFilter
                //vtkSmartPointer<vtkGeometryFilter> geometryFilter = vtkSmartPointer<vtkGeometryFilter>::New();
                //geometryFilter->SetInputConnection(delaunay->GetOutputPort());
                //geometryFilter->Update();

                // Get the vtkPolyData representing the membrane surface
                //vtkSmartPointer<vtkPolyData> membranePolyData = geometryFilter->GetOutput();

                // Create a vtkPolyDataMapper and set the input as the extracted surface
                vtkSmartPointer<vtkPolyDataMapper> membraneMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
                //membraneMapper->SetInputData(membranePolyData);

                // Create a vtkActor for the membrane and set its mapper
                //vtkSmartPointer<vtkActor> membraneActor = vtkSmartPointer<vtkActor>::New();
                //membraneActor->SetMapper(membraneMapper);
                //membraneActor->GetProperty()->SetColor(0.7, 0.7, 0.7);  // Set color to gray

                // Add the membrane actor to the renderer
                // renderer->AddActor(membraneActor);

                polyData->SetLines(lines);

                // Create a mapper and actor for the neuron positions
                vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
                mapper->SetInputData(polyData);
                vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
                actor->SetMapper(mapper);
                renderer->AddActor(actor);
            }
            // Set up the render window and start the interaction
            renderWindow->SetSize(400, 300);
            renderWindow->Render();
            vtkSmartPointer<vtkWindowToImageFilter> windowToImageFilter = vtkSmartPointer<vtkWindowToImageFilter>::New();
            windowToImageFilter->SetInput(renderWindow);
            windowToImageFilter->Update();

            vtkSmartPointer<vtkJPEGWriter> jpegWriter = vtkSmartPointer<vtkJPEGWriter>::New();
            jpegWriter->SetInputConnection(windowToImageFilter->GetOutputPort());
            jpegWriter->SetQuality(80);  // Adjust the JPEG quality as needed
            jpegWriter->Write();

            vtkImageData* imageData = jpegWriter->GetResult();
            if (!imageData) {
                return "";
            }

            unsigned char* buffer = static_cast<unsigned char*>(imageData->GetScalarPointer());
            size_t bufferSize = imageData->GetScalarSize() * imageData->GetNumberOfPoints();

            std::string base64Image = base64_encode(buffer, bufferSize);

            renderWindowInteractor->Start();

            // Sleep for a certain interval to control the visualization rate
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } catch (const std::exception &e) {
        logger << "Error: " << e.what() << std::endl;
        std::cerr << "Error: " << e.what() << std::endl;
    } catch (...) {
        logger << "Error: Unknown exception occurred." << std::endl;
        std::cerr << "Error: Unknown exception occurred." << std::endl;
    }

    return 0;
}
