// visualiser.cpp

#include "visualiser.h"
#include "Logger.h"
#include "wss.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <chrono>
#include <string>
#include <stdexcept>
#include <set>
#include <iomanip> // For std::put_time, std::setw, etc.

#include <pqxx/pqxx>                  // libpqxx for PostgreSQL access
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#pragma message("JSON.hpp is coming from: " __FILE__)
#include <boost/json.hpp>            // Boost.JSON (if needed for future JSON messages)

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkOutputWindow.h>
#include <vtkCommand.h>
#include <vtkCallbackCommand.h>
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
#include <vtkProperty.h>
#include <vtkDataObject.h>

namespace {

//------------------------------------------------------------------------------
// VTKErrorObserver:
// A small helper class to catch VTK warnings/errors and print them to stderr.
//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
// UpdateTimerCallback:
// A VTK timer callback that calls Visualiser::buildAndRenderFrame() each interval.
//------------------------------------------------------------------------------
    class UpdateTimerCallback : public vtkCallbackCommand {
    public:
        static UpdateTimerCallback* New() {
            return new UpdateTimerCallback;
        }

        // Link to the Visualiser instance
        void SetVisualiser(Visualiser* vis) { this->Vis = vis; }

        // Interval in milliseconds between updates
        void SetInterval(int ms) { this->IntervalMs = ms; }

        // Called whenever the VTK interactor fires a TimerEvent
        void Execute(vtkObject* caller, unsigned long eventId, void* /*callData*/) override {
            if (eventId != vtkCommand::TimerEvent) {
                return;
            }
            if (!Vis) {
                return;
            }

            // Rebuild pipeline and re-render
            Vis->buildAndRenderFrame();

            // Reschedule the next timer
            vtkRenderWindowInteractor* iren = static_cast<vtkRenderWindowInteractor*>(caller);
            iren->CreateRepeatingTimer(IntervalMs);
        }

    private:
        Visualiser* Vis = nullptr;
        int IntervalMs = 5000; // default 5 seconds
    };

} // namespace (anonymous)

namespace json = boost::json;

//------------------------------------------------------------------------------
// Visualiser constructor:
//   - Stores the shared_ptr to pqxx::connection
//   - References to Logger and WebSocketServer
//------------------------------------------------------------------------------
Visualiser::Visualiser(std::shared_ptr<pqxx::connection> conn,
                       Logger& logger,
                       WebSocketServer& ws_server)
        : conn_(std::move(conn)), logger_(logger), ws_server_(ws_server)
{
    // Nothing else to do here; most initialization is deferred to visualise().
}

//------------------------------------------------------------------------------
// insertDendriteBranches:
// Recursively query dendrite branches under a given parent (soma or dendrite_id),
// append points, lines, glyphs, and colors to the appropriate VTK arrays.
//------------------------------------------------------------------------------
void Visualiser::insertDendriteBranches(pqxx::transaction_base& txn,
                                        vtkSmartPointer<vtkPoints>& points,
                                        vtkSmartPointer<vtkCellArray>& lines,
                                        vtkSmartPointer<vtkPoints>& glyphPoints,
                                        vtkSmartPointer<vtkFloatArray>& glyphVectors,
                                        vtkSmartPointer<vtkUnsignedCharArray>& glyphTypes,
                                        vtkSmartPointer<vtkUnsignedCharArray>& glyphColors,
                                        int parent_soma_id,
                                        int parent_dendrite_id)
{
    try {
        pqxx::result dendritebranches;
        int branchGlyphType = 3; // Glyph type index for dendrite branch

        // Choose which query to run based on which parent ID is valid
        if (parent_dendrite_id != -1) {
            // Dendrite branches attached to another dendrite
            dendritebranches = txn.exec_params(
                    "SELECT dendrite_branch_id, dendrite_id, x, y, z, energy_level, max_energy_level "
                    "FROM dendritebranches "
                    "WHERE dendrite_id = $1 ORDER BY dendrite_branch_id ASC",
                    parent_dendrite_id
            );
        }
        else if (parent_soma_id != -1) {
            // Dendrite branches attached to a soma
            dendritebranches = txn.exec_params(
                    "SELECT dendrite_branch_id, dendrite_id, x, y, z, energy_level, max_energy_level "
                    "FROM dendritebranches "
                    "WHERE soma_id = $1 ORDER BY dendrite_branch_id ASC",
                    parent_soma_id
            );
        }
        else {
            // No valid parent: nothing to insert
            return;
        }

        // Iterate over each dendrite branch record
        for (const auto& branch : dendritebranches) {
            int dendrite_branch_id = branch[0].as<int>();
            int dendrite_id        = branch[1].is_null() ? -1 : branch[1].as<int>();
            double x               = branch[2].as<double>();
            double y               = branch[3].as<double>();
            double z               = branch[4].as<double>();
            double energy_level    = branch[5].as<double>();
            double max_energy_level = branch[6].is_null() ? 100.0 : branch[6].as<double>();

            // 1) Insert branch endpoint into 'points'
            points->InsertNextPoint(x, y, z);
            vtkIdType branchAnchor = points->GetNumberOfPoints() - 1;

            // 2) Insert glyph for the branch itself
            glyphPoints->InsertNextPoint(x, y, z);
            glyphVectors->InsertNextTuple3(0.0, 0.0, 0.0); // no vector for branch center
            glyphTypes->InsertNextValue(branchGlyphType);

            // Compute RGB color based on energy_level [0..100]
            double speedCalculation = (255.0 / max_energy_level) * energy_level; // Scale to 255
            unsigned char R = static_cast<unsigned char>(speedCalculation);
            unsigned char G = 0;
            unsigned char B = static_cast<unsigned char>(255.0 - speedCalculation);
            unsigned char color[3] = { R, G, B };
            glyphColors->InsertNextTypedTuple(color);

            // 3) Now fetch all dendrites under this branch
            pqxx::result dendrites = txn.exec_params(
                    "SELECT dendrite_id, x, y, z, energy_level, max_energy_level "
                    "FROM dendrites "
                    "WHERE dendrite_branch_id = $1 ORDER BY dendrite_id ASC",
                    dendrite_branch_id
            );

            for (const auto& dendrite : dendrites) {
                int dendrite_id_new = dendrite[0].as<int>();
                double dx = dendrite[1].as<double>();
                double dy = dendrite[2].as<double>();
                double dz = dendrite[3].as<double>();
                double denergy_level = dendrite[4].as<double>();
                double dmax_energy_level = dendrite[5].is_null() ? 100.0 : dendrite[5].as<double>();

                // Insert dendrite endpoint
                points->InsertNextPoint(dx, dy, dz);
                vtkIdType dendriteAnchor = points->GetNumberOfPoints() - 1;

                // Create a line from branch to dendrite
                vtkSmartPointer<vtkLine> dendriteLine = vtkSmartPointer<vtkLine>::New();
                dendriteLine->GetPointIds()->SetId(0, branchAnchor);
                dendriteLine->GetPointIds()->SetId(1, dendriteAnchor);
                lines->InsertNextCell(dendriteLine);

                // Insert glyph for dendrite
                glyphPoints->InsertNextPoint(dx, dy, dz);
                glyphVectors->InsertNextTuple3(dx - x, dy - y, dz - z);
                glyphTypes->InsertNextValue(2); // Glyph type index for dendrite

                double speedCalculation = (255.0 / dmax_energy_level) * denergy_level; // Scale to 255
                unsigned char R_d = static_cast<unsigned char>(speedCalculation);
                unsigned char G_d = 0;
                unsigned char B_d = static_cast<unsigned char>(255.0 - speedCalculation);
                unsigned char color_d[3] = { R_d, G_d, B_d };
                glyphColors->InsertNextTypedTuple(color_d);

                // 4) Fetch all dendrite boutons under this dendrite
                pqxx::result boutons = txn.exec_params(
                        "SELECT dendrite_bouton_id, x, y, z, energy_level, max_energy_level "
                        "FROM dendriteboutons "
                        "WHERE dendrite_id = $1 ORDER BY dendrite_bouton_id ASC",
                        dendrite_id_new
                );

                for (const auto& bouton : boutons) {
                    int bouton_id       = bouton[0].as<int>();
                    double bx           = bouton[1].as<double>();
                    double by           = bouton[2].as<double>();
                    double bz           = bouton[3].as<double>();
                    double benergy_level = bouton[4].as<double>();
                    double bmax_energy_level = bouton[5].is_null() ? 100.0 : bouton[5].as<double>();

                    // Insert bouton point
                    points->InsertNextPoint(bx, by, bz);
                    vtkIdType boutonAnchor = points->GetNumberOfPoints() - 1;

                    // Line from dendrite to bouton
                    vtkSmartPointer<vtkLine> boutonLine = vtkSmartPointer<vtkLine>::New();
                    boutonLine->GetPointIds()->SetId(0, dendriteAnchor);
                    boutonLine->GetPointIds()->SetId(1, boutonAnchor);
                    lines->InsertNextCell(boutonLine);

                    // Insert glyph for bouton
                    glyphPoints->InsertNextPoint(bx, by, bz);
                    glyphVectors->InsertNextTuple3(bx - dx, by - dy, bz - dz);
                    glyphTypes->InsertNextValue(2); // reuse bouton glyph index (could be distinct)

                    double speedCalculation = (255.0 / bmax_energy_level) * benergy_level; // Scale to 255
                    unsigned char R_b = static_cast<unsigned char>(speedCalculation);
                    unsigned char G_b = 0;
                    unsigned char B_b = static_cast<unsigned char>(255.0 - speedCalculation);
                    unsigned char color_b[3] = { R_b, G_b, B_b };
                    glyphColors->InsertNextTypedTuple(color_b);
                }

                // 5) Recursively insert nested dendrite branches
                insertDendriteBranches(txn,
                                       points, lines,
                                       glyphPoints, glyphVectors, glyphTypes, glyphColors,
                                       -1,             // no direct soma parent
                                       dendrite_id_new // pass current dendrite as new parent
                );
            }
        }
    }
    catch (const std::exception& e) {
        // Log any errors encountered during recursion
        logger_ << "Error in insertDendriteBranches: " << e.what() << std::endl;
        std::cerr << "Error in insertDendriteBranches: " << e.what() << std::endl;
    }
}

//------------------------------------------------------------------------------
// insertAxons:
// Recursively query axon branches and their attached axons, boutons, synaptic gaps.
// Appends points, lines, glyphs, and colors to the VTK arrays.
//------------------------------------------------------------------------------
void Visualiser::insertAxons(pqxx::transaction_base& txn,
                             vtkSmartPointer<vtkPoints>& points,
                             vtkSmartPointer<vtkCellArray>& lines,
                             vtkSmartPointer<vtkPoints>& glyphPoints,
                             vtkSmartPointer<vtkFloatArray>& glyphVectors,
                             vtkSmartPointer<vtkUnsignedCharArray>& glyphTypes,
                             vtkSmartPointer<vtkUnsignedCharArray>& glyphColors,
                             int parent_axon_id,
                             int parent_axon_branch_id,
                             int parent_axon_hillock_id)
{
    try {
        pqxx::result axonbranches;

        // Determine which query to run based on valid parent identifier
        if (parent_axon_branch_id != -1) {
            // Axon branches under another axon branch
            axonbranches = txn.exec_params(
                    "SELECT axon_branch_id, x, y, z, energy_level, max_energy_level "
                    "FROM axonbranches "
                    "WHERE parent_axon_branch_id = $1 ORDER BY axon_branch_id ASC",
                    parent_axon_branch_id
            );
        }
        else if (parent_axon_id != -1) {
            // Axon branches directly under an axon
            axonbranches = txn.exec_params(
                    "SELECT axon_branch_id, x, y, z, energy_level, max_energy_level "
                    "FROM axonbranches "
                    "WHERE parent_axon_id = $1 ORDER BY axon_branch_id ASC",
                    parent_axon_id
            );
        }
        else if (parent_axon_hillock_id != -1) {
            // Axon branches under an axon hillock
            axonbranches = txn.exec_params(
                    "SELECT ab.axon_branch_id, ab.x, ab.y, ab.z, ab.energy_level, ab.max_energy_level "
                    "FROM axonbranches AS ab "
                    "JOIN axons ON ab.parent_axon_id = axons.axon_id "
                    "WHERE axons.axon_hillock_id = $1 ORDER BY ab.axon_branch_id ASC",
                    parent_axon_hillock_id
            );
        }
        else {
            // No valid parent: nothing to insert
            return;
        }

        // Iterate over each axon branch
        for (const auto& branch : axonbranches) {
            int axon_branch_id = branch[0].as<int>();
            double x           = branch[1].as<double>();
            double y           = branch[2].as<double>();
            double z           = branch[3].as<double>();
            double energy_level = branch[4].as<double>();
            double max_energy_level = branch[5].is_null() ? 100.0 : branch[5].as<double>(); // default to 100 if null

            // 1) Insert branch point
            points->InsertNextPoint(x, y, z);
            vtkIdType branchAnchor = points->GetNumberOfPoints() - 1;

            // 2) Insert glyph for the branch
            glyphPoints->InsertNextPoint(x, y, z);
            glyphVectors->InsertNextTuple3(0.0, 0.0, 0.0); // no vector for branch
            glyphTypes->InsertNextValue(7); // glyph index for axon branch

            double speedCalculation = (255.0 / max_energy_level) * energy_level; // Scale to 255
            unsigned char R = static_cast<unsigned char>(speedCalculation);
            unsigned char G = 0;
            unsigned char B = static_cast<unsigned char>(255.0 - speedCalculation);
            unsigned char color[3] = { R, G, B };
            glyphColors->InsertNextTypedTuple(color);

            // 3) Query all axons under this branch
            pqxx::result axons = txn.exec_params(
                    "SELECT axon_id, x, y, z, energy_level, max_energy_level "
                    "FROM axons "
                    "WHERE axon_branch_id = $1 ORDER BY axon_id ASC",
                    axon_branch_id
            );

            for (const auto& axon : axons) {
                int axon_id_new        = axon[0].as<int>();
                double ax              = axon[1].as<double>();
                double ay              = axon[2].as<double>();
                double az              = axon[3].as<double>();
                double aenergy_level   = axon[4].as<double>();
                double amax_energy_level = axon[5].is_null() ? 100.0 : axon[5].as<double>(); // default to 100 if null

                // Insert axon endpoint
                points->InsertNextPoint(ax, ay, az);
                vtkIdType axonAnchor = points->GetNumberOfPoints() - 1;

                // Create line from branch to axon
                vtkSmartPointer<vtkLine> axonLine = vtkSmartPointer<vtkLine>::New();
                axonLine->GetPointIds()->SetId(0, branchAnchor);
                axonLine->GetPointIds()->SetId(1, axonAnchor);
                lines->InsertNextCell(axonLine);

                // Insert glyph for axon
                glyphPoints->InsertNextPoint(ax, ay, az);
                glyphVectors->InsertNextTuple3(ax - x, ay - y, az - z);
                glyphTypes->InsertNextValue(8); // glyph index for axon

                double speedCalculation = (255.0 / amax_energy_level) * aenergy_level; // Scale to 255
                unsigned char R_a = static_cast<unsigned char>(speedCalculation);
                unsigned char G_a = 0;
                unsigned char B_a = static_cast<unsigned char>(255.0 - speedCalculation);
                unsigned char color_a[3] = { R_a, G_a, B_a };
                glyphColors->InsertNextTypedTuple(color_a);

                // 4) Query all axon boutons under this axon
                pqxx::result boutons = txn.exec_params(
                        "SELECT axon_bouton_id, x, y, z, energy_level, max_energy_level "
                        "FROM axonboutons "
                        "WHERE axon_id = $1 ORDER BY axon_bouton_id ASC",
                        axon_id_new
                );

                for (const auto& bouton : boutons) {
                    int bouton_id        = bouton[0].as<int>();
                    double bx            = bouton[1].as<double>();
                    double by            = bouton[2].as<double>();
                    double bz            = bouton[3].as<double>();
                    double benergy_level  = bouton[4].as<double>();
                    double bmax_energy_level = bouton[5].is_null() ? 100.0 : bouton[5].as<double>(); // default to 100 if null

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
                    glyphVectors->InsertNextTuple3(bx - ax, by - ay, bz - az);
                    glyphTypes->InsertNextValue(9); // glyph index for axon bouton

                    double speedCalculation = (255.0 / bmax_energy_level) * benergy_level; // Scale to 255
                    unsigned char R_b = static_cast<unsigned char>(speedCalculation);
                    unsigned char G_b = 0;
                    unsigned char B_b = static_cast<unsigned char>(255.0 - speedCalculation);
                    unsigned char color_b[3] = { R_b, G_b, B_b };
                    glyphColors->InsertNextTypedTuple(color_b);

                    // 5) Query all synaptic gaps under this bouton
                    pqxx::result synapticgaps = txn.exec_params(
                            "SELECT synaptic_gap_id, x, y, z, energy_level, max_energy_level "
                            "FROM synapticgaps "
                            "WHERE axon_bouton_id = $1 ORDER BY synaptic_gap_id ASC",
                            bouton_id
                    );

                    for (const auto& gap : synapticgaps) {
                        int syn_gap_id       = gap[0].as<int>();
                        double gx            = gap[1].as<double>();
                        double gy            = gap[2].as<double>();
                        double gz            = gap[3].as<double>();
                        double genergy_level = gap[4].as<double>();
                        double gmax_energy_level = gap[5].is_null() ? 100.0 : gap[5].as<double>(); // default to 100 if null

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
                        glyphVectors->InsertNextTuple3(0.0, 0.0, 0.0);
                        glyphTypes->InsertNextValue(10); // glyph index for synaptic gap

                        double speedCalculation = (255.0 / gmax_energy_level) * genergy_level; // Scale to 255
                        unsigned char R_g = static_cast<unsigned char>(speedCalculation);
                        unsigned char G_g = 0;
                        unsigned char B_g = static_cast<unsigned char>(255.0 - speedCalculation);
                        unsigned char color_g[3] = { R_g, G_g, B_g };
                        glyphColors->InsertNextTypedTuple(color_g);
                    }
                }

                // 6) Recursively insert nested axon branches
                insertAxons(txn,
                            points, lines,
                            glyphPoints, glyphVectors, glyphTypes, glyphColors,
                            axon_id_new,  // pass the new axon as parent_axon_id
                            -1,           // no intermediate axon_branch parent
                            -1            // no direct axon_hillock parent
                );
            }
        }
    }
    catch (const std::exception& e) {
        logger_ << "Error in insertAxons: " << e.what() << std::endl;
        std::cerr << "Error in insertAxons: " << e.what() << std::endl;
    }
}

//------------------------------------------------------------------------------
// buildAndRenderFrame:
// This method encapsulates the “database query + pipeline rebuild” logic that used
// to live inside your infinite loop. It is invoked by the timer callback every N ms.
//------------------------------------------------------------------------------
void Visualiser::buildAndRenderFrame() {
    // Protect pipeline updates behind a mutex if necessary (not strictly needed if called
    // only from the VTK event loop on the main thread, but kept here for thread safety).
    std::lock_guard<std::mutex> lock(pipelineMutex_);

    // 1) Reset existing geometry arrays
    points_->Reset();
    lines_->Reset();
    glyphPoints_->Reset();
    glyphVectors_->Reset();
    glyphTypes_->Reset();
    glyphColors_->Reset();

    try {
        // 2) Start a new DB transaction
        pqxx::work txn(*conn_);

        // 3) Query top-level neurons (limit 10 for demo)
        pqxx::result neurons = txn.exec(
                "SELECT neuron_id, x, y, z, propagation_rate, neuron_type, energy_level, max_energy_level "
                "FROM neurons ORDER BY neuron_id ASC LIMIT 10"
        );

        for (const auto& neuron : neurons) {
            if (neuron.size() != 7) {
                continue;
            }
            if (neuron[0].is_null() || neuron[1].is_null() || neuron[2].is_null() ||
                neuron[3].is_null() || neuron[4].is_null() || neuron[5].is_null() ||
                neuron[6].is_null() || neuron[7].is_null()) {
                continue;
            }

            int neuron_id        = neuron[0].as<int>();
            double nx            = neuron[1].as<double>();
            double ny            = neuron[2].as<double>();
            double nz            = neuron[3].as<double>();
            double propagation   = neuron[4].as<double>(); // unused for drawing
            int neuron_type      = neuron[5].as<int>();    // unused for drawing
            double nenergy_level = neuron[6].as<double>();
            double nmax_energy_level = neuron[7].is_null() ? 100.0 : neuron[7].as<double>(); // default to 100 if null

            // 1) Insert the neuron’s center as a glyph point
            glyphPoints_->InsertNextPoint(nx, ny, nz);
            glyphVectors_->InsertNextTuple3(0.0, 0.0, 0.0);
            glyphTypes_->InsertNextValue(1); // glyph index for neuron

            double speedCalculation = (255.0 / nmax_energy_level) * nenergy_level; // Scale to 255
            unsigned char R = static_cast<unsigned char>(speedCalculation);
            unsigned char G = 0;
            unsigned char B = static_cast<unsigned char>(255.0 - speedCalculation);
            unsigned char color[3] = { R, G, B };
            glyphColors_->InsertNextTypedTuple(color);

            // 2) Fetch all somas for this neuron
            pqxx::result somas = txn.exec_params(
                    "SELECT soma_id, x, y, z, energy_level, max_energy_level "
                    "FROM somas "
                    "WHERE neuron_id = $1 ORDER BY soma_id ASC",
                    neuron_id
            );

            for (const auto& soma : somas) {
                if (soma.size() != 5) {
                    continue;
                }
                if (soma[0].is_null() || soma[1].is_null() || soma[2].is_null() ||
                    soma[3].is_null() || soma[4].is_null() || soma[5].is_null()) {
                    continue;
                }

                int soma_id        = soma[0].as<int>();
                double sx          = soma[1].as<double>();
                double sy          = soma[2].as<double>();
                double sz          = soma[3].as<double>();
                double senergy     = soma[4].as<double>();
                double smax_energy_level = soma[5].is_null() ? 100.0 : soma[5].as<double>(); // default to 100 if null

                // Insert soma glyph
                glyphPoints_->InsertNextPoint(sx, sy, sz);
                glyphVectors_->InsertNextTuple3(0.0, 0.0, 0.0);
                glyphTypes_->InsertNextValue(4); // glyph index for soma

                double speedCalculation = (255.0 / smax_energy_level) * senergy; // Scale to 255
                unsigned char R_s = static_cast<unsigned char>(speedCalculation);
                unsigned char G_s = 0;
                unsigned char B_s = static_cast<unsigned char>(255.0 - speedCalculation);
                unsigned char color_s[3] = { R_s, G_s, B_s };
                glyphColors_->InsertNextTypedTuple(color_s);

                // 3) Recursively insert dendrite branches under this soma
                insertDendriteBranches(txn,
                                       points_, lines_,
                                       glyphPoints_, glyphVectors_, glyphTypes_, glyphColors_,
                                       soma_id,  // parent_soma_id
                                       -1        // parent_dendrite_id
                );

                // 4) Fetch all axon hillocks for this soma
                pqxx::result axonhillocks = txn.exec_params(
                        "SELECT axon_hillock_id, x, y, z, energy_level, max_energy_level "
                        "FROM axonhillocks "
                        "WHERE soma_id = $1 ORDER BY axon_hillock_id ASC",
                        soma_id
                );

                for (const auto& hillock : axonhillocks) {
                    if (hillock.size() != 5) {
                        continue;
                    }
                    if (hillock[0].is_null() || hillock[1].is_null() || hillock[2].is_null() ||
                        hillock[3].is_null() || hillock[4].is_null() || hillock[5].is_null()) {
                        continue;
                    }

                    int hillock_id        = hillock[0].as<int>();
                    double ahx            = hillock[1].as<double>();
                    double ahy            = hillock[2].as<double>();
                    double ahz            = hillock[3].as<double>();
                    double ahenergy       = hillock[4].as<double>();
                    double ahmax_energy_level = hillock[5].is_null() ? 100.0 : hillock[5].as<double>(); // default to 100 if null

                    // Insert axon hillock glyph
                    glyphPoints_->InsertNextPoint(ahx, ahy, ahz);
                    glyphVectors_->InsertNextTuple3(0.0, 0.0, 0.0);
                    glyphTypes_->InsertNextValue(5); // glyph index for axon hillock

                    double speedCalculation = (255.0 / ahmax_energy_level) * ahenergy; // Scale to 255
                    unsigned char R_ah = static_cast<unsigned char>(speedCalculation);
                    unsigned char G_ah = 0;
                    unsigned char B_ah = static_cast<unsigned char>(255.0 - speedCalculation);
                    unsigned char color_ah[3] = { R_ah, G_ah, B_ah };
                    glyphColors_->InsertNextTypedTuple(color_ah);

                    // 5) Query all axons under this hillock
                    pqxx::result axons = txn.exec_params(
                            "SELECT axon_id, x, y, z, energy_level, max_energy_level "
                            "FROM axons "
                            "WHERE axon_hillock_id = $1 ORDER BY axon_id ASC",
                            hillock_id
                    );

                    for (const auto& axon : axons) {
                        if (axon.size() != 5) {
                            continue;
                        }
                        if (axon[0].is_null() || axon[1].is_null() || axon[2].is_null() ||
                            axon[3].is_null() || axon[4].is_null() || axon[5].is_null()) {
                            continue;
                        }

                        int axon_id          = axon[0].as<int>();
                        double ax            = axon[1].as<double>();
                        double ay            = axon[2].as<double>();
                        double az            = axon[3].as<double>();
                        double aenergy       = axon[4].as<double>();
                        double amax_energy_level = axon[5].is_null() ? 100.0 : axon[5].as<double>(); // default to 100 if null

                        // Insert axon endpoint
                        points_->InsertNextPoint(ax, ay, az);
                        vtkIdType axonAnchor = points_->GetNumberOfPoints() - 1;

                        // Line from hillock to axon
                        vtkSmartPointer<vtkLine> axonLine = vtkSmartPointer<vtkLine>::New();
                        axonLine->GetPointIds()->SetId(0, points_->GetNumberOfPoints() - 2); // hillock
                        axonLine->GetPointIds()->SetId(1, axonAnchor);
                        lines_->InsertNextCell(axonLine);

                        // Insert glyph for axon
                        glyphPoints_->InsertNextPoint(ax, ay, az);
                        glyphVectors_->InsertNextTuple3(ax - ahx, ay - ahy, az - ahz);
                        glyphTypes_->InsertNextValue(8); // glyph index for axon

                        double speedCalculation = (255.0 / amax_energy_level) * aenergy; // Scale to 255
                        unsigned char R_a = static_cast<unsigned char>(speedCalculation);
                        unsigned char G_a = 0;
                        unsigned char B_a = static_cast<unsigned char>(255.0 - speedCalculation);
                        unsigned char color_a[3] = { R_a, G_a, B_a };
                        glyphColors_->InsertNextTypedTuple(color_a);

                        // Recursively insert axons, boutons, synaptic gaps under this axon
                        insertAxons(txn,
                                    points_, lines_,
                                    glyphPoints_, glyphVectors_, glyphTypes_, glyphColors_,
                                    axon_id,    // parent_axon_id
                                    -1,         // parent_axon_branch_id
                                    -1          // parent_axon_hillock_id
                        );
                    }
                }
            }

            txn.commit();
        }
    }
    catch (const std::exception& e) {
        logger_ << "Error building frame: " << e.what() << std::endl;
        std::cerr << "Error building frame: " << e.what() << std::endl;
    }

    // 6) Pass updated arrays into polydata and glyph pipeline

    polyData_->SetPoints(points_);
    polyData_->SetLines(lines_);

    glyphPolyData_->SetPoints(glyphPoints_);
    glyphPolyData_->GetPointData()->AddArray(glyphVectors_);
    glyphPolyData_->GetPointData()->AddArray(glyphTypes_);
    glyphPolyData_->GetPointData()->AddArray(glyphColors_);
    glyphPolyData_->GetPointData()->SetActiveScalars("Colors");

    // Update mappers
    lineMapper_->SetInputData(polyData_);

    glyph3D_->SetInputData(glyphPolyData_);
    glyph3D_->SetSourceConnection(0, sphereSource_->GetOutputPort());
    glyph3D_->SetSourceConnection(1, cubeSource_->GetOutputPort());
    glyph3D_->SetSourceConnection(2, cylinderSource_->GetOutputPort());
    glyph3D_->SetSourceConnection(3, cubeSource_->GetOutputPort());
    glyph3D_->SetSourceConnection(4, sphereSource_->GetOutputPort());
    glyph3D_->SetSourceConnection(5, sphereSource_->GetOutputPort());
    glyph3D_->SetSourceConnection(6, cylinderSource_->GetOutputPort());
    glyph3D_->SetSourceConnection(7, cylinderSource_->GetOutputPort());
    glyph3D_->SetSourceConnection(8, cubeSource_->GetOutputPort());
    glyph3D_->SetSourceConnection(9, sphereSource_->GetOutputPort());
    glyph3D_->Update();

    glyphMapper_->SetInputConnection(glyph3D_->GetOutputPort());

    // 7) Finally, re-render the window
    renderWindow_->Render();
}

//------------------------------------------------------------------------------
// visualise():
//   - Sets up VTK pipeline (renderer, window, interactor, glyphs, mappers, actors).
//   - Forces on-screen rendering.
//   - Shows an empty window immediately.
//   - Installs a repeating timer that calls buildAndRenderFrame() every 5 seconds.
//   - Starts the interactor event loop (blocking until window is closed).
//------------------------------------------------------------------------------
void Visualiser::visualise() {
    std::cout << "Starting Visualiser..." << std::endl;

    // 1) Build VTK renderer + window + interactor
    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    renderWindow_ = vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow_->AddRenderer(renderer);
    renderWindow_->SetSize(800, 600);

    // Force on-screen rendering in case VTK defaulted to off-screen
    renderWindow_->SetOffScreenRendering(0);

    vtkSmartPointer<vtkRenderWindowInteractor> interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    interactor->SetRenderWindow(renderWindow_);

    // Install an observer to catch VTK warnings/errors
    vtkSmartPointer<VTKErrorObserver> errorObserver = vtkSmartPointer<VTKErrorObserver>::New();
    vtkSmartPointer<vtkOutputWindow> outputWindow = vtkSmartPointer<vtkOutputWindow>::New();
    vtkOutputWindow::SetInstance(outputWindow);
    outputWindow->AddObserver(vtkCommand::ErrorEvent, errorObserver);
    outputWindow->AddObserver(vtkCommand::WarningEvent, errorObserver);

    // 2) Create and configure shared VTK data arrays
    points_      = vtkSmartPointer<vtkPoints>::New();
    lines_       = vtkSmartPointer<vtkCellArray>::New();
    glyphPoints_ = vtkSmartPointer<vtkPoints>::New();

    glyphVectors_ = vtkSmartPointer<vtkFloatArray>::New();
    glyphVectors_->SetNumberOfComponents(3);
    glyphVectors_->SetName("Vectors");

    glyphTypes_ = vtkSmartPointer<vtkUnsignedCharArray>::New();
    glyphTypes_->SetNumberOfComponents(1);
    glyphTypes_->SetName("GlyphType");

    glyphColors_ = vtkSmartPointer<vtkUnsignedCharArray>::New();
    glyphColors_->SetNumberOfComponents(3);
    glyphColors_->SetName("Colors");

    polyData_      = vtkSmartPointer<vtkPolyData>::New();
    glyphPolyData_ = vtkSmartPointer<vtkPolyData>::New();

    // 3) Set up glyph sources (static geometry)
    sphereSource_ = vtkSmartPointer<vtkSphereSource>::New();
    sphereSource_->SetRadius(0.5);
    sphereSource_->Update();

    cubeSource_ = vtkSmartPointer<vtkCubeSource>::New();
    cubeSource_->SetXLength(0.5);
    cubeSource_->SetYLength(0.5);
    cubeSource_->SetZLength(0.5);
    cubeSource_->Update();

    cylinderSource_ = vtkSmartPointer<vtkCylinderSource>::New();
    cylinderSource_->SetRadius(0.1);
    cylinderSource_->SetHeight(1.0);
    cylinderSource_->Update();

    // 4) Configure Glyph3D to dispatch glyphs based on "GlyphType" array
    glyph3D_ = vtkSmartPointer<vtkGlyph3D>::New();
    glyph3D_->SetOrient(true);
    glyph3D_->SetVectorModeToUseVector();
    glyph3D_->SetScaleModeToScaleByVector();
    glyph3D_->SetScaleFactor(1.0);

    // Map each integer in GlyphType to one of the source ports:
    glyph3D_->SetSourceConnection(0, sphereSource_->GetOutputPort());   // 0 → neuron
    glyph3D_->SetSourceConnection(1, cubeSource_->GetOutputPort());     // 1 → bouton or soma (reuse)
    glyph3D_->SetSourceConnection(2, cylinderSource_->GetOutputPort()); // 2 → dendrite
    glyph3D_->SetSourceConnection(3, cubeSource_->GetOutputPort());     // 3 → dendrite branch
    glyph3D_->SetSourceConnection(4, sphereSource_->GetOutputPort());   // 4 → soma
    glyph3D_->SetSourceConnection(5, sphereSource_->GetOutputPort());   // 5 → axon hillock
    glyph3D_->SetSourceConnection(6, cylinderSource_->GetOutputPort()); // 6 → axon branch
    glyph3D_->SetSourceConnection(7, cylinderSource_->GetOutputPort()); // 7 → axon
    glyph3D_->SetSourceConnection(8, cubeSource_->GetOutputPort());     // 8 → axon bouton
    glyph3D_->SetSourceConnection(9, sphereSource_->GetOutputPort());   // 9 → synaptic gap

    // 5) Configure mappers and actors
    // Line mapper + actor
    lineMapper_ = vtkSmartPointer<vtkPolyDataMapper>::New();
    lineActor_ = vtkSmartPointer<vtkActor>::New();
    lineActor_->SetMapper(lineMapper_);
    lineActor_->GetProperty()->SetColor(1.0, 1.0, 0.0); // Yellow lines

    // Glyph mapper + actor
    glyphMapper_ = vtkSmartPointer<vtkPolyDataMapper>::New();
    glyphMapper_->ScalarVisibilityOn();
    glyphMapper_->SetScalarModeToUsePointData();
    glyphMapper_->SelectColorArray("Colors");

    glyphActor_ = vtkSmartPointer<vtkActor>::New();
    glyphActor_->SetMapper(glyphMapper_);

    // Add actors once
    renderer->AddActor(lineActor_);
    renderer->AddActor(glyphActor_);
    renderer->ResetCamera();

    // 6) Perform an initial empty Render so the window actually appears
    renderWindow_->Render();

    // 7) Initialize the interactor (must be called before any timer events)
    interactor->Initialize();

    // 8) Create and configure our repeating timer callback
    vtkSmartPointer<UpdateTimerCallback> timerCallback = vtkSmartPointer<UpdateTimerCallback>::New();
    timerCallback->SetVisualiser(this);
    timerCallback->SetInterval(5000); // call buildAndRenderFrame() every 5000 ms

    interactor->AddObserver(vtkCommand::TimerEvent, timerCallback);
    interactor->CreateRepeatingTimer(5000);

    // 9) Hand control to VTK’s event loop: this will not return until the window is closed
    interactor->Start();

    // Once the window is closed, Start() returns:
    std::cout << "Render window closed. Exiting visualise()." << std::endl;
}

//------------------------------------------------------------------------------
// Destructor (if needed) for cleanup
//------------------------------------------------------------------------------
Visualiser::~Visualiser() {
    // All vtkSmartPointer members are automatically cleaned up.
}

// Reads “key=value” lines (ignoring comments/blank) from each filename and
// returns a map<string,string>.
static std::map<std::string,std::string>
read_config(const std::vector<std::string>& filenames)
{
    std::map<std::string,std::string> config;
    for(const auto& filename : filenames) {
        std::ifstream file(filename);
        if(!file.is_open()) {
            throw std::runtime_error("Failed to open config file: " + filename);
        }
        std::string line;
        while(std::getline(file, line)) {
            if(line.empty() || line[0]=='#') continue;
            std::istringstream ss(line);
            std::string key, value;
            if(std::getline(ss, key, '=') && std::getline(ss, value)) {
                // Trim whitespace from key
                key.erase(0, key.find_first_not_of(" \t\r\n"));
                key.erase(key.find_last_not_of(" \t\r\n") + 1);
                // Trim whitespace from value
                value.erase(0, value.find_first_not_of(" \t\r\n"));
                value.erase(value.find_last_not_of(" \t\r\n") + 1);
                config[key] = value;
            }
        }
    }
    return config;
}

// Builds a PostgreSQL connection string from keys “host”, “port”, “user”,
// “password”, “dbname” (if present in the map), concatenated with spaces:
static std::string
build_connection_string(const std::map<std::string,std::string>& config)
{
    std::string connstr;
    const std::set<std::string> allowed = {"host","port","user","password","dbname"};
    for(const auto& kv : config) {
        if(allowed.count(kv.first)) {
            connstr += kv.first + "=" + kv.second + " ";
        }
    }
    return connstr;
}

int main(int argc, char** argv) {
    try {
        // 1) Initialise Logger
        Logger logger("errors_visualiser.log");

        // 2) Read your config files (if any) just as AARNN does,
        //    build a Postgres connection string:
        std::vector<std::string> config_files = { "Visualiser.conf", "simulation.conf" };
        auto config = read_config(config_files);
        std::string connection_str = build_connection_string(config);

        // 3) Open a pqxx::connection
        auto conn = std::make_shared<pqxx::connection>(connection_str);
        if (!conn->is_open()) {
            std::cerr << "Unable to connect to PostgreSQL with string: "
                      << connection_str << std::endl;
            return 1;
        }
        logger << "Connected to database.\n";

        // 4) Launch (or prepare) your WebSocketServer
        WebSocketServer ws_server;
        std::thread ws_thread([&ws_server]() {
            ws_server.run(9002);
        });

        // 5) Create and run your Visualiser instance
        Visualiser vis(conn, logger, ws_server);
        vis.visualise();

        // 6) (If visualise() ever returns) clean up:
        ws_thread.join();
    }
    catch (const std::exception& ex) {
        std::cerr << "Fatal error in Visualiser::main(): " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
