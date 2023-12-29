#include "../include/nvTimerCallback.h"

static nvTimerCallback::nvTimerCallback *New()
{
    auto *nvCB = new nvTimerCallback;
    nvCB->nvTimerCount = 0;
    return nvCB;
}

void nvTimerCallback::Execute(vtkObject *vtkNotUsed(caller), unsigned long eventId, void *vtkNotUsed(callData)) override
{
    if (vtkCommand::TimerEvent == eventId)
    {
        ++this->nvTimerCount;
    }
    std::lock_guard<std::mutex> lock(*nvMutex);
    // Iterate over the neurons and add their positions to the vtkPolyData
    nvPoints->Reset();
    for (const auto &neuron : nvNeurons)
    {
        const PositionPtr &neuronPosition = neuron->getPosition();

        // Create a vtkIdList to hold the point indices of the line vertices
        vtkSmartPointer<vtkIdList> pointIDs = vtkSmartPointer<vtkIdList>::New();
        pointIDs->InsertNextId(
            nvPoints->InsertNextPoint(neuronPosition->x, neuronPosition->y, neuronPosition->z));

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
        nvRenderer->AddActor(sphereActor);

        // Iterate over the dendrite branches and add their positions to the vtkPolyData
        const std::vector<std::shared_ptr<DendriteBranch>> &dendriteBranches = neuron->getSoma()->getDendriteBranches();
        for (const auto &dendriteBranch : dendriteBranches)
        {
            addDendriteBranchPositionsToPolyData(nvRenderer, dendriteBranch, nvPoints, pointIDs);
        }

        // Add the position of the axon hillock to the vtkPoints and vtkPolyData
        const PositionPtr &axonHillockPosition = neuron->getSoma()->getAxonHillock()->getPosition();
        pointIDs->InsertNextId(nvPoints->InsertNextPoint(axonHillockPosition->x, axonHillockPosition->y, axonHillockPosition->z));

        // Add the position of the axon to the vtkPoints and vtkPolyData
        const PositionPtr &axonPosition = neuron->getSoma()->getAxonHillock()->getAxon()->getPosition();
        pointIDs->InsertNextId(nvPoints->InsertNextPoint(axonPosition->x, axonPosition->y, axonPosition->z));

        // Add the position of the axon bouton to the vtkPoints and vtkPolyData
        const PositionPtr &axonBoutonPosition = neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getPosition();
        pointIDs->InsertNextId(nvPoints->InsertNextPoint(axonBoutonPosition->x, axonBoutonPosition->y, axonBoutonPosition->z));

        // Add the position of the synaptic gap to the vtkPoints and vtkPolyData
        addSynapticGapToRenderer(nvRenderer, neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBouton()->getSynapticGap());

        // Iterate over the axon branches and add their positions to the vtkPolyData
        const std::vector<std::shared_ptr<AxonBranch>> &axonBranches = neuron->getSoma()->getAxonHillock()->getAxon()->getAxonBranches();
        for (const auto &axonBranch : axonBranches)
        {
            addAxonBranchPositionsToPolyData(nvRenderer, axonBranch, nvPoints, pointIDs);
        }

        // Add the line to the cell array
        nvLines->InsertNextCell(pointIDs);

        // Create a vtkPolyData object and set the points and lines as its data
        nvPolyData->SetPoints(nvPoints);

        vtkSmartPointer<vtkDelaunay3D> delaunay = vtkSmartPointer<vtkDelaunay3D>::New();
        delaunay->SetInputData(nvPolyData);
        delaunay->SetAlpha(0.1); // Adjust for mesh density
        // delaunay->Update();

        // Extract the surface of the Delaunay output using vtkGeometryFilter
        // vtkSmartPointer<vtkGeometryFilter> geometryFilter = vtkSmartPointer<vtkGeometryFilter>::New();
        // geometryFilter->SetInputConnection(delaunay->GetOutputPort());
        // geometryFilter->Update();

        // Get the vtkPolyData representing the membrane surface
        // vtkSmartPointer<vtkPolyData> membranePolyData = geometryFilter->GetOutput();

        // Create a vtkPolyDataMapper and set the input as the extracted surface
        vtkSmartPointer<vtkPolyDataMapper> membraneMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        // membraneMapper->SetInputData(membranePolyData);

        // Create a vtkActor for the membrane and set its mapper
        // vtkSmartPointer<vtkActor> membraneActor = vtkSmartPointer<vtkActor>::New();
        // membraneActor->SetMapper(membraneMapper);
        // membraneActor->GetProperty()->SetColor(0.7, 0.7, 0.7);  // Set color to gray

        // Add the membrane actor to the renderer
        // renderer->AddActor(membraneActor);

        nvPolyData->SetLines(nvLines);

        // Create a mapper and actor for the neuron positions
        nvPolyData->Modified();
        nvMapper->SetInputData(nvPolyData);
        nvActor->SetMapper(nvMapper);
        nvRenderer->AddActor(nvActor);
    }
    nvRenderer->Modified();
}

void nvTimerCallback::setNeurons(const std::vector<std::shared_ptr<Neuron>> &visualiseNeurons) { this->nvNeurons = visualiseNeurons; }

void nvTimerCallback::setMutex(std::mutex &mutex) { this->nvMutex = &mutex; }

void nvTimerCallback::setPoints(const vtkSmartPointer<vtkPoints> &points) { this->nvPoints = points; }

void nvTimerCallback::setLines(const vtkSmartPointer<vtkCellArray> &lines) { this->nvLines = lines; }

void nvTimerCallback::setPolyData(const vtkSmartPointer<vtkPolyData> &polydata) { this->nvPolyData = polydata; }

void nvTimerCallback::setActor(const vtkSmartPointer<vtkActor> &actor) { this->nvActor = actor; }

void nvTimerCallback::setMapper(const vtkSmartPointer<vtkPolyDataMapper> &mapper) { this->nvMapper = mapper; }

void nvTimerCallback::setRenderer(const vtkSmartPointer<vtkRenderer> &renderer) { this->nvRenderer = renderer; }

void nvTimerCallback::setRenderWindow(const vtkSmartPointer<vtkRenderWindow> &renderWindow) { this->nvRenderWindow = renderWindow; }