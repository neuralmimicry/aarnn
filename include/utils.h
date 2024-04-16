#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include "AxonBranch.h"
#include "DendriteBranch.h"
#include "Position.h"
#include "SynapticGap.h"
#include "math.h"
#include "vtkincludes.h"

#include <random>

const int     SAMPLE_RATE       = 16000;
const int     FRAMES_PER_BUFFER = 256;
typedef float SAMPLE;
static int    gNumNoInputs = 0;
using PositionPtr          = std::shared_ptr<Position>;
inline std::uniform_real_distribution<> distr(-0.15, 1.0 - 0.15);
inline std::mt19937                     gen(12345);  // Always generates the same sequence of numbers

class AxonBranch;

inline void addDendriteBranchToRenderer(vtkRenderer*                           renderer,
                                        const std::shared_ptr<DendriteBranch>& dendriteBranch,
                                        const std::shared_ptr<Dendrite>&       dendrite,
                                        const vtkSmartPointer<vtkPoints>&      points,
                                        const vtkSmartPointer<vtkIdList>&      pointIds)
{
    const PositionPtr& dendriteBranchPosition = dendriteBranch->getPosition();
    const PositionPtr& dendritePosition       = dendrite->getPosition();

    // Calculate the radii at each end
    double radius1 = 0.5;   // Radius at the start point (dendrite branch)
    double radius2 = 0.25;  // Radius at the end point (dendrite)

    // Compute the vector from the start point to the end point
    double vectorX = dendritePosition->x - dendriteBranchPosition->x;
    double vectorY = dendritePosition->y - dendriteBranchPosition->y;
    double vectorZ = dendritePosition->z - dendriteBranchPosition->z;

    // Compute the length of the vector
    double length = std::sqrt(vectorX * vectorX + vectorY * vectorY + vectorZ * vectorZ);

    // Compute the midpoint of the vector
    double midpointX = (dendritePosition->x + dendriteBranchPosition->x) / 2.0;
    double midpointY = (dendritePosition->y + dendriteBranchPosition->y) / 2.0;
    double midpointZ = (dendritePosition->z + dendriteBranchPosition->z) / 2.0;

    // Create the cone with the desired dimensions
    vtkSmartPointer<vtkConeSource> coneSource = vtkSmartPointer<vtkConeSource>::New();
    coneSource->SetRadius(radius1);
    coneSource->SetHeight(length);
    coneSource->SetResolution(50);  // Set resolution for a smooth cone

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
    coneActor->GetProperty()->SetColor(0.0, 1.0, 0.0);  // Set colour to green

    // Translate the cone to the midpoint position
    vtkSmartPointer<vtkTransform> translationTransform = vtkSmartPointer<vtkTransform>::New();
    translationTransform->Translate(midpointX, midpointY, midpointZ);
    coneActor->SetUserTransform(translationTransform);

    // Add the actor to the renderer
    renderer->AddActor(coneActor);
}

inline void addAxonBranchToRenderer(vtkRenderer*                       renderer,
                                    const std::shared_ptr<AxonBranch>& axonBranch,
                                    const std::shared_ptr<Axon>&       axon,
                                    const vtkSmartPointer<vtkPoints>&  points,
                                    const vtkSmartPointer<vtkIdList>&  pointIds)
{
    const PositionPtr& axonBranchPosition = axonBranch->getPosition();
    const PositionPtr& axonPosition       = axon->getPosition();

    // Calculate the radii at each end
    double radius1 = 0.5;   // Radius at the start point (dendrite branch)
    double radius2 = 0.25;  // Radius at the end point (dendrite)

    // Compute the vector from the start point to the end point
    double vectorX = axonPosition->x - axonBranchPosition->x;
    double vectorY = axonPosition->y - axonBranchPosition->y;
    double vectorZ = axonPosition->z - axonBranchPosition->z;

    // Compute the length of the vector
    double length = std::sqrt(vectorX * vectorX + vectorY * vectorY + vectorZ * vectorZ);

    // Compute the midpoint of the vector
    double midpointX = (axonPosition->x + axonBranchPosition->x) / 2.0;
    double midpointY = (axonPosition->y + axonBranchPosition->y) / 2.0;
    double midpointZ = (axonPosition->z + axonBranchPosition->z) / 2.0;

    // Create the cone with the desired dimensions
    vtkSmartPointer<vtkConeSource> coneSource = vtkSmartPointer<vtkConeSource>::New();
    coneSource->SetRadius(radius1);
    coneSource->SetHeight(length);
    coneSource->SetResolution(50);  // Set resolution for a smooth cone

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
    coneActor->GetProperty()->SetColor(1.0, 0.0, 0.0);  // Set colour to red

    // Translate the cone to the midpoint position
    vtkSmartPointer<vtkTransform> translationTransform = vtkSmartPointer<vtkTransform>::New();
    translationTransform->Translate(midpointX, midpointY, midpointZ);
    coneActor->SetUserTransform(translationTransform);

    // Add the actor to the renderer
    renderer->AddActor(coneActor);
}

inline void addSynapticGapToRenderer(vtkRenderer* renderer, const std::shared_ptr<SynapticGap>& synapticGap)
{
    const PositionPtr& synapticGapPosition = synapticGap->getPosition();

    // Create the sphere source for the glyph
    vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    sphereSource->SetRadius(0.175);  // Set the radius of the sphere

    // Create a vtkPoints object to hold the positions of the spheres
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

    // Add multiple points around the synaptic gap position to create a cloud
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

    // Create a vtkPolyData object and set the points as its vertices
    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
    polyData->SetPoints(points);

    // Create the glyph filter and set the polydata as the input
    vtkSmartPointer<vtkGlyph3D> glyph3D = vtkSmartPointer<vtkGlyph3D>::New();
    glyph3D->SetInputData(polyData);
    glyph3D->SetSourceConnection(sphereSource->GetOutputPort());
    glyph3D->SetScaleFactor(1.0);  // Set the scale factor for the glyph

    // Create a mapper and actor for the glyph
    vtkSmartPointer<vtkPolyDataMapper> glyphMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    glyphMapper->SetInputConnection(glyph3D->GetOutputPort());

    vtkSmartPointer<vtkActor> glyphActor = vtkSmartPointer<vtkActor>::New();
    glyphActor->SetMapper(glyphMapper);
    glyphActor->GetProperty()->SetColor(1.0, 1.0, 0.0);  // Set color to yellow

    // Add the glyph actor to the renderer
    renderer->AddActor(glyphActor);
}

inline void addDendriteBranchPositionsToPolyData(vtkRenderer*                           renderer,
                                                 const std::shared_ptr<DendriteBranch>& dendriteBranch,
                                                 const vtkSmartPointer<vtkPoints>&      definePoints,
                                                 vtkSmartPointer<vtkIdList>             pointIDs)
{
    // Add the position of the dendrite branch to vtkPolyData
    PositionPtr branchPosition = dendriteBranch->getPosition();
    pointIDs->InsertNextId(definePoints->InsertNextPoint(branchPosition->x, branchPosition->y, branchPosition->z));

    // Iterate over the dendrites and add their positions to vtkPolyData
    const std::vector<std::shared_ptr<Dendrite>>& dendrites = dendriteBranch->getDendrites();
    for(const auto& dendrite: dendrites)
    {
        PositionPtr dendritePosition = dendrite->getPosition();
        addDendriteBranchToRenderer(renderer, dendriteBranch, dendrite, definePoints, pointIDs);

        // Select the dendrite bouton and add its position to vtkPolyData
        const std::shared_ptr<DendriteBouton>& bouton         = dendrite->getDendriteBouton();
        PositionPtr                            boutonPosition = bouton->getPosition();
        pointIDs->InsertNextId(definePoints->InsertNextPoint(boutonPosition->x, boutonPosition->y, boutonPosition->z));

        // Recursively process child dendrite branches
        const std::vector<std::shared_ptr<DendriteBranch>>& childBranches = dendrite->getDendriteBranches();
        for(const auto& childBranch: childBranches)
        {
            addDendriteBranchPositionsToPolyData(renderer, childBranch, definePoints, pointIDs);
        }
    }
}

inline void addAxonBranchPositionsToPolyData(vtkRenderer*                       renderer,
                                             const std::shared_ptr<AxonBranch>& axonBranch,
                                             const vtkSmartPointer<vtkPoints>&  definePoints,
                                             vtkSmartPointer<vtkIdList>         pointIDs)
{
    // Add the position of the axon branch to vtkPolyData
    PositionPtr branchPosition = axonBranch->getPosition();
    pointIDs->InsertNextId(definePoints->InsertNextPoint(branchPosition->x, branchPosition->y, branchPosition->z));

    // Iterate over the axons and add their positions to vtkPolyData
    const std::vector<std::shared_ptr<Axon>>& axons = axonBranch->getAxons();
    for(const auto& axon: axons)
    {
        PositionPtr axonPosition = axon->getPosition();
        addAxonBranchToRenderer(renderer, axonBranch, axon, definePoints, pointIDs);

        // Select the axon bouton and add its position to vtkPolyData
        const std::shared_ptr<AxonBouton>& axonBouton         = axon->getAxonBouton();
        PositionPtr                        axonBoutonPosition = axonBouton->getPosition();
        pointIDs->InsertNextId(
         definePoints->InsertNextPoint(axonBoutonPosition->x, axonBoutonPosition->y, axonBoutonPosition->z));

        // Select the synaptic gap and add its position to vtkPolyData
        const std::shared_ptr<SynapticGap>& synapticGap = axonBouton->getSynapticGap();
        addSynapticGapToRenderer(renderer, synapticGap);

        // Recursively process child axon branches
        const std::vector<std::shared_ptr<AxonBranch>>& childBranches = axon->getAxonBranches();
        for(const auto& childBranch: childBranches)
        {
            addAxonBranchPositionsToPolyData(renderer, childBranch, definePoints, pointIDs);
        }
    }
}

inline std::tuple<double, double, double> get_coordinates(int i, int total_points, int points_per_layer)
{
    const double golden_angle = M_PI * (3 - std::sqrt(5));  // golden angle in radians

    // calculate the layer and index in layer based on i
    int layer          = i / points_per_layer;
    int index_in_layer = i % points_per_layer;

    // normalize radial distance based on layer number
    double r = (double)(layer + 1) / (double(total_points) / points_per_layer);

    // azimuthal angle based on golden angle
    double theta = golden_angle * index_in_layer;

    // height y is a function of layer
    double y = 1 - (double)(layer) / (double(total_points) / points_per_layer - 1);

    // radius at y
    double radius = std::sqrt(1 - y * y);

    // polar angle evenly distributed
    double phi = 2 * M_PI * (double)(index_in_layer) / points_per_layer;

    // Convert spherical coordinates to Cartesian coordinates
    double x = r * radius * std::cos(theta);
    y        = r * y;
    double z = r * radius * std::sin(theta);

    return std::make_tuple(x, y, z);
}

#endif  // UTILS_H_INCLUDED
