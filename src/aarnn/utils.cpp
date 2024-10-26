//
// Created by pbisaacs on 24/06/24.
//
#include "utils.h"
#include <algorithm>
#include <iostream>
#include <sstream>

void atomic_add(std::atomic<double>& atomic_val, double value) {
    double current = atomic_val.load();
    while (!atomic_val.compare_exchange_weak(current, current + value)) {}
}

double mesh_distance = 1.0; // Distance from the convex hull to the mesh

// Function to compute the Euclidean distance between two points in 3D space (x, y, z)
double distance_3d(const Point_3& p1, const Point_3& p2) {
    return std::sqrt(std::pow(p1.x() - p2.x(), 2) +
                     std::pow(p1.y() - p2.y(), 2) +
                     std::pow(p1.z() - p2.z(), 2));
}

// Function to compute the energy at a point considering the energy diminishment due to range
double compute_effective_energy(const std::vector<std::tuple<double, double, double, double>>& points, const Point_3& new_point) {
    double total_energy = 0.0;

    for (const auto& pt : points) {
        Point_3 existing_point(std::get<0>(pt), std::get<1>(pt), std::get<2>(pt));
        double distance = distance_3d(existing_point, new_point);
        double energy = std::get<3>(pt) / (1 + distance); // Energy diminishes with distance
        total_energy += energy;
    }

    return total_energy;
}

// Function to compute the additional energy from the mesh
double compute_mesh_energy(const Point_3& point) {
    // Assuming a uniform energy distribution within the space
    // between the convex hull and the mesh, we calculate the mesh energy
    // as an example, we use a simple function that decreases with distance from the convex hull
    double distance_from_mesh = mesh_distance; // Simplified for demonstration
    double mesh_energy = 100.0 / (1 + distance_from_mesh); // Adjust as needed
    return mesh_energy;
}

std::tuple<double, double, double> generate_point_on_convex_hull(std::vector<std::tuple<double, double, double, double>>& points) {
    // Handle the case where there are no prior tuples
    if (points.empty()) {
        return std::make_tuple(0.0, 0.0, 0.0);
    }

    // Handle the case where there is only one prior tuple
    if (points.size() == 1) {
        // Place the new point on the unit sphere surface around the origin
        double theta = 2 * M_PI * (std::rand() / (RAND_MAX + 1.0));  // Random angle theta
        double phi = acos(1 - 2 * (std::rand() / (RAND_MAX + 1.0))); // Random angle phi
        double x = sin(phi) * cos(theta);
        double y = sin(phi) * sin(theta);
        double z = cos(phi);
        return std::make_tuple(x, y, z);
    }

    std::vector<Point_3> cgal_points;

    for (const auto& pt : points) {
        cgal_points.emplace_back(std::get<0>(pt), std::get<1>(pt), std::get<2>(pt));
    }

    // Compute the convex hull
    Polyhedron poly;
    CGAL::convex_hull_3(cgal_points.begin(), cgal_points.end(), poly);

    std::vector<Point_3> hull_points;
    for (auto it = poly.points_begin(); it != poly.points_end(); ++it) {
        hull_points.push_back(*it);
    }

    // Find the point on the convex hull with the highest effective energy
    Point_3 best_point = hull_points[0];
    double max_effective_energy = std::numeric_limits<double>::lowest();

    for (const auto& pt : hull_points) {
        double effective_energy = compute_effective_energy(points, pt) + compute_mesh_energy(pt);
        if (effective_energy > max_effective_energy) {
            max_effective_energy = effective_energy;
            best_point = pt;
        }
    }

    return std::make_tuple(best_point.x(), best_point.y(), best_point.z());
}

void visualize_points_and_hull(const std::vector<std::tuple<double, double, double, double>>& points) {
    vtkSmartPointer<vtkPoints> vtk_points = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkPoints> vtk_mesh_points = vtkSmartPointer<vtkPoints>::New();
    for (const auto& pt : points) {
        vtk_points->InsertNextPoint(std::get<0>(pt), std::get<1>(pt), std::get<2>(pt));
    }

    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
    polyData->SetPoints(vtk_points);

    vtkSmartPointer<vtkDelaunay3D> delaunay = vtkSmartPointer<vtkDelaunay3D>::New();
    delaunay->SetInputData(polyData);

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(delaunay->GetOutputPort());

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(0.0, 1.0, 0.0); // Green for convex hull

    // Create the renderer, render window, and interactor
    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->AddRenderer(renderer);
    vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    renderWindowInteractor->SetRenderWindow(renderWindow);

    // Add the actor to the scene
    renderer->AddActor(actor);
    renderer->SetBackground(0.1, 0.1, 0.1); // Background color dark gray

    // Compute and add the mesh points
    for (const auto& pt : points) {
        double x = std::get<0>(pt) + mesh_distance;
        double y = std::get<1>(pt) + mesh_distance;
        double z = std::get<2>(pt) + mesh_distance;
        vtk_mesh_points->InsertNextPoint(x, y, z);
    }

    vtkSmartPointer<vtkPolyData> meshPolyData = vtkSmartPointer<vtkPolyData>::New();
    meshPolyData->SetPoints(vtk_mesh_points);

    vtkSmartPointer<vtkDelaunay3D> meshDelaunay = vtkSmartPointer<vtkDelaunay3D>::New();
    meshDelaunay->SetInputData(meshPolyData);

    vtkSmartPointer<vtkPolyDataMapper> meshMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    meshMapper->SetInputConnection(meshDelaunay->GetOutputPort());

    vtkSmartPointer<vtkActor> meshActor = vtkSmartPointer<vtkActor>::New();
    meshActor->SetMapper(meshMapper);
    meshActor->GetProperty()->SetColor(0.0, 0.0, 1.0); // Blue for mesh

    renderer->AddActor(meshActor);

    // Render and interact
    renderWindow->Render();
    renderWindowInteractor->Start();
}

void addDendriteBranchToRenderer(vtkRenderer *renderer,
                                 const std::shared_ptr<DendriteBranch> &dendriteBranch,
                                 const std::shared_ptr<Dendrite> &dendrite,
                                 const vtkSmartPointer<vtkPoints> &points,
                                 const vtkSmartPointer<vtkIdList> &pointIds)
{
    const PositionPtr &dendriteBranchPosition = dendriteBranch->getPosition();
    const PositionPtr &dendritePosition = dendrite->getPosition();

    double radius1 = 0.5;
    double radius2 = 0.25;

    double vectorX = dendritePosition->x - dendriteBranchPosition->x;
    double vectorY = dendritePosition->y - dendriteBranchPosition->y;
    double vectorZ = dendritePosition->z - dendriteBranchPosition->z;

    double length = std::sqrt(vectorX * vectorX + vectorY * vectorY + vectorZ * vectorZ);

    double midpointX = (dendritePosition->x + dendriteBranchPosition->x) / 2.0;
    double midpointY = (dendritePosition->y + dendriteBranchPosition->y) / 2.0;
    double midpointZ = (dendritePosition->z + dendriteBranchPosition->z) / 2.0;

    vtkSmartPointer<vtkConeSource> coneSource = vtkSmartPointer<vtkConeSource>::New();
    coneSource->SetRadius(radius1);
    coneSource->SetHeight(length);
    coneSource->SetResolution(50);

    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->Scale(radius2 / radius1, radius2 / radius1, 1.0);

    vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    transformFilter->SetInputConnection(coneSource->GetOutputPort());
    transformFilter->SetTransform(transform);
    transformFilter->Update();

    vtkSmartPointer<vtkPolyDataMapper> coneMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    coneMapper->SetInputData(transformFilter->GetOutput());

    vtkSmartPointer<vtkActor> coneActor = vtkSmartPointer<vtkActor>::New();
    coneActor->SetMapper(coneMapper);
    coneActor->GetProperty()->SetColor(0.0, 1.0, 0.0);

    vtkSmartPointer<vtkTransform> translationTransform = vtkSmartPointer<vtkTransform>::New();
    translationTransform->Translate(midpointX, midpointY, midpointZ);
    coneActor->SetUserTransform(translationTransform);

    renderer->AddActor(coneActor);
}

void addAxonBranchToRenderer(vtkRenderer *renderer,
                             const std::shared_ptr<AxonBranch> &axonBranch,
                             const std::shared_ptr<Axon> &axon,
                             const vtkSmartPointer<vtkPoints> &points,
                             const vtkSmartPointer<vtkIdList> &pointIds)
{
    const PositionPtr &axonBranchPosition = axonBranch->getPosition();
    const PositionPtr &axonPosition = axon->getPosition();

    double radius1 = 0.5;
    double radius2 = 0.25;

    double vectorX = axonPosition->x - axonBranchPosition->x;
    double vectorY = axonPosition->y - axonBranchPosition->y;
    double vectorZ = axonPosition->z - axonBranchPosition->z;

    double length = std::sqrt(vectorX * vectorX + vectorY * vectorY + vectorZ * vectorZ);

    double midpointX = (axonPosition->x + axonBranchPosition->x) / 2.0;
    double midpointY = (axonPosition->y + axonBranchPosition->y) / 2.0;
    double midpointZ = (axonPosition->z + axonBranchPosition->z) / 2.0;

    vtkSmartPointer<vtkConeSource> coneSource = vtkSmartPointer<vtkConeSource>::New();
    coneSource->SetRadius(radius1);
    coneSource->SetHeight(length);
    coneSource->SetResolution(50);

    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->Scale(radius2 / radius1, radius2 / radius1, 1.0);

    vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    transformFilter->SetInputConnection(coneSource->GetOutputPort());
    transformFilter->SetTransform(transform);
    transformFilter->Update();

    vtkSmartPointer<vtkPolyDataMapper> coneMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    coneMapper->SetInputData(transformFilter->GetOutput());

    vtkSmartPointer<vtkActor> coneActor = vtkSmartPointer<vtkActor>::New();
    coneActor->SetMapper(coneMapper);
    coneActor->GetProperty()->SetColor(1.0, 0.0, 0.0);

    vtkSmartPointer<vtkTransform> translationTransform = vtkSmartPointer<vtkTransform>::New();
    translationTransform->Translate(midpointX, midpointY, midpointZ);
    coneActor->SetUserTransform(translationTransform);

    renderer->AddActor(coneActor);
}

void addSynapticGapToRenderer(vtkRenderer *renderer, const std::shared_ptr<SynapticGap> &synapticGap)
{
    const PositionPtr &synapticGapPosition = synapticGap->getPosition();

    vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    sphereSource->SetRadius(0.175);

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

    for(int i = 0; i < 8; ++i)
    {
        double offsetX = distr(gen);
        double offsetY = distr(gen);
        double offsetZ = distr(gen);

        double x = synapticGapPosition->x + offsetX;
        double y = synapticGapPosition->y + offsetY;
        double z = synapticGapPosition->z + offsetZ;

        points->InsertNextPoint(x, y, z);
    }

    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
    polyData->SetPoints(points);

    vtkSmartPointer<vtkGlyph3D> glyph3D = vtkSmartPointer<vtkGlyph3D>::New();
    glyph3D->SetInputData(polyData);
    glyph3D->SetSourceConnection(sphereSource->GetOutputPort());
    glyph3D->SetScaleFactor(1.0);

    vtkSmartPointer<vtkPolyDataMapper> glyphMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    glyphMapper->SetInputConnection(glyph3D->GetOutputPort());

    vtkSmartPointer<vtkActor> glyphActor = vtkSmartPointer<vtkActor>::New();
    glyphActor->SetMapper(glyphMapper);
    glyphActor->GetProperty()->SetColor(1.0, 1.0, 0.0);

    renderer->AddActor(glyphActor);
}

void addDendriteBranchPositionsToPolyData(vtkRenderer *renderer,
                                          const std::shared_ptr<DendriteBranch> &dendriteBranch,
                                          const vtkSmartPointer<vtkPoints> &definePoints,
                                          vtkSmartPointer<vtkIdList> pointIDs)
{
    PositionPtr branchPosition = dendriteBranch->getPosition();
    pointIDs->InsertNextId(definePoints->InsertNextPoint(branchPosition->x, branchPosition->y, branchPosition->z));

    const std::vector<std::shared_ptr<Dendrite>> &dendrites = dendriteBranch->getDendrites();
    for(const auto &dendrite: dendrites)
    {
        PositionPtr dendritePosition = dendrite->getPosition();
        addDendriteBranchToRenderer(renderer, dendriteBranch, dendrite, definePoints, pointIDs);

        const std::shared_ptr<DendriteBouton> &bouton = dendrite->getDendriteBouton();
        PositionPtr boutonPosition = bouton->getPosition();
        pointIDs->InsertNextId(definePoints->InsertNextPoint(boutonPosition->x, boutonPosition->y, boutonPosition->z));

        const std::vector<std::shared_ptr<DendriteBranch>> &childBranches = dendrite->getDendriteBranches();
        for(const auto &childBranch: childBranches)
        {
            addDendriteBranchPositionsToPolyData(renderer, childBranch, definePoints, pointIDs);
        }
    }
}

void addAxonBranchPositionsToPolyData(vtkRenderer *renderer,
                                      const std::shared_ptr<AxonBranch> &axonBranch,
                                      const vtkSmartPointer<vtkPoints> &definePoints,
                                      vtkSmartPointer<vtkIdList> pointIDs)
{
    PositionPtr branchPosition = axonBranch->getPosition();
    pointIDs->InsertNextId(definePoints->InsertNextPoint(branchPosition->x, branchPosition->y, branchPosition->z));

    const std::vector<std::shared_ptr<Axon>> &axons = axonBranch->getAxons();
    for(const auto &axon: axons)
    {
        PositionPtr axonPosition = axon->getPosition();
        addAxonBranchToRenderer(renderer, axonBranch, axon, definePoints, pointIDs);

        const std::shared_ptr<AxonBouton> &axonBouton = axon->getAxonBouton();
        PositionPtr axonBoutonPosition = axonBouton->getPosition();
        pointIDs->InsertNextId(definePoints->InsertNextPoint(axonBoutonPosition->x, axonBoutonPosition->y, axonBoutonPosition->z));

        const std::shared_ptr<SynapticGap> &synapticGap = axonBouton->getSynapticGap();
        addSynapticGapToRenderer(renderer, synapticGap);

        const std::vector<std::shared_ptr<AxonBranch>> &childBranches = axon->getAxonBranches();
        for(const auto &childBranch: childBranches)
        {
            addAxonBranchPositionsToPolyData(renderer, childBranch, definePoints, pointIDs);
        }
    }
}

std::tuple<double, double, double> get_coordinates(int i, int total_points, int points_per_layer)
{
    // Calculate the number of layers
    int num_layers = total_points / points_per_layer;
    if (num_layers == 0) num_layers = 1; // Avoid division by zero

    // Determine the layer index and index within the layer
    int layer = i / points_per_layer;
    int index_in_layer = i % points_per_layer;

    // Calculate the radius for this layer
    double radius = 1.0 + layer * 0.5; // Adjust 0.5 to control the spacing between layers

    // Calculate the angles for spherical coordinates
    double theta = M_PI * (double)(layer + 1) / (double)(num_layers + 1);          // Polar angle from 0 to PI
    double phi = 2.0 * M_PI * (double)(index_in_layer) / (double)(points_per_layer); // Azimuthal angle from 0 to 2PI

    // Convert spherical coordinates to Cartesian coordinates
    double x = radius * sin(theta) * cos(phi);
    double y = radius * sin(theta) * sin(phi);
    double z = radius * cos(theta);

    return std::make_tuple(x, y, z);
}

const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64_encode(const unsigned char* data, size_t length) {
    std::string base64;
    int i = 0;
    int j;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (length--) {
        char_array_3[i++] = *(data++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++) {
                base64 += base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++) {
            char_array_3[j] = '\0';
        }

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; j < i + 1; j++) {
            base64 += base64_chars[char_array_4[j]];
        }

        while ((i++ < 3)) {
            base64 += '=';
        }
    }

    return base64;
}

bool convertStringToBool(const std::string& value) {
    std::string lowerValue = value;
    std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);

    if (lowerValue == "true" || lowerValue == "yes" || lowerValue == "1") {
        return true;
    } else if (lowerValue == "false" || lowerValue == "no" || lowerValue == "0") {
        return false;
    } else {
        std::cerr << "Invalid boolean string representation: " << value << std::endl;
        return false;
    }
}

void associateSynapticGap(Neuron& neuron1, Neuron& neuron2, double proximityThreshold) {
    for (auto& gap : neuron1.getSynapticGapsAxon()) {
        if (!gap || gap->isAssociated()) continue;

        for (auto& dendriteBouton : neuron2.getDendriteBoutons()) {
            if (!dendriteBouton) continue;
            if (gap->getPosition()->distanceTo(*(dendriteBouton->getPosition())) < proximityThreshold) {
                dendriteBouton->connectSynapticGap(gap);
                gap->setAsAssociated();
                break;
            }
        }
    }
}

void associateSynapticGap(SensoryReceptor& receptor, Neuron& neuron, double proximityThreshold) {
    // Iterate over all SynapticGaps of receptor
    for (auto& gap : receptor.getSynapticGaps()) {
        // If the bouton has a synaptic gap, and it is not associated yet
        if (gap && !gap->isAssociated()) {
            // Iterate over all DendriteBoutons in neuron
            for (auto& dendriteBouton : neuron.getDendriteBoutons()) {
                std::cout << "Checking gap " << gap->getPosition() << " with dendrite bouton " << dendriteBouton->getPosition() << std::endl;

                // If the distance between the gap and the dendriteBouton is below the proximity threshold
                if (gap->getPosition()->distanceTo(*(dendriteBouton->getPosition())) < proximityThreshold) {
                    // Associate the synaptic gap with the dendriteBouton
                    dendriteBouton->connectSynapticGap(gap);
                    // Set the SynapticGap as associated
                    gap->setAsAssociated();
                    std::cout << "Associated gap " << gap->getPosition() << " with dendrite bouton " << dendriteBouton->getPosition() << std::endl;
                    // No need to check other DendriteBoutons once the gap is associated
                    break;
                }
            }
        }
    }
}
