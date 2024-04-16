#include "DB.h"
#include "boostincludes.h"
#include "config.h"
#include "vtkincludes.h"
#define DO_TRACE_
#include "traceutil.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <set>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

void insertDendriteBranches(DB                                    &database,
                            const vtkSmartPointer<vtkPoints>      &points,
                            vtkSmartPointer<vtkCellArray>         &lines,
                            vtkSmartPointer<vtkPoints>            &glyphPoints,
                            vtkNew<vtkFloatArray>                 &glyphVectors,
                            vtkSmartPointer<vtkUnsignedCharArray> &glyphTypes,
                            int                                   &dendrite_branch_id,
                            int                                   &dendrite_id,
                            int                                   &dendrite_bouton_id,
                            int                                   &soma_id)
{
    int                      dendriteBranchGlyphType;
    int                      dendriteGlyphType       = 3;
    int                      dendriteBoutonGlyphType = 2;
    vtkIdType                dendriteBranchAnchor;
    vtkIdType                dendriteAnchor;
    vtkIdType                dendriteBoutonAnchor;
    vtkIdType                glyphDendriteBranchAnchor;
    vtkIdType                glyphDendriteAnchor;
    vtkIdType                glyphDendriteBoutonAnchor;
    double                   x, y, z;
    double                   glyphVectorBlank[3] = {0.0, 0.0, 0.0};
    pqxx::result             dendritebranches;
    pqxx::result             innerdendritebranches;
    pqxx::result             dendrites;
    pqxx::result             dendriteboutons;
    vtkSmartPointer<vtkLine> line;

    if(soma_id == -1)
    {
        dendritebranches        = database.exec("SELECT dendrite_branch_id, dendrite_id, x, y, "
                                                "z FROM dendritebranches WHERE dendrite_id = "
                                         + std::to_string(dendrite_id) + " ORDER BY dendrite_branch_id ASC");
        dendriteBranchGlyphType = 2;
    }
    else
    {
        dendritebranches        = database.exec("SELECT dendrite_branch_id, soma_id, x, y, z FROM "
                                                "dendritebranches_soma WHERE soma_id = "
                                         + std::to_string(soma_id) + " ORDER BY dendrite_branch_id ASC");
        dendriteBranchGlyphType = 3;
    }

    // std::cout << "Dendrite branches: " << dendritebranches.size() << std::endl;
    // std::cout << "soma_id: " << soma_id << std::endl;
    for(auto dendritebranch: dendritebranches)
    {
        dendrite_branch_id = dendritebranch[0].as<int>();
        x                  = dendritebranch[2].as<double>();
        y                  = dendritebranch[3].as<double>();
        z                  = dendritebranch[4].as<double>();
        points->InsertNextPoint(x, y, z);
        dendriteBranchAnchor = points->GetNumberOfPoints() - 1;
        glyphPoints->InsertNextPoint(x, y, z);
        glyphDendriteBranchAnchor = glyphPoints->GetNumberOfPoints() - 1;
        glyphVectors->InsertNextTuple(glyphVectorBlank);
        glyphTypes->InsertNextValue(0);

        dendrites = database.exec("SELECT dendrite_id, dendrite_branch_id, x, y, z FROM "
                                  "dendrites WHERE dendrite_branch_id = "
                                  + std::to_string(dendrite_branch_id) + " ORDER BY dendrite_id ASC");
        // std::cout << "dendrites.size(): " << dendrites.size() << std::endl;
        for(auto dendrite: dendrites)
        {
            dendrite_id = dendrite[0].as<int>();
            x           = dendrite[2].as<double>();
            y           = dendrite[3].as<double>();
            z           = dendrite[4].as<double>();
            points->InsertNextPoint(x, y, z);
            dendriteAnchor = points->GetNumberOfPoints() - 1;
            glyphPoints->InsertNextPoint(x, y, z);
            glyphDendriteAnchor = glyphPoints->GetNumberOfPoints() - 1;
            line                = vtkSmartPointer<vtkLine>::New();
            line->GetPointIds()->SetId(0, dendriteBranchAnchor);
            line->GetPointIds()->SetId(1, dendriteAnchor);
            lines->InsertNextCell(line);
            double glyphVector[3] = {
             glyphPoints->GetPoint(glyphDendriteAnchor)[0] - glyphPoints->GetPoint(glyphDendriteBranchAnchor)[0],
             glyphPoints->GetPoint(glyphDendriteAnchor)[1] - glyphPoints->GetPoint(glyphDendriteBranchAnchor)[1],
             glyphPoints->GetPoint(glyphDendriteAnchor)[2] - glyphPoints->GetPoint(glyphDendriteBranchAnchor)[2]};
            glyphVectors->InsertNextTuple(glyphVector);
            glyphTypes->InsertNextValue(dendriteGlyphType);

            dendriteboutons = database.exec("SELECT dendrite_bouton_id, dendrite_id, x, y, z FROM "
                                            "dendriteboutons WHERE dendrite_id = "
                                            + std::to_string(dendrite_id) + " ORDER BY dendrite_bouton_id ASC");

            for(auto dendritebouton: dendriteboutons)
            {
                dendrite_bouton_id = dendritebouton[0].as<int>();
                x                  = dendritebouton[2].as<double>();
                y                  = dendritebouton[3].as<double>();
                z                  = dendritebouton[4].as<double>();
                points->InsertNextPoint(x, y, z);
                dendriteBoutonAnchor = points->GetNumberOfPoints() - 1;
                glyphPoints->InsertNextPoint(x, y, z);
                glyphDendriteBoutonAnchor = glyphPoints->GetNumberOfPoints() - 1;
                double glyphVector[3]     = {
                     glyphPoints->GetPoint(glyphDendriteBoutonAnchor)[0] - glyphPoints->GetPoint(glyphDendriteAnchor)[0],
                     glyphPoints->GetPoint(glyphDendriteBoutonAnchor)[1] - glyphPoints->GetPoint(glyphDendriteAnchor)[1],
                     glyphPoints->GetPoint(glyphDendriteBoutonAnchor)[2] - glyphPoints->GetPoint(glyphDendriteAnchor)[2]};
                glyphVectors->InsertNextTuple(glyphVector);
                glyphTypes->InsertNextValue(dendriteBoutonGlyphType);
            }

            innerdendritebranches = database.exec("SELECT dendrite_branch_id, dendrite_id, x, y, z FROM "
                                                  "dendritebranches WHERE dendrite_id = "
                                                  + std::to_string(dendrite_id) + " ORDER BY dendrite_branch_id ASC");
            soma_id               = -1;
            for(auto innerdendritebranch: innerdendritebranches)
            {
                insertDendriteBranches(database,
                                       points,
                                       lines,
                                       glyphPoints,
                                       glyphVectors,
                                       glyphTypes,
                                       dendrite_branch_id,
                                       dendrite_id,
                                       dendrite_bouton_id,
                                       soma_id);
            }
        }
    }
}

void insertAxons(DB                                    &database,
                 const vtkSmartPointer<vtkPoints>      &points,
                 vtkSmartPointer<vtkCellArray>         &lines,
                 vtkSmartPointer<vtkPoints>            &glyphPoints,
                 vtkNew<vtkFloatArray>                 &glyphVectors,
                 vtkSmartPointer<vtkUnsignedCharArray> &glyphTypes,
                 int                                   &axon_branch_id,
                 int                                   &axon_id,
                 int                                   &axon_bouton_id,
                 int                                   &synaptic_gap_id,
                 int                                   &axon_hillock_id,
                 long                                  &glyphAxonBranchAnchor)
{
    int                      axonBranchGlyphType;
    int                      axonGlyphType        = 8;
    int                      axonBoutonGlyphType  = 9;
    int                      synapticGapGlyphType = 10;
    vtkIdType                axonBranchAnchor;
    vtkIdType                axonAnchor;
    vtkIdType                axonBoutonAnchor;
    vtkIdType                synapticGapAnchor;
    vtkIdType                glyphAxonAnchor;
    vtkIdType                glyphAxonBoutonAnchor;
    vtkIdType                glyphSynapticGapAnchor;
    double                   x, y, z;
    double                   glyphVectorBlank[3] = {0.0, 0.0, 0.0};
    pqxx::result             axonbranches;
    pqxx::result             inneraxonbranches;
    pqxx::result             axons;
    pqxx::result             axonboutons;
    pqxx::result             synapticgaps;
    vtkSmartPointer<vtkLine> line;

    if(axon_hillock_id == -1)
    {
        axons               = database.exec("SELECT axon_id, axon_branch_id, x, y, z FROM axons WHERE "
                                            "axon_branch_id = "
                              + std::to_string(axon_branch_id) + " ORDER BY axon_id ASC");
        axonBranchGlyphType = 6;
    }
    else
    {
        axons               = database.exec("SELECT axon_id, axon_hillock_id, x, y, z FROM axons "
                                            "WHERE axon_hillock_id = "
                              + std::to_string(axon_hillock_id) + " ORDER BY axon_id ASC");
        axonBranchGlyphType = 7;
    }

    for(auto axon: axons)
    {
        axon_id = axon[0].as<int>();
        // std::cout << "axon_id: " << axon_id << std::endl;
        x = axon[2].as<double>();
        y = axon[3].as<double>();
        z = axon[4].as<double>();
        points->InsertNextPoint(x, y, z);
        axonAnchor = points->GetNumberOfPoints() - 1;
        glyphPoints->InsertNextPoint(x, y, z);
        glyphAxonAnchor = glyphPoints->GetNumberOfPoints() - 1;
        line            = vtkSmartPointer<vtkLine>::New();
        line->GetPointIds()->SetId(0, axonBranchAnchor);
        line->GetPointIds()->SetId(1, axonAnchor);
        lines->InsertNextCell(line);
        double glyphVector[3] = {
         glyphPoints->GetPoint(glyphAxonAnchor)[0] - glyphPoints->GetPoint(glyphAxonBranchAnchor)[0],
         glyphPoints->GetPoint(glyphAxonAnchor)[1] - glyphPoints->GetPoint(glyphAxonBranchAnchor)[1],
         glyphPoints->GetPoint(glyphAxonAnchor)[2] - glyphPoints->GetPoint(glyphAxonBranchAnchor)[2]};
        glyphVectors->InsertNextTuple(glyphVector);
        glyphTypes->InsertNextValue(axonGlyphType);

        axonboutons = database.exec("SELECT axon_bouton_id, axon_id, x, y, z FROM axonboutons "
                                    "WHERE axon_id = "
                                    + std::to_string(axon_id) + " ORDER BY axon_bouton_id ASC");

        for(auto axonbouton: axonboutons)
        {
            axon_bouton_id = axonbouton[0].as<int>();
            // std::cout << "axon_bouton_id: " << axon_bouton_id << std::endl;
            x = axonbouton[2].as<double>();
            y = axonbouton[3].as<double>();
            z = axonbouton[4].as<double>();
            points->InsertNextPoint(x, y, z);
            axonBoutonAnchor = points->GetNumberOfPoints() - 1;
            glyphPoints->InsertNextPoint(x, y, z);
            glyphAxonBoutonAnchor = glyphPoints->GetNumberOfPoints() - 1;
            line                  = vtkSmartPointer<vtkLine>::New();
            line->GetPointIds()->SetId(0, axonAnchor);
            line->GetPointIds()->SetId(1, axonBoutonAnchor);
            lines->InsertNextCell(line);
            double glyphVector[3] = {
             glyphPoints->GetPoint(glyphAxonBoutonAnchor)[0] - glyphPoints->GetPoint(glyphAxonAnchor)[0],
             glyphPoints->GetPoint(glyphAxonBoutonAnchor)[1] - glyphPoints->GetPoint(glyphAxonAnchor)[1],
             glyphPoints->GetPoint(glyphAxonBoutonAnchor)[2] - glyphPoints->GetPoint(glyphAxonAnchor)[2]};
            glyphVectors->InsertNextTuple(glyphVector);
            glyphTypes->InsertNextValue(axonBoutonGlyphType);

            synapticgaps = database.exec("SELECT synaptic_gap_id, axon_bouton_id, x, y, z "
                                         "FROM synapticgaps WHERE axon_bouton_id = "
                                         + std::to_string(axon_bouton_id) + " ORDER BY synaptic_gap_id ASC");

            for(auto synapticgap: synapticgaps)
            {
                synaptic_gap_id = synapticgap[0].as<int>();
                x               = synapticgap[2].as<double>();
                y               = synapticgap[3].as<double>();
                z               = synapticgap[4].as<double>();
                points->InsertNextPoint(x, y, z);
                synapticGapAnchor = points->GetNumberOfPoints() - 1;
                glyphPoints->InsertNextPoint(x, y, z);
                glyphSynapticGapAnchor = glyphPoints->GetNumberOfPoints() - 1;
                glyphVectors->InsertNextTuple(glyphVectorBlank);
                glyphTypes->InsertNextValue(synapticGapGlyphType);
            }
        }

        for(auto axonbranch: axonbranches)
        {
            axon_branch_id = axonbranch[0].as<int>();
            x              = axonbranch[2].as<double>();
            y              = axonbranch[3].as<double>();
            z              = axonbranch[4].as<double>();
            points->InsertNextPoint(x, y, z);
            axonBranchAnchor = points->GetNumberOfPoints() - 1;
            glyphPoints->InsertNextPoint(x, y, z);
            glyphAxonBranchAnchor = glyphPoints->GetNumberOfPoints() - 1;
            line                  = vtkSmartPointer<vtkLine>::New();
            line->GetPointIds()->SetId(0, axonAnchor);
            line->GetPointIds()->SetId(1, axonBranchAnchor);
            lines->InsertNextCell(line);
            double glyphVector[3] = {
             glyphPoints->GetPoint(glyphAxonBranchAnchor)[0] - glyphPoints->GetPoint(glyphAxonAnchor)[0],
             glyphPoints->GetPoint(glyphAxonBranchAnchor)[1] - glyphPoints->GetPoint(glyphAxonAnchor)[1],
             glyphPoints->GetPoint(glyphAxonBranchAnchor)[2] - glyphPoints->GetPoint(glyphAxonAnchor)[2]};
            glyphVectors->InsertNextTuple(glyphVector);
            glyphTypes->InsertNextValue(axonBranchGlyphType);

            inneraxonbranches = database.exec("SELECT axon_branch_id, axon_id, x, y, z FROM axonbranches "
                                              "WHERE axon_id = "
                                              + std::to_string(axon_id) + " ORDER BY axon_branch_id ASC");
            axon_hillock_id   = -1;
            for(auto inneraxonbranch: inneraxonbranches)
            {
                insertAxons(database,
                            points,
                            lines,
                            glyphPoints,
                            glyphVectors,
                            glyphTypes,
                            axon_branch_id,
                            axon_id,
                            axon_bouton_id,
                            synaptic_gap_id,
                            axon_hillock_id,
                            glyphAxonBranchAnchor);
            }
        }
    }
}

class MyErrorObserver : public vtkCommand
{
    public:
    static MyErrorObserver *New()
    {
        return new MyErrorObserver;
    }

    void Execute(vtkObject *vtkNotUsed(caller), unsigned long event, void *calldata) override
    {
        if(event == vtkCommand::ErrorEvent || event == vtkCommand::WarningEvent)
        {
            spdlog::error(static_cast<char *>(calldata));
        }
    }
};

int main()
{
    try
    {
        // Initialize VTK
        vtkSmartPointer<vtkOutputWindow> myOutputWindow = vtkSmartPointer<vtkOutputWindow>::New();
        vtkOutputWindow::SetInstance(myOutputWindow);
        TRACE0;

        vtkSmartPointer<MyErrorObserver> errorObserver = vtkSmartPointer<MyErrorObserver>::New();
        myOutputWindow->AddObserver(vtkCommand::ErrorEvent, errorObserver);
        myOutputWindow->AddObserver(vtkCommand::WarningEvent, errorObserver);

        // vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
        // vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
        // renderWindow->AddRenderer(renderer);
        // vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
        // vtkSmartPointer<vtkRenderWindowInteractor>::New(); renderWindowInteractor->SetRenderWindow(renderWindow);
        // renderer->SetBackground(0, 0, 0); // Background colour
        // renderWindow->SetSize(1000, 1000); // Window size
        // renderWindow->SetWindowName("AARNN Visualiser"); // Window title

        // Create a vtkCellArray object to hold the lines
        vtkSmartPointer<vtkPoints>            points      = vtkSmartPointer<vtkPoints>::New();
        vtkSmartPointer<vtkPoints>            glyphPoints = vtkSmartPointer<vtkPoints>::New();
        vtkSmartPointer<vtkCellArray>         lines       = vtkSmartPointer<vtkCellArray>::New();
        vtkSmartPointer<vtkUnsignedCharArray> neuronIds   = vtkSmartPointer<vtkUnsignedCharArray>::New();
        neuronIds->SetName("NeuronIds");
        neuronIds->SetNumberOfComponents(1);
        vtkSmartPointer<vtkUnsignedCharArray> glyphTypes = vtkSmartPointer<vtkUnsignedCharArray>::New();
        glyphTypes->SetName("GlyphType");
        glyphTypes->SetNumberOfComponents(1);
        vtkNew<vtkFloatArray> glyphVectors;
        glyphVectors->SetNumberOfComponents(3);
        glyphVectors->SetName("vectors");
        vtkSmartPointer<vtkRenderer>               renderer     = vtkSmartPointer<vtkRenderer>::New();
        vtkSmartPointer<vtkRenderWindow>           renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
        vtkSmartPointer<vtkRenderWindowInteractor> interactor   = vtkSmartPointer<vtkRenderWindowInteractor>::New();

        // Read the database connection configuration and simulation configuration
        Config              config{std::vector<std::string>{"db_connection.conf", "simulation.conf"}};
        std::vector<double> propagationRates;

        // Continuously update the visualization
        while(true)
        {
            long   glyphAxonBranchAnchor = 0;
            double x                     = 0;
            double y                     = 0;
            double z                     = 0;
            int    neuron_id             = 0;
            int    soma_id               = 0;
            int    dendrite_branch_id    = 0;
            int    dendrite_id           = 0;
            int    axon_hillock_id       = 0;
            int    axon_branch_id        = 0;
            int    axon_id               = 0;
            int    synaptic_gap_id       = 0;
            int    axon_bouton_id        = 0;
            int    dendrite_bouton_id    = 0;
            // Map neuron_id to the index of the point in vtkNeurons
            std::unordered_map<int, int> neuronIndexMap;
            // Map soma_id to the index of the point in vtkSomas
            std::unordered_map<int, int> somaIndexMap;
            // Map axon_hillock_id to the index of the point in vtkAxonHillocks
            std::unordered_map<int, int> axonHillockIndexMap;
            // Map axon_id to the index of the point in vtkAxons
            std::unordered_map<int, int> axonIndexMap;
            // Map axon_bouton_id to the index of the point in vtkAxonBoutons
            std::unordered_map<int, int> axonBoutonIndexMap;
            double                       glyphVectorBlank[3] = {0.0, 0.0, 0.0};
            renderWindow->RemoveRenderer(renderer);
            renderer->RemoveAllViewProps();
            glyphTypes->Reset();
            glyphVectors->Reset();
            glyphPoints->Reset();
            lines->Reset();
            points->Reset();
            // Read data from the database
            // Select neurons

            // Connect to PostgreSQL
            DB database{config["POSTGRES_USER"],
                        config["POSTGRES_PASSWORD"],
                        config["POSTGRES_PORT"],
                        config["POSTGRES_HOST"],
                        config["POSTGRES_DB"]};
            // Set the transaction isolation level
            database.startTransaction();
            // TODO: LIMIT the amount of neurons displayed
            auto neurons = database.exec("SELECT neuron_id, x, y, z FROM neurons "
                                         "ORDER BY neuron_id ASC LIMIT 1500");

            int index = 0;
            for(auto neuron: neurons)
            {
                neuron_id = neuron[0].as<int>();
                // std::cout << "Neuron " << neuron_id << std::endl;
                x = neuron[1].as<double>();
                y = neuron[2].as<double>();
                z = neuron[3].as<double>();
                glyphPoints->InsertNextPoint(x, y, z);
                glyphVectors->InsertNextTuple(glyphVectorBlank);
                glyphTypes->InsertNextValue(0);

                // Select somas and create VTK points
                pqxx::result somas = database.exec("SELECT soma_id, neuron_id, x, y, z FROM somas WHERE neuron_id = "
                                                   + std::to_string(neuron_id) + " ORDER BY soma_id ASC");

                // Add the soma points to the points object
                for(auto soma: somas)
                {
                    soma_id = soma[0].as<int>();
                    // std::cout << "Soma " << soma_id << std::endl;
                    x = soma[2].as<double>();
                    y = soma[3].as<double>();
                    z = soma[4].as<double>();
                    glyphPoints->InsertNextPoint(x, y, z);
                    glyphVectors->InsertNextTuple(glyphVectorBlank);
                    glyphTypes->InsertNextValue(1);

                    insertDendriteBranches(database,
                                           points,
                                           lines,
                                           glyphPoints,
                                           glyphVectors,
                                           glyphTypes,
                                           dendrite_branch_id,
                                           dendrite_id,
                                           dendrite_bouton_id,
                                           soma_id);

                    // Add the position of the axon hillock to the vtkPoints and
                    // vtkPolyData
                    // std::cout << "Selecting axon hillocks..." << std::endl;
                    pqxx::result axonhillocks =
                     database.exec("SELECT axon_hillock_id, soma_id, x, y, z FROM axonhillocks "
                                   "WHERE soma_id = "
                                   + std::to_string(soma_id) + " ORDER BY axon_hillock_id ASC");
                    for(auto axonhillock: axonhillocks)
                    {
                        axon_hillock_id = axonhillock[0].as<int>();
                        // std::cout << "axon_hillock_id: " << axon_hillock_id << std::endl;
                        x = axonhillock[2].as<double>();
                        y = axonhillock[3].as<double>();
                        z = axonhillock[4].as<double>();
                        points->InsertNextPoint(x, y, z);
                        glyphPoints->InsertNextPoint(x, y, z);
                        glyphVectors->InsertNextTuple(glyphVectorBlank);
                        glyphTypes->InsertNextValue(1);
                        glyphAxonBranchAnchor = glyphPoints->GetNumberOfPoints() - 1;

                        insertAxons(database,
                                    points,
                                    lines,
                                    glyphPoints,
                                    glyphVectors,
                                    glyphTypes,
                                    axon_branch_id,
                                    axon_id,
                                    axon_bouton_id,
                                    synaptic_gap_id,
                                    axon_hillock_id,
                                    glyphAxonBranchAnchor);
                    }
                }
            }
            std::cout << "Number of points: " << points->GetNumberOfPoints() << std::endl;
            std::cout << "Number of lines: " << lines->GetNumberOfCells() << std::endl;
            std::cout << "Number of glyph vectors: " << glyphVectors->GetNumberOfTuples() << std::endl;
            std::cout << "Number of glyph types: " << glyphTypes->GetNumberOfTuples() << std::endl;

            // Create a polydata object
            vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
            polyData->SetPoints(points);
            polyData->SetLines(lines);
            vtkSmartPointer<vtkPolyData> polyPointData = vtkSmartPointer<vtkPolyData>::New();
            polyPointData->SetPoints(points);

            std::cout << "polyData->GetNumberOfPoints(): " << polyData->GetNumberOfPoints() << std::endl;
            std::cout << "polyData->GetNumberOfLines(): " << polyData->GetNumberOfLines() << std::endl;
            std::cout << "polyPointData->GetNumberOfPoints(): " << polyPointData->GetNumberOfPoints() << std::endl;

            // Create a glyphPolydata object
            vtkSmartPointer<vtkPolyData> glyphPolyData = vtkSmartPointer<vtkPolyData>::New();
            glyphPolyData->SetPoints(glyphPoints);
            glyphPolyData->GetPointData()->AddArray(glyphVectors);
            glyphPolyData->GetPointData()->SetScalars(glyphTypes);

            std::cout << "glyphPolyData->GetNumberOfPoints(): " << glyphPolyData->GetNumberOfPoints() << std::endl;

            // Create glyph sources
            vtkSmartPointer<vtkPoints>   zeroPoints = vtkSmartPointer<vtkPoints>::New();
            vtkSmartPointer<vtkPolyData> emptySource =
             vtkSmartPointer<vtkPolyData>::New();  // Dummy source for no glyph
            vtkSmartPointer<vtkSphereSource>   sphereSource   = vtkSmartPointer<vtkSphereSource>::New();
            vtkSmartPointer<vtkCubeSource>     cubeSource     = vtkSmartPointer<vtkCubeSource>::New();
            vtkSmartPointer<vtkCylinderSource> cylinderSource = vtkSmartPointer<vtkCylinderSource>::New();

            // Update the glyph sources to make sure they have data to draw
            emptySource->SetPoints(zeroPoints);

            sphereSource->SetRadius(0.5);
            sphereSource->Update();

            cubeSource->SetXLength(0.5);
            cubeSource->SetYLength(0.5);
            cubeSource->SetZLength(0.5);
            cubeSource->Update();

            cylinderSource->SetRadius(0.1);
            cylinderSource->Update();

            // Create the Glyph3D filter
            vtkSmartPointer<vtkGlyph3D> glyph3D = vtkSmartPointer<vtkGlyph3D>::New();
            glyph3D->SetOrient(true);
            glyph3D->SetVectorModeToUseVector();
            glyph3D->SetScaleModeToScaleByVector();
            glyph3D->SetScaleFactor(1.0);
            glyph3D->SetInputData(glyphPolyData);
            glyph3D->SetSourceData(0, emptySource);
            glyph3D->SetSourceData(1, cubeSource->GetOutput());
            glyph3D->SetSourceData(2, sphereSource->GetOutput());
            glyph3D->SetSourceData(3, cylinderSource->GetOutput());
            glyph3D->SetSourceData(4, cylinderSource->GetOutput());
            glyph3D->SetSourceData(5, cylinderSource->GetOutput());
            glyph3D->SetSourceData(6, cylinderSource->GetOutput());
            glyph3D->SetSourceData(7, cylinderSource->GetOutput());
            glyph3D->SetSourceData(8, cylinderSource->GetOutput());
            glyph3D->SetSourceData(9, sphereSource->GetOutput());
            glyph3D->SetSourceData(10, sphereSource->GetOutput());
            glyph3D->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "GlyphType");
            glyph3D->Update();

            spdlog::info("glyph3D update finished");

            // Mapper for lines
            vtkSmartPointer<vtkPolyDataMapper> lineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            lineMapper->SetInputData(polyData);

            // Actor for lines
            vtkSmartPointer<vtkActor> lineActor = vtkSmartPointer<vtkActor>::New();
            lineActor->SetMapper(lineMapper);
            // Set the colour for the lines
            lineActor->GetProperty()->SetColor(1.0, 1.0, 0.0);

            // Mapper for glyphs
            vtkSmartPointer<vtkPolyDataMapper> glyphMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            glyphMapper->SetInputConnection(glyph3D->GetOutputPort());

            // Actor for glyphs
            vtkSmartPointer<vtkActor> glyphActor = vtkSmartPointer<vtkActor>::New();
            glyphActor->SetMapper(glyphMapper);

            // Create Delaunay3D filter
            vtkSmartPointer<vtkDelaunay3D> delaunay = vtkSmartPointer<vtkDelaunay3D>::New();
            delaunay->SetInputData(polyPointData);

            // Convert the unstructured grid to polydata
            vtkSmartPointer<vtkGeometryFilter> geometryFilter = vtkSmartPointer<vtkGeometryFilter>::New();
            geometryFilter->SetInputConnection(delaunay->GetOutputPort());
            geometryFilter->Update();

            // Mapper for the membrane
            vtkSmartPointer<vtkPolyDataMapper> membraneMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            membraneMapper->SetInputConnection(geometryFilter->GetOutputPort());

            // Actor for the membrane
            vtkSmartPointer<vtkActor> membraneActor = vtkSmartPointer<vtkActor>::New();
            membraneActor->SetMapper(membraneMapper);
            membraneActor->GetProperty()->SetColor(0.75, 0.75, 0.75);  // Set the colour for the membrane
            membraneActor->GetProperty()->SetOpacity(0.1);             // Set the opacity for the membrane

            // Renderer
            renderer->AddActor(lineActor);
            renderer->AddActor(glyphActor);
            renderer->AddActor(membraneActor);

            // Render window
            renderWindow->AddRenderer(renderer);

            // Interactor
            interactor->SetRenderWindow(renderWindow);

            // Start rendering
            interactor->Start();
        }
    }
    catch(const std::exception &e)
    {
        spdlog::error(e.what());
    }
    catch(...)
    {
        spdlog::error("Error: Unknown exception occurred.");
    }

    return 0;
}
