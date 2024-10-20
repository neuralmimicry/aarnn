// visualiser.cpp
#include "visualiser.h"
#include "Logger.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <string>
#include <stdexcept>
#include <set>
#include <iomanip> // For std::put_time, std::setw, etc.

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkOutputWindow.h>
#include <vtkCommand.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkUnsignedCharArray.h>
#include <vtkLine.h>
#include <vtkSphereSource.h>
#include <vtkCubeSource.h>
#include <vtkCylinderSource.h>
#include <vtkGlyph3D.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkDelaunay3D.h>
#include <vtkGeometryFilter.h>
#include <vtkProperty.h>
#include <vtkDataObject.h>

// Utility Functions

std::map<std::string, std::string> read_config(const std::vector<std::string>& filenames) {
    std::map<std::string, std::string> config;
    for (const auto& filename : filenames) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open config file: " + filename);
        }
        std::string line;
        while (std::getline(file, line)) {
            // Ignore comments and empty lines
            if (line.empty() || line[0] == '#') continue;

            std::istringstream line_stream(line);
            std::string key, value;
            if (std::getline(line_stream, key, '=') && std::getline(line_stream, value)) {
                // Trim whitespace
                key.erase(0, key.find_first_not_of(" \t\r\n"));
                key.erase(key.find_last_not_of(" \t\r\n") + 1);
                value.erase(0, value.find_first_not_of(" \t\r\n"));
                value.erase(value.find_last_not_of(" \t\r\n") + 1);
                config[key] = value;
            }
        }
    }
    return config;
}

std::string build_connection_string(const std::map<std::string, std::string>& config) {
    std::string connection_string;
    const std::set<std::string> allowed_params = {"host", "port", "user", "password", "dbname"};
    for (const auto& [key, value] : config) {
        if (allowed_params.find(key) != allowed_params.end()) {
            connection_string += key + "=" + value + " ";
        }
    }
    return connection_string;
}

// VTK Error Observer

class VTKErrorObserver : public vtkCommand {
public:
    static VTKErrorObserver* New() { return new VTKErrorObserver(); }

    void Execute(vtkObject* caller, unsigned long eventId, void* callData) override {
        if (eventId == vtkCommand::ErrorEvent || eventId == vtkCommand::WarningEvent) {
            const char* message = static_cast<char*>(callData);
            std::cerr << "VTK Error/Warning: " << message << std::endl;
        }
    }
};

// Constructor Implementation
Visualiser::Visualiser(std::shared_ptr<pqxx::connection> conn, Logger& logger)
        : conn_(conn), logger_(logger) {}

// Setup VTK Components
void Visualiser::setupVTK(vtkSmartPointer<vtkRenderer>& renderer,
                          vtkSmartPointer<vtkRenderWindow>& renderWindow,
                          vtkSmartPointer<vtkRenderWindowInteractor>& interactor) {
    renderer = vtkSmartPointer<vtkRenderer>::New();
    renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->AddRenderer(renderer);
    interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    interactor->SetRenderWindow(renderWindow);
}

// Insert Dendrite Branches
void Visualiser::insertDendriteBranches(pqxx::transaction_base& txn,
                                        vtkSmartPointer<vtkPoints>& points,
                                        vtkSmartPointer<vtkCellArray>& lines,
                                        vtkSmartPointer<vtkPoints>& glyphPoints,
                                        vtkSmartPointer<vtkFloatArray>& glyphVectors,
                                        vtkSmartPointer<vtkUnsignedCharArray>& glyphTypes,
                                        int parent_soma_id,
                                        int parent_dendrite_id) {
    try {
        pqxx::result dendritebranches;
        int dendriteBranchGlyphType = 3; // Glyph type for dendrite branch connected to soma
        if (parent_dendrite_id != -1) {
            dendritebranches = txn.exec_params(
                    "SELECT dendrite_branch_id, dendrite_id, x, y, z FROM dendritebranches "
                    "WHERE dendrite_id = $1 ORDER BY dendrite_branch_id ASC",
                    parent_dendrite_id
            );
            dendriteBranchGlyphType = 2; // Glyph type for dendrite branch connected to dendrite
        } else {
            dendritebranches = txn.exec_params(
                    "SELECT dendrite_branch_id, dendrite_id, x, y, z FROM dendritebranches "
                    "WHERE soma_id = $1 ORDER BY dendrite_branch_id ASC",
                    parent_soma_id
            );
            dendriteBranchGlyphType = 3;
        }

        for (const auto& branch : dendritebranches) {
            int dendrite_branch_id = branch[0].as<int>();
            int dendrite_id = branch[1].as<int>();
            double x = branch[2].as<double>();
            double y = branch[3].as<double>();
            double z = branch[4].as<double>();

            // Insert branch point
            points->InsertNextPoint(x, y, z);
            vtkIdType branchAnchor = points->GetNumberOfPoints() - 1;

            // Insert glyph for branch
            glyphPoints->InsertNextPoint(x, y, z);
            glyphVectors->InsertNextTuple3(0.0, 0.0, 0.0); // No vector for branch glyph
            glyphTypes->InsertNextValue(dendriteBranchGlyphType);

            // Retrieve dendrites under this branch
            pqxx::result dendrites = txn.exec_params(
                    "SELECT dendrite_id, x, y, z FROM dendrites "
                    "WHERE dendrite_branch_id = $1 ORDER BY dendrite_id ASC",
                    dendrite_branch_id
            );

            for (const auto& dendrite : dendrites) {
                int dendrite_id_new = dendrite[0].as<int>();
                double dx = dendrite[1].as<double>();
                double dy = dendrite[2].as<double>();
                double dz = dendrite[3].as<double>();

                // Insert dendrite point
                points->InsertNextPoint(dx, dy, dz);
                vtkIdType dendriteAnchor = points->GetNumberOfPoints() - 1;

                // Create line from branch to dendrite
                vtkSmartPointer<vtkLine> dendriteLine = vtkSmartPointer<vtkLine>::New();
                dendriteLine->GetPointIds()->SetId(0, branchAnchor);
                dendriteLine->GetPointIds()->SetId(1, dendriteAnchor);
                lines->InsertNextCell(dendriteLine);

                // Insert glyph for dendrite
                glyphPoints->InsertNextPoint(dx, dy, dz);
                glyphVectors->InsertNextTuple3(dx - x, dy - y, dz - z); // Vector from branch to dendrite
                glyphTypes->InsertNextValue(3); // Glyph type for dendrite

                // Retrieve dendrite boutons
                pqxx::result dendriteboutons = txn.exec_params(
                        "SELECT dendrite_bouton_id, x, y, z FROM dendriteboutons "
                        "WHERE dendrite_id = $1 ORDER BY dendrite_bouton_id ASC",
                        dendrite_id_new
                );

                for (const auto& bouton : dendriteboutons) {
                    int dendrite_bouton_id = bouton[0].as<int>();
                    double bx = bouton[1].as<double>();
                    double by = bouton[2].as<double>();
                    double bz = bouton[3].as<double>();

                    // Insert bouton point
                    points->InsertNextPoint(bx, by, bz);
                    vtkIdType boutonAnchor = points->GetNumberOfPoints() - 1;

                    // Create line from dendrite to bouton
                    vtkSmartPointer<vtkLine> boutonLine = vtkSmartPointer<vtkLine>::New();
                    boutonLine->GetPointIds()->SetId(0, dendriteAnchor);
                    boutonLine->GetPointIds()->SetId(1, boutonAnchor);
                    lines->InsertNextCell(boutonLine);

                    // Insert glyph for bouton
                    glyphPoints->InsertNextPoint(bx, by, bz);
                    glyphVectors->InsertNextTuple3(bx - dx, by - dy, bz - dz); // Vector from dendrite to bouton
                    glyphTypes->InsertNextValue(2); // Glyph type for bouton
                }

                // Recursively insert inner dendrite branches
                insertDendriteBranches(txn, points, lines, glyphPoints, glyphVectors, glyphTypes, -1, dendrite_id_new);
            }
        }
    } catch (const std::exception& e) {
        // Log the exception
        logger_ << "Error inserting dendrite branches: " << e.what() << std::endl;
        std::cerr << "Error inserting dendrite branches: " << e.what() << std::endl;
    }
}

// Insert Axons
void Visualiser::insertAxons(pqxx::transaction_base& txn,
                             vtkSmartPointer<vtkPoints>& points,
                             vtkSmartPointer<vtkCellArray>& lines,
                             vtkSmartPointer<vtkPoints>& glyphPoints,
                             vtkSmartPointer<vtkFloatArray>& glyphVectors,
                             vtkSmartPointer<vtkUnsignedCharArray>& glyphTypes,
                             int parent_axon_id,
                             int parent_axon_branch_id,
                             int parent_axon_hillock_id) {
    try {
        pqxx::result axonbranches;
        int axonBranchGlyphType = 7; // Glyph type for axon branch connected to hillock

        if (parent_axon_branch_id != -1) {
            // Branch connected to another axon branch
            axonbranches = txn.exec_params(
                    "SELECT axon_branch_id, x, y, z FROM axonbranches "
                    "WHERE parent_axon_branch_id = $1 ORDER BY axon_branch_id ASC",
                    parent_axon_branch_id
            );
            axonBranchGlyphType = 6; // Glyph type for axon branch connected to another branch
        } else {
            // Branch connected to axon hillock
            axonbranches = txn.exec_params(
                    "SELECT axon_branch_id, x, y, z FROM axonbranches "
                    "WHERE parent_axon_id = $1 ORDER BY axon_branch_id ASC",
                    parent_axon_id
            );
            axonBranchGlyphType = 7;
        }

        for (const auto& branch : axonbranches) {
            int axon_branch_id = branch[0].as<int>();
            double x = branch[1].as<double>();
            double y = branch[2].as<double>();
            double z = branch[3].as<double>();

            // Insert branch point
            points->InsertNextPoint(x, y, z);
            vtkIdType branchAnchor = points->GetNumberOfPoints() - 1;

            // Insert glyph for branch
            glyphPoints->InsertNextPoint(x, y, z);
            glyphVectors->InsertNextTuple3(0.0, 0.0, 0.0); // No vector for branch glyph
            glyphTypes->InsertNextValue(axonBranchGlyphType);

            // Retrieve axons under this branch
            pqxx::result axons = txn.exec_params(
                    "SELECT axon_id, x, y, z FROM axons "
                    "WHERE axon_branch_id = $1 ORDER BY axon_id ASC",
                    axon_branch_id
            );

            for (const auto& axon : axons) {
                int axon_id_new = axon[0].as<int>();
                double ax = axon[1].as<double>();
                double ay = axon[2].as<double>();
                double az = axon[3].as<double>();

                // Insert axon point
                points->InsertNextPoint(ax, ay, az);
                vtkIdType axonAnchor = points->GetNumberOfPoints() - 1;

                // Create line from branch to axon
                vtkSmartPointer<vtkLine> axonLine = vtkSmartPointer<vtkLine>::New();
                axonLine->GetPointIds()->SetId(0, branchAnchor);
                axonLine->GetPointIds()->SetId(1, axonAnchor);
                lines->InsertNextCell(axonLine);

                // Insert glyph for axon
                glyphPoints->InsertNextPoint(ax, ay, az);
                glyphVectors->InsertNextTuple3(ax - x, ay - y, az - z); // Vector from branch to axon
                glyphTypes->InsertNextValue(8); // Glyph type for axon

                // Retrieve axon boutons
                pqxx::result axonboutons = txn.exec_params(
                        "SELECT axon_bouton_id, x, y, z FROM axonboutons "
                        "WHERE axon_id = $1 ORDER BY axon_bouton_id ASC",
                        axon_id_new
                );

                for (const auto& bouton : axonboutons) {
                    int axon_bouton_id = bouton[0].as<int>();
                    double bx = bouton[1].as<double>();
                    double by = bouton[2].as<double>();
                    double bz = bouton[3].as<double>();

                    // Insert bouton point
                    points->InsertNextPoint(bx, by, bz);
                    vtkIdType boutonAnchor = points->GetNumberOfPoints() - 1;

                    // Create line from axon to bouton
                    vtkSmartPointer<vtkLine> boutonLine = vtkSmartPointer<vtkLine>::New();
                    boutonLine->GetPointIds()->SetId(0, axonAnchor);
                    boutonLine->GetPointIds()->SetId(1, boutonAnchor);
                    lines->InsertNextCell(boutonLine);

                    // Insert glyph for bouton
                    glyphPoints->InsertNextPoint(bx, by, bz);
                    glyphVectors->InsertNextTuple3(bx - ax, by - ay, bz - az); // Vector from axon to bouton
                    glyphTypes->InsertNextValue(9); // Glyph type for bouton

                    // Retrieve synaptic gaps
                    pqxx::result synapticgaps = txn.exec_params(
                            "SELECT synaptic_gap_id, x, y, z FROM synapticgaps "
                            "WHERE axon_bouton_id = $1 ORDER BY synaptic_gap_id ASC",
                            axon_bouton_id
                    );

                    for (const auto& gap : synapticgaps) {
                        int synaptic_gap_id = gap[0].as<int>();
                        double gx = gap[1].as<double>();
                        double gy = gap[2].as<double>();
                        double gz = gap[3].as<double>();

                        // Insert synaptic gap point
                        points->InsertNextPoint(gx, gy, gz);
                        vtkIdType gapAnchor = points->GetNumberOfPoints() - 1;

                        // Create line from bouton to synaptic gap
                        vtkSmartPointer<vtkLine> gapLine = vtkSmartPointer<vtkLine>::New();
                        gapLine->GetPointIds()->SetId(0, boutonAnchor);
                        gapLine->GetPointIds()->SetId(1, gapAnchor);
                        lines->InsertNextCell(gapLine);

                        // Insert glyph for synaptic gap
                        glyphPoints->InsertNextPoint(gx, gy, gz);
                        glyphVectors->InsertNextTuple3(0.0, 0.0, 0.0); // No vector for synaptic gap glyph
                        glyphTypes->InsertNextValue(10); // Glyph type for synaptic gap
                    }
                }

                // Recursively insert axon branches
                insertAxons(txn, points, lines, glyphPoints, glyphVectors, glyphTypes, axon_id_new, axon_branch_id, parent_axon_hillock_id);
            }
        }
    } catch (const std::exception& e) {
        // Log the exception
        logger_ << "Error inserting axon branches: " << e.what() << std::endl;
        std::cerr << "Error inserting axon branches: " << e.what() << std::endl;
    }
}

// Visualisation Method
void Visualiser::visualise() {
    try {
        // Initialize VTK components
        vtkSmartPointer<vtkRenderer> renderer;
        vtkSmartPointer<vtkRenderWindow> renderWindow;
        vtkSmartPointer<vtkRenderWindowInteractor> interactor;
        setupVTK(renderer, renderWindow, interactor);

        // Setup VTK error observer
        vtkSmartPointer<VTKErrorObserver> errorObserver = vtkSmartPointer<VTKErrorObserver>::New();
        vtkSmartPointer<vtkOutputWindow> outputWindow = vtkSmartPointer<vtkOutputWindow>::New();
        vtkOutputWindow::SetInstance(outputWindow);
        outputWindow->AddObserver(vtkCommand::ErrorEvent, errorObserver);
        outputWindow->AddObserver(vtkCommand::WarningEvent, errorObserver);

        // Infinite loop for continuous visualisation updates
        while (true) {
            // Reset VTK structures
            vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
            vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
            vtkSmartPointer<vtkPoints> glyphPoints = vtkSmartPointer<vtkPoints>::New();
            vtkSmartPointer<vtkFloatArray> glyphVectors = vtkSmartPointer<vtkFloatArray>::New();
            glyphVectors->SetNumberOfComponents(3);
            glyphVectors->SetName("Vectors");
            vtkSmartPointer<vtkUnsignedCharArray> glyphTypes = vtkSmartPointer<vtkUnsignedCharArray>::New();
            glyphTypes->SetName("GlyphType");
            glyphTypes->SetNumberOfComponents(1);

            // Start database transaction
            pqxx::work txn(*conn_);

            // Retrieve neurons with additional fields
            pqxx::result neurons = txn.exec(
                    "SELECT neuron_id, x, y, z, propagation_rate, neuron_type, axon_length "
                    "FROM neurons ORDER BY neuron_id ASC LIMIT 1500"
            );

            for (const auto& neuron : neurons) {
                if (neuron.size() != 6) {
                    std:cout << "Neuron has incorrect number of fields." << std::endl;
                    continue;
                }
                if (neuron[0].is_null() || neuron[1].is_null() || neuron[2].is_null() || neuron[3].is_null() ||
                    neuron[4].is_null() || neuron[5].is_null()) {
                    std::cout << "Neuron has NULL fields." << std::endl;
                    continue;
                }
                int neuron_id = neuron[0].as<int>();
                double nx = neuron[1].as<double>();
                double ny = neuron[2].as<double>();
                double nz = neuron[3].as<double>();
                double propagation_rate = neuron[4].as<double>();
                int neuron_type = neuron[5].as<int>();

                // Insert neuron glyph
                glyphPoints->InsertNextPoint(nx, ny, nz);
                glyphVectors->InsertNextTuple3(0.0, 0.0, 0.0); // No vector for neuron glyph
                glyphTypes->InsertNextValue(1); // Glyph type for neuron

                // Retrieve somas for this neuron
                pqxx::result somas = txn.exec_params(
                        "SELECT soma_id, x, y, z FROM somas "
                        "WHERE neuron_id = $1 ORDER BY soma_id ASC",
                        neuron_id
                );

                for (const auto& soma : somas) {
                    if (soma.size() != 4) {
                        std::cout << "Soma has incorrect number of fields." << std::endl;
                        continue;
                    }
                    if (soma[0].is_null() || soma[1].is_null() || soma[2].is_null() || soma[3].is_null()) {
                        std::cout << "Soma has NULL fields." << std::endl;
                        continue;
                    }
                    int soma_id = soma[0].as<int>();
                    double sx = soma[1].as<double>();
                    double sy = soma[2].as<double>();
                    double sz = soma[3].as<double>();

                    // Insert soma glyph
                    glyphPoints->InsertNextPoint(sx, sy, sz);
                    glyphVectors->InsertNextTuple3(0.0, 0.0, 0.0); // No vector for soma glyph
                    glyphTypes->InsertNextValue(4); // Glyph type for soma

                    // Insert dendrite branches
                    insertDendriteBranches(txn, points, lines, glyphPoints, glyphVectors, glyphTypes, soma_id);

                    // Retrieve axon hillocks for this soma
                    pqxx::result axonhillocks = txn.exec_params(
                            "SELECT axon_hillock_id, x, y, z FROM axonhillocks "
                            "WHERE soma_id = $1 ORDER BY axon_hillock_id ASC",
                            soma_id
                    );

                    for (const auto& hillock : axonhillocks) {
                        if (hillock.size() != 4) {
                            std::cout << "Axon hillock has incorrect number of fields." << std::endl;
                            continue;
                        }
                        if (hillock[0].is_null() || hillock[1].is_null() || hillock[2].is_null() || hillock[3].is_null()) {
                            std::cout << "Axon hillock has NULL fields." << std::endl;
                            continue;
                        }
                        int axon_hillock_id = hillock[0].as<int>();
                        double ahx = hillock[1].as<double>();
                        double ahy = hillock[2].as<double>();
                        double ahz = hillock[3].as<double>();

                        // Insert axon hillock glyph
                        points->InsertNextPoint(ahx, ahy, ahz);
                        vtkIdType hillockAnchor = points->GetNumberOfPoints() - 1;

                        glyphPoints->InsertNextPoint(ahx, ahy, ahz);
                        glyphVectors->InsertNextTuple3(0.0, 0.0, 0.0); // No vector for hillock glyph
                        glyphTypes->InsertNextValue(5); // Glyph type for axon hillock

                        // Retrieve axons connected to this hillock
                        pqxx::result axons = txn.exec_params(
                                "SELECT axon_id, x, y, z FROM axons "
                                "WHERE axon_hillock_id = $1 ORDER BY axon_id ASC",
                                axon_hillock_id
                        );

                        for (const auto& axon : axons) {
                            if (axon.size() != 4) {
                                std::cout << "Axon has incorrect number of fields." << std::endl;
                                continue;
                            }
                            if (axon[0].is_null() || axon[1].is_null() || axon[2].is_null() || axon[3].is_null()) {
                                std::cout << "Axon has NULL fields." << std::endl;
                                continue;
                            }
                            int axon_id = axon[0].as<int>();
                            double ax = axon[1].as<double>();
                            double ay = axon[2].as<double>();
                            double az = axon[3].as<double>();

                            // Insert axon point
                            points->InsertNextPoint(ax, ay, az);
                            vtkIdType axonAnchor = points->GetNumberOfPoints() - 1;

                            // Create line from hillock to axon
                            vtkSmartPointer<vtkLine> axonLine = vtkSmartPointer<vtkLine>::New();
                            axonLine->GetPointIds()->SetId(0, hillockAnchor);
                            axonLine->GetPointIds()->SetId(1, axonAnchor);
                            lines->InsertNextCell(axonLine);

                            // Insert glyph for axon
                            glyphPoints->InsertNextPoint(ax, ay, az);
                            glyphVectors->InsertNextTuple3(ax - ahx, ay - ahy, az - ahz); // Vector from hillock to axon
                            glyphTypes->InsertNextValue(8); // Glyph type for axon

                            // Insert axon boutons and synaptic gaps
                            pqxx::result axonboutons = txn.exec_params(
                                    "SELECT axon_bouton_id, x, y, z FROM axonboutons "
                                    "WHERE axon_id = $1 ORDER BY axon_bouton_id ASC",
                                    axon_id
                            );

                            for (const auto& bouton : axonboutons) {
                                if (bouton.size() != 4) {
                                    std::cout << "Axon bouton has incorrect number of fields." << std::endl;
                                    continue;
                                }
                                if (bouton[0].is_null() || bouton[1].is_null() || bouton[2].is_null() || bouton[3].is_null()) {
                                    std::cout << "Axon bouton has NULL fields." << std::endl;
                                    continue;
                                }
                                int axon_bouton_id = bouton[0].as<int>();
                                double bx = bouton[1].as<double>();
                                double by = bouton[2].as<double>();
                                double bz = bouton[3].as<double>();

                                // Insert bouton point
                                points->InsertNextPoint(bx, by, bz);
                                vtkIdType boutonAnchor = points->GetNumberOfPoints() - 1;

                                // Create line from axon to bouton
                                vtkSmartPointer<vtkLine> boutonLine = vtkSmartPointer<vtkLine>::New();
                                boutonLine->GetPointIds()->SetId(0, axonAnchor);
                                boutonLine->GetPointIds()->SetId(1, boutonAnchor);
                                lines->InsertNextCell(boutonLine);

                                // Insert glyph for bouton
                                glyphPoints->InsertNextPoint(bx, by, bz);
                                glyphVectors->InsertNextTuple3(bx - ax, by - ay, bz - az); // Vector from axon to bouton
                                glyphTypes->InsertNextValue(9); // Glyph type for bouton

                                // Retrieve synaptic gaps
                                pqxx::result synapticgaps = txn.exec_params(
                                        "SELECT synaptic_gap_id, x, y, z FROM synapticgaps "
                                        "WHERE axon_bouton_id = $1 ORDER BY synaptic_gap_id ASC",
                                        axon_bouton_id
                                );

                                for (const auto& gap : synapticgaps) {
                                    if (gap.size() != 4) {
                                        std::cout << "Synaptic gap has incorrect number of fields." << std::endl;
                                        continue;
                                    }
                                    if (gap[0].is_null() || gap[1].is_null() || gap[2].is_null() || gap[3].is_null()) {
                                        std::cout << "Synaptic gap has NULL fields." << std::endl;
                                        continue;
                                    }
                                    int synaptic_gap_id = gap[0].as<int>();
                                    double gx = gap[1].as<double>();
                                    double gy = gap[2].as<double>();
                                    double gz = gap[3].as<double>();

                                    // Insert synaptic gap point
                                    points->InsertNextPoint(gx, gy, gz);
                                    vtkIdType gapAnchor = points->GetNumberOfPoints() - 1;

                                    // Create line from bouton to synaptic gap
                                    vtkSmartPointer<vtkLine> gapLine = vtkSmartPointer<vtkLine>::New();
                                    gapLine->GetPointIds()->SetId(0, boutonAnchor);
                                    gapLine->GetPointIds()->SetId(1, gapAnchor);
                                    lines->InsertNextCell(gapLine);

                                    // Insert glyph for synaptic gap
                                    glyphPoints->InsertNextPoint(gx, gy, gz);
                                    glyphVectors->InsertNextTuple3(0.0, 0.0, 0.0); // No vector for synaptic gap glyph
                                    glyphTypes->InsertNextValue(10); // Glyph type for synaptic gap
                                }
                            }

                            // Recursively insert axon branches
                            insertAxons(txn, points, lines, glyphPoints, glyphVectors, glyphTypes, axon_id, -1, axon_hillock_id);
                        }
                    }
                }
            }

            // Commit transaction
            txn.commit();

            // Prepare VTK PolyData
            vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
            polyData->SetPoints(points);
            polyData->SetLines(lines);

            vtkSmartPointer<vtkPolyData> glyphPolyData = vtkSmartPointer<vtkPolyData>::New();
            glyphPolyData->SetPoints(glyphPoints);
            glyphPolyData->GetPointData()->AddArray(glyphVectors);
            glyphPolyData->GetPointData()->SetScalars(glyphTypes);

            // Setup Glyph Sources
            vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
            sphereSource->SetRadius(0.5);
            sphereSource->Update();

            vtkSmartPointer<vtkCubeSource> cubeSource = vtkSmartPointer<vtkCubeSource>::New();
            cubeSource->SetXLength(0.5);
            cubeSource->SetYLength(0.5);
            cubeSource->SetZLength(0.5);
            cubeSource->Update();

            vtkSmartPointer<vtkCylinderSource> cylinderSource = vtkSmartPointer<vtkCylinderSource>::New();
            cylinderSource->SetRadius(0.1);
            cylinderSource->SetHeight(1.0);
            cylinderSource->Update();

            // Setup Glyph3D
            vtkSmartPointer<vtkGlyph3D> glyph3D = vtkSmartPointer<vtkGlyph3D>::New();
            glyph3D->SetOrient(true);
            glyph3D->SetVectorModeToUseVector();
            glyph3D->SetScaleModeToScaleByVector();
            glyph3D->SetScaleFactor(1.0);
            glyph3D->SetInputData(glyphPolyData);
            glyph3D->SetSourceConnection(0, sphereSource->GetOutputPort());
            glyph3D->SetSourceConnection(1, cubeSource->GetOutputPort());
            glyph3D->SetSourceConnection(2, cylinderSource->GetOutputPort());
            glyph3D->SetSourceConnection(3, cylinderSource->GetOutputPort());
            glyph3D->SetSourceConnection(4, cylinderSource->GetOutputPort());
            glyph3D->SetSourceConnection(5, cylinderSource->GetOutputPort());
            glyph3D->SetSourceConnection(6, cylinderSource->GetOutputPort());
            glyph3D->SetSourceConnection(7, cylinderSource->GetOutputPort());
            glyph3D->SetSourceConnection(8, cylinderSource->GetOutputPort());
            glyph3D->SetSourceConnection(9, sphereSource->GetOutputPort());
            glyph3D->SetSourceConnection(10, sphereSource->GetOutputPort());

            // Assign glyph types to corresponding sources
            glyph3D->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "GlyphType");
            glyph3D->Update();

            // Setup Line Mapper and Actor
            vtkSmartPointer<vtkPolyDataMapper> lineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            lineMapper->SetInputData(polyData);
            vtkSmartPointer<vtkActor> lineActor = vtkSmartPointer<vtkActor>::New();
            lineActor->SetMapper(lineMapper);
            lineActor->GetProperty()->SetColor(1.0, 1.0, 0.0); // Yellow lines

            // Setup Glyph Mapper and Actor
            vtkSmartPointer<vtkPolyDataMapper> glyphMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            glyphMapper->SetInputConnection(glyph3D->GetOutputPort());
            vtkSmartPointer<vtkActor> glyphActor = vtkSmartPointer<vtkActor>::New();
            glyphActor->SetMapper(glyphMapper);

            // Setup Membrane (Delaunay Triangulation)
            vtkSmartPointer<vtkDelaunay3D> delaunay = vtkSmartPointer<vtkDelaunay3D>::New();
            delaunay->SetInputData(polyData);
            delaunay->Update();

            vtkSmartPointer<vtkGeometryFilter> geometryFilter = vtkSmartPointer<vtkGeometryFilter>::New();
            geometryFilter->SetInputConnection(delaunay->GetOutputPort());
            geometryFilter->Update();

            vtkSmartPointer<vtkPolyDataMapper> membraneMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            membraneMapper->SetInputConnection(geometryFilter->GetOutputPort());

            vtkSmartPointer<vtkActor> membraneActor = vtkSmartPointer<vtkActor>::New();
            membraneActor->SetMapper(membraneMapper);
            membraneActor->GetProperty()->SetColor(0.75, 0.75, 0.75); // Grey membrane
            membraneActor->GetProperty()->SetOpacity(0.1); // Semi-transparent

            // Update Renderer
            renderer->RemoveAllViewProps();
            renderer->AddActor(lineActor);
            renderer->AddActor(glyphActor);
            renderer->AddActor(membraneActor);
            renderWindow->Render();

            // Start Interactor
            interactor->Initialize();
            interactor->Start();

            // Sleep for a defined interval before the next update
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }

    } catch (const std::exception& e) {
        // Log the exception
        logger_ << "Visualisation Error: " << e.what() << std::endl;
        std::cerr << "Visualisation Error: " << e.what() << std::endl;
    }
}

// Main Function

int main() {
    try {
        // Initialize Logger
        Logger logger("errors_visualiser.log");

        // Read Configuration Files
        std::vector<std::string> config_files = {"db_connection.conf", "simulation.conf"};
        auto config = read_config(config_files);
        std::string connection_str = build_connection_string(config);

        // Initialize Database Connection
        std::shared_ptr<pqxx::connection> conn = std::make_shared<pqxx::connection>(connection_str);
        if (!conn->is_open()) {
            throw std::runtime_error("Unable to open database connection.");
        }
        logger << "Successfully connected to the database." << std::endl;
        std::cout << "Connected to PostgreSQL database." << std::endl;

        // Initialize and Run Visualiser
        Visualiser visualiser(conn, logger);
        visualiser.visualise();

    } catch (const pqxx::sql_error& e) {
        std::cerr << "SQL Error: " << e.what() << std::endl;
        std::cerr << "Query was: " << e.query() << std::endl;
        // Optionally log the error
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        // Optionally log the error
    } catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
        // Optionally log the error
    }

    return 0;
}
