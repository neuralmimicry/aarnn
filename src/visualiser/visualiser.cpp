// visualiser.cpp (Complete & Bulk-Optimised)
//
// This file is responsible for fetching neural network data from a PostgreSQL
// database and rendering it in a 3D scene using the Visualization Toolkit (VTK).
// It operates in a non-blocking manner, periodically refreshing the data
// from the database to reflect real-time changes in the simulation.
//
// Optimisation Strategy:
// To prevent thousands of small SELECT queries, this implementation uses a
// bulk-fetching approach. At each update, it reads entire tables (somas,
// axons, dendrites, etc.) into in-memory maps. It then builds the neuron
// tree structures in C++ before translating them into VTK geometry. This
// significantly reduces database load and improves rendering-loop performance.

#include "visualiser.h"
#include "Logger.h" // Assumed to be a custom logging utility
#include "wss.h"    // Assumed to be a WebSocket server utility

#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <chrono>
#include <string>
#include <stdexcept>
#include <vector>
#include <map>
#include <pqxx/pqxx>
#include <fstream> // For file I/O in main()
#include <sstream> // For string streams in main()
#include <set>     // For key whitelisting in main()
#include <boost/json.hpp>
// VTK Includes
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkCallbackCommand.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkUnsignedCharArray.h>
#include <vtkPointData.h>
#include <vtkLine.h>
#include <vtkSphereSource.h>
#include <vtkCubeSource.h>
#include <vtkCylinderSource.h>
#include <vtkGlyph3D.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkOutputWindow.h>

namespace {
    //------------------------------------------------------------------------------
    // VTKErrorObserver: catches VTK warnings/errors and logs them
    class VTKErrorObserver : public vtkCommand {
    public:
        static VTKErrorObserver* New() { return new VTKErrorObserver(); }
        void Execute(vtkObject* caller, unsigned long eventId, void* callData) override {
            if (eventId == vtkCommand::ErrorEvent || eventId == vtkCommand::WarningEvent) {
                const char* msg = static_cast<char*>(callData);
                std::cerr << "[VTK] " << msg << std::endl;
            }
        }
    };

    /**
     * @class UpdateTimerCallback
     * @brief A VTK command that is executed on a timer to refresh the visualisation.
     *
     * This class creates a callback that hooks into the VTK interactor's timer
     * events. Every time the timer fires, it triggers the main
     * buildAndRenderFrame method of the Visualiser class, ensuring the scene
     * is periodically updated with the latest data from the database. This is the
     * core mechanism for the non-blocking, real-time update capability.
     */

    class UpdateTimerCallback : public vtkCallbackCommand {
    public:
        static UpdateTimerCallback* New() { return new UpdateTimerCallback; }
        void SetVisualiser(Visualiser* vis) { vis_ = vis; }
        void Execute(vtkObject* caller, unsigned long eventId, void* /*callData*/) override {
            if (eventId != vtkCommand::TimerEvent || !vis_) return;
            vis_->buildAndRenderFrame();
            if (auto* iren = dynamic_cast<vtkRenderWindowInteractor*>(caller)) {
                iren->CreateRepeatingTimer(vis_->getUpdateInterval());
            }
        }
    private:
        Visualiser* vis_ = nullptr;
    };
} // anonymous namespace

namespace json = boost::json;

//------------------------------------------------------------------------------
// Visualiser implementation
//------------------------------------------------------------------------------

Visualiser::~Visualiser() = default;

Visualiser::Visualiser(std::shared_ptr<pqxx::connection> conn,
                       Logger& logger,
                       WebSocketServer& ws_server)
        : conn_(std::move(conn)),
          logger_(logger),
          ws_server_(ws_server),
          update_interval_ms_(500) // Default update interval in milliseconds
{}

void Visualiser::visualise() {
    if (!conn_ || !conn_->is_open()) {
        logger_ << "Invalid DB connection in visualise()\n";
        return;
    }

    logger_ << "Visualiser started. Initializing VTK components...\n";
    // Install VTK error observer
    {
        vtkSmartPointer<VTKErrorObserver> errObs = vtkSmartPointer<VTKErrorObserver>::New();
        vtkSmartPointer<vtkOutputWindow> outWin = vtkSmartPointer<vtkOutputWindow>::New();
        vtkOutputWindow::SetInstance(outWin);
        outWin->AddObserver(vtkCommand::ErrorEvent, errObs);
        outWin->AddObserver(vtkCommand::WarningEvent, errObs);
    }

    logger_ << "Initializing VTK pipeline...\n";
    initializeVTKPipeline();

    // Setup timer callback
    logger_ << "Setting up timer callback...\n";
    vtkSmartPointer<UpdateTimerCallback> timerCb = vtkSmartPointer<UpdateTimerCallback>::New();
    timerCb->SetVisualiser(this);

    logger_ << "Adding actors to renderer...\n";
    renderer_->AddActor(lineActor_);
    renderer_->AddActor(glyphActor_);
    renderWindow_->AddRenderer(renderer_);
    interactor_->SetRenderWindow(renderWindow_);

    logger_ << "About to initialize interactor\n";
    interactor_->Initialize();
    logger_ << "Interactor initialized\n";
    interactor_->AddObserver(vtkCommand::TimerEvent, timerCb);

    logger_ << "Starting VTK render loop. Close window to exit or if unavailable, it will exit immediately.\n";
    // Initial build & render & broadcast
    buildAndRenderFrame();
    logger_ << "Initial frame built and broadcast.\n";
    interactor_->CreateRepeatingTimer(update_interval_ms_);
    renderWindow_->Render();
    logger_ << "Render window created and initial render done. Starting interactor.\n";

    // Record time before Start
    auto t0 = std::chrono::steady_clock::now();
    interactor_->Start();
    // When Start returns, either user closed the window or VTK could not open it.
    auto t1 = std::chrono::steady_clock::now();
    auto dur_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    logger_ << "interactor_->Start() returned after " << dur_ms << " ms.\n";

    // If the interactor ran only very briefly (e.g. < some threshold) or always when it returns:
    logger_ << "VTK interactor loop exited or unavailable. Switching to headless broadcast mode.\n";

    // Start a background thread to continue periodic JSON broadcasts
    std::thread([this]() {
        logger_ << "Headless loop: entering periodic buildAndRenderFrame every "
                << update_interval_ms_ << " ms.\n";
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(update_interval_ms_));
            try {
                buildAndRenderFrame();
            } catch (const std::exception& e) {
                logger_ << "Error in periodic buildAndRenderFrame: " << e.what() << "\n";
            }
        }
    }).detach();

    // Keep the main thread alive indefinitely so the process does not exit.
    // For example, block here on a condition variable or similar.
    logger_ << "Visualiser main thread now blocking indefinitely to keep WebSocket alive.\n";
    std::mutex mtx;
    std::unique_lock<std::mutex> lk(mtx);
    std::condition_variable cv;
    cv.wait(lk);  // Wait forever (or until you signal shutdown externally)
}

void Visualiser::buildAndRenderFrame() {
    std::lock_guard<std::mutex> lock(pipelineMutex_);

    points_->Reset(); lines_->Reset(); glyphOriginalIds_.clear(); glyphPoints_->Reset();
    glyphVectors_->Reset(); glyphTypes_->Reset(); glyphColors_->Reset();

    try {
        pqxx::nontransaction txn(*conn_);
        fetchAllData(txn);

        for (auto& [soma_id, soma] : somas_) {
            glyphPoints_->InsertNextPoint(soma.pos.data());
            glyphVectors_->InsertNextTuple3(0,0,0);
            glyphTypes_->InsertNextValue(SOMA_GLYPH);
            vtkIdType somaPt = points_->InsertNextPoint(soma.pos.data());
            unsigned char* c = computeColor(soma.energy, soma.max_energy);
            glyphColors_->InsertNextTypedTuple(c);
            glyphOriginalIds_.push_back(soma_id);
            if (soma_to_hillock_.count(soma_id)) buildAxonTree(soma_to_hillock_[soma_id], somaPt);
            if (soma_to_dendrite_branches_.count(soma_id)) {
                for (int bid : soma_to_dendrite_branches_[soma_id])
                    buildDendriteTree(bid, somaPt, true);
            }
        }

        polyData_->SetPoints(points_);
        polyData_->SetLines(lines_);
        glyphPolyData_->SetPoints(glyphPoints_);
        glyphPolyData_->GetPointData()->AddArray(glyphVectors_);
        glyphPolyData_->GetPointData()->AddArray(glyphTypes_);
        glyphPolyData_->GetPointData()->AddArray(glyphColors_);
        glyphPolyData_->GetPointData()->SetActiveScalars("Colors");

        renderWindow_->Render();
        logger_ << "Frame rendered. Broadcasting JSON...\n";

        json::object j;
        json::array pts;
        for (vtkIdType i = 0; i < points_->GetNumberOfPoints(); ++i) {
            double p[3]; points_->GetPoint(i,p);
            pts.emplace_back(json::array{p[0],p[1],p[2]});
        }
        j["points"] = pts;

        json::array lns;
        vtkSmartPointer<vtkIdList> ids = vtkSmartPointer<vtkIdList>::New();
        lines_->InitTraversal();
        while (lines_->GetNextCell(ids)) {
            if (ids->GetNumberOfIds() == 2)
                lns.emplace_back(json::array{ids->GetId(0), ids->GetId(1)});
        }
        j["lines"] = lns;

        json::array glyphs;
        for (vtkIdType i = 0; i < glyphPoints_->GetNumberOfPoints(); ++i) {
            double p[3]; glyphPoints_->GetPoint(i,p);
            int type = glyphTypes_->GetValue(i);
            unsigned char col[3]; glyphColors_->GetTypedTuple(i,col);
            json::object ge;
            ge["id"] = glyphOriginalIds_[i];
            ge["pos"] = json::array{p[0],p[1],p[2]};
            ge["type"] = type;
            ge["color"] = json::array{col[0],col[1],col[2]};
            glyphs.push_back(ge);
        }
        j["glyphs"] = glyphs;

        ws_server_.broadcast(boost::json::serialize(j));
        logger_ << "Broadcast complete.\n";

    } catch (const std::exception& e) {
        logger_ << "Error in buildAndRenderFrame: " << e.what() << "\n";
        std::cerr << "Error in buildAndRenderFrame: " << e.what() << std::endl;
    }
}

//------------------------------------------------------------------------------
// Initialization of VTK pipeline components
//------------------------------------------------------------------------------

void Visualiser::initializeVTKPipeline() {
    // Points & lines
    points_   = vtkSmartPointer<vtkPoints>::New();
    lines_    = vtkSmartPointer<vtkCellArray>::New();
    polyData_ = vtkSmartPointer<vtkPolyData>::New();

    // Glyph arrays
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

    glyphPolyData_ = vtkSmartPointer<vtkPolyData>::New();
    glyphPolyData_->SetPoints(glyphPoints_);

    // Glyph sources
    sphereSource_   = vtkSmartPointer<vtkSphereSource>::New();  sphereSource_->SetRadius(0.5);
    cubeSource_     = vtkSmartPointer<vtkCubeSource>::New();    cubeSource_->SetXLength(0.5); cubeSource_->SetYLength(0.5); cubeSource_->SetZLength(0.5);
    cylinderSource_= vtkSmartPointer<vtkCylinderSource>::New(); cylinderSource_->SetRadius(0.1); cylinderSource_->SetHeight(1.0);

    // Glyph3D mapper
    glyph3D_ = vtkSmartPointer<vtkGlyph3D>::New();
    glyph3D_->SetOrient(true);
    glyph3D_->SetVectorModeToUseVector();
    glyph3D_->SetScaleModeToScaleByVector();
    glyph3D_->SetScaleFactor(1.0);

    // Map glyph types to sources
    glyph3D_->SetSourceConnection(NEURON_GLYPH, sphereSource_->GetOutputPort());
    glyph3D_->SetSourceConnection(DENDRITE_GLYPH, cylinderSource_->GetOutputPort());
    glyph3D_->SetSourceConnection(DENDRITE_BRANCH_GLYPH, cubeSource_->GetOutputPort());
    glyph3D_->SetSourceConnection(DENDRITE_BOUTON_GLYPH, cubeSource_->GetOutputPort());
    glyph3D_->SetSourceConnection(SOMA_GLYPH, sphereSource_->GetOutputPort());
    glyph3D_->SetSourceConnection(AXON_HILLOCK_GLYPH, sphereSource_->GetOutputPort());
    glyph3D_->SetSourceConnection(AXON_BRANCH_GLYPH, cylinderSource_->GetOutputPort());
    glyph3D_->SetSourceConnection(AXON_GLYPH, cylinderSource_->GetOutputPort());
    glyph3D_->SetSourceConnection(AXON_BOUTON_GLYPH, cubeSource_->GetOutputPort());
    glyph3D_->SetSourceConnection(SYNAPTIC_GAP_GLYPH, sphereSource_->GetOutputPort());

    glyph3D_->SetInputData(glyphPolyData_);
    glyph3D_->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "GlyphType");

    glyphMapper_ = vtkSmartPointer<vtkPolyDataMapper>::New();
    glyphMapper_->SetInputConnection(glyph3D_->GetOutputPort());
    glyphMapper_->ScalarVisibilityOn();
    glyphMapper_->SelectColorArray("Colors");

    glyphActor_ = vtkSmartPointer<vtkActor>::New();
    glyphActor_->SetMapper(glyphMapper_);

    // Lines
    lineMapper_ = vtkSmartPointer<vtkPolyDataMapper>::New();
    lineMapper_->SetInputData(polyData_);
    lineActor_ = vtkSmartPointer<vtkActor>::New();
    lineActor_->SetMapper(lineMapper_);
    lineActor_->GetProperty()->SetLineWidth(1.5);
    lineActor_->GetProperty()->SetColor(1.0,1.0,0.0);

    // Renderer / window / interactor
    renderer_    = vtkSmartPointer<vtkRenderer>::New();
    renderWindow_= vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow_->SetSize(1280,960);
    renderWindow_->SetWindowName("AARNN Visualiser (Bulk+JSON)");

    interactor_ = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    vtkSmartPointer<vtkInteractorStyleTrackballCamera> style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    interactor_->SetInteractorStyle(style);
}

//------------------------------------------------------------------------------
// Compute RGB color based on energy fraction
//------------------------------------------------------------------------------
unsigned char* Visualiser::computeColor(double energy, double max_energy) {
    static unsigned char col[3];
    double frac = (max_energy>0 ? energy/max_energy : 0);
    auto intensity = static_cast<unsigned char>(frac * 255);
    col[0] = intensity;
    col[1] = 0;
    col[2] = 255 - intensity;
    return col;
}

//------------------------------------------------------------------------------
// Bulk-fetch data from DB into maps (including energy levels)
//------------------------------------------------------------------------------
void Visualiser::fetchAllData(pqxx::transaction_base& txn) {
    // Clear previous data
    somas_.clear();
    hillocks_.clear();
    axons_.clear();
    axon_branches_.clear();
    axon_boutons_.clear();
    dendrite_branches_.clear();
    dendrites_.clear();
    dendrite_boutons_.clear();

    soma_to_hillock_.clear();
    hillock_to_axon_.clear();
    axon_to_branches_.clear();
    branch_to_axons_.clear();
    axon_to_boutons_.clear();
    bouton_to_synaptic_gap_.clear();

    soma_to_dendrite_branches_.clear();
    dendrite_to_branches_.clear();
    branch_to_dendrites_.clear();

    // 1) Fetch somas
    {
        const std::string query =
                "SELECT soma_id, x, y, z, energy_level, max_energy_level FROM somas";
        pqxx::result result = txn.exec(query);
        for (const auto& row : result) {
            int id = row["soma_id"].as<int>();
            ComponentData cd{};
            cd.pos = { row["x"].as<double>(), row["y"].as<double>(), row["z"].as<double>() };
            cd.energy = row["energy_level"].as<double>();
            cd.max_energy = row["max_energy_level"].as<double>();
            somas_[id] = cd;
            // Note: mapping from soma to hillock(s) happens below in hillocks fetch
        }
    }

    // 2) Fetch axon hillocks
    {
        const std::string query =
                "SELECT axon_hillock_id, soma_id, x, y, z, energy_level, max_energy_level FROM axonhillocks";
        pqxx::result result = txn.exec(query);
        for (const auto& row : result) {
            int hid = row["axon_hillock_id"].as<int>();
            int sid = row["soma_id"].as<int>();  // assume not null
            ComponentData cd{};
            cd.pos = { row["x"].as<double>(), row["y"].as<double>(), row["z"].as<double>() };
            cd.energy = row["energy_level"].as<double>();
            cd.max_energy = row["max_energy_level"].as<double>();
            hillocks_[hid] = cd;
            // Associate this hillock with its soma:
            soma_to_hillock_[sid] = hid;
        }
    }

    // 3) Fetch axons
    {
        const std::string query =
                "SELECT axon_id, axon_hillock_id, x, y, z, energy_level, max_energy_level FROM axons";
        pqxx::result result = txn.exec(query);
        for (const auto& row : result) {
            int aid = row["axon_id"].as<int>();
            int hid = row["axon_hillock_id"].as<int>();  // assume not null
            ComponentData cd{};
            cd.pos = { row["x"].as<double>(), row["y"].as<double>(), row["z"].as<double>() };
            cd.energy = row["energy_level"].as<double>();
            cd.max_energy = row["max_energy_level"].as<double>();
            axons_[aid] = cd;
            // Map hillock to this axon:
            hillock_to_axon_[hid] = aid;
        }
    }

    // 4) Fetch axon branches
    {
        const std::string query =
                "SELECT axon_branch_id, parent_axon_id, parent_axon_branch_id, x, y, z, energy_level, max_energy_level FROM axonbranches";
        pqxx::result result = txn.exec(query);
        for (const auto& row : result) {
            int bid = row["axon_branch_id"].as<int>();

            // parent may be NULL in DB, so check
            int parent_axon_id = -1;
            if (!row["parent_axon_id"].is_null()) {
                parent_axon_id = row["parent_axon_id"].as<int>();
            }
            int parent_branch_id = -1;
            if (!row["parent_axon_branch_id"].is_null()) {
                parent_branch_id = row["parent_axon_branch_id"].as<int>();
            }

            ComponentData cd{};
            cd.pos = { row["x"].as<double>(), row["y"].as<double>(), row["z"].as<double>() };
            cd.energy = row["energy_level"].as<double>();
            cd.max_energy = row["max_energy_level"].as<double>();
            axon_branches_[bid] = cd;

            // If this branch is directly under an axon:
            if (parent_axon_id != -1) {
                axon_to_branches_[parent_axon_id].push_back(bid);
            }
            // If this branch is a sub-branch of another branch:
            if (parent_branch_id != -1) {
                branch_to_sub_branches_[parent_branch_id].push_back(bid);
            }
        }
    }

    // 5) Fetch axon boutons
    {
        const std::string query =
                "SELECT axon_bouton_id, axon_id, x, y, z, energy_level, max_energy_level FROM axonboutons";
        pqxx::result result = txn.exec(query);
        for (const auto& row : result) {
            int bout_id = row["axon_bouton_id"].as<int>();
            int axon_id = row["axon_id"].as<int>();  // assume not null

            ComponentData cd{};
            cd.pos = { row["x"].as<double>(), row["y"].as<double>(), row["z"].as<double>() };
            cd.energy = row["energy_level"].as<double>();
            cd.max_energy = row["max_energy_level"].as<double>();
            axon_boutons_[bout_id] = cd;

            axon_to_boutons_[axon_id].push_back(bout_id);
        }
    }

    // 6) Fetch dendrite branches
    {
        const std::string query =
                "SELECT dendrite_branch_id, soma_id, parent_dendrite_id, x, y, z, energy_level, max_energy_level FROM dendritebranches";
        pqxx::result result = txn.exec(query);
        for (const auto& row : result) {
            int dbid = row["dendrite_branch_id"].as<int>();

            // parent may be NULL
            int soma_id = -1;
            if (!row["soma_id"].is_null()) {
                soma_id = row["soma_id"].as<int>();
            }
            int parent_dendrite_id = -1;
            if (!row["parent_dendrite_id"].is_null()) {
                parent_dendrite_id = row["parent_dendrite_id"].as<int>();
            }

            ComponentData cd{};
            cd.pos = { row["x"].as<double>(), row["y"].as<double>(), row["z"].as<double>() };
            cd.energy = row["energy_level"].as<double>();
            cd.max_energy = row["max_energy_level"].as<double>();
            dendrite_branches_[dbid] = cd;

            // If top-level branch off a soma:
            if (soma_id != -1) {
                soma_to_dendrite_branches_[soma_id].push_back(dbid);
            }
            // If sub-branch of another dendrite:
            if (parent_dendrite_id != -1) {
                dendrite_to_branches_[parent_dendrite_id].push_back(dbid);
            }
        }
    }

    // 7) Fetch dendrites (leaf points) attached to branches
    {
        const std::string query =
                "SELECT dendrite_id, dendrite_branch_id, x, y, z, energy_level, max_energy_level FROM dendrites";
        pqxx::result result = txn.exec(query);
        for (const auto& row : result) {
            int did = row["dendrite_id"].as<int>();
            int branch_id = row["dendrite_branch_id"].as<int>();  // assume not null

            ComponentData cd{};
            cd.pos = { row["x"].as<double>(), row["y"].as<double>(), row["z"].as<double>() };
            cd.energy = row["energy_level"].as<double>();
            cd.max_energy = row["max_energy_level"].as<double>();
            dendrites_[did] = cd;

            branch_to_dendrites_[branch_id].push_back(did);
        }
    }

    // 8) Fetch dendrite boutons
    {
        const std::string query =
                "SELECT dendrite_bouton_id, dendrite_id, x, y, z, energy_level, max_energy_level FROM dendriteboutons";
        pqxx::result result = txn.exec(query);
        for (const auto& row : result) {
            int bout_id = row["dendrite_bouton_id"].as<int>();
            int dendrite_id = row["dendrite_id"].as<int>();  // assume not null

            ComponentData cd{};
            cd.pos = { row["x"].as<double>(), row["y"].as<double>(), row["z"].as<double>() };
            cd.energy = row["energy_level"].as<double>();
            cd.max_energy = row["max_energy_level"].as<double>();
            dendrite_boutons_[bout_id] = cd;

            dendrite_to_boutons_[dendrite_id].push_back(bout_id);
        }
    }

    // Log counts for debugging:
     logger_ << "Fetched: "
             << somas_.size() << " somas, "
             << hillocks_.size() << " hillocks, "
             << axons_.size() << " axons, "
             << axon_branches_.size() << " axon branches, "
             << axon_boutons_.size() << " axon boutons, "
             << dendrite_branches_.size() << " dendrite branches, "
             << dendrites_.size() << " dendrites, "
             << dendrite_boutons_.size() << " dendrite boutons.\n";
}

void Visualiser::buildAxonTree(int hillock_id, vtkIdType parent_point_id) {
    if (!hillocks_.count(hillock_id)) return;
    const auto& hillock_data = hillocks_.at(hillock_id);

    // Draw line from parent (soma) to hillock
    vtkIdType hillock_point_id = points_->InsertNextPoint(hillock_data.pos.data());
    createLine(parent_point_id, hillock_point_id);

    if (!hillock_to_axon_.count(hillock_id)) return;
    int axon_id = hillock_to_axon_.at(hillock_id);
    if (!axons_.count(axon_id)) return;
    const auto& axon_data = axons_.at(axon_id);

    // Draw line from hillock to axon proper
    vtkIdType axon_point_id = points_->InsertNextPoint(axon_data.pos.data());
    createLine(hillock_point_id, axon_point_id);

    // Recursively build the branches starting from this axon
    if (axon_to_branches_.count(axon_id)) {
        for (int branch_id : axon_to_branches_.at(axon_id)) {
            buildAxonBranch(branch_id, axon_point_id);
        }
    }
}

void Visualiser::buildAxonBranch(int branch_id, vtkIdType parent_point_id) {
    if (!axon_branches_.count(branch_id)) return;
    const auto& branch_data = axon_branches_.at(branch_id);

    // Draw line from parent to this branch point
    vtkIdType branch_point_id = points_->InsertNextPoint(branch_data.pos.data());
    createLine(parent_point_id, branch_point_id);

    // Draw axons attached to this branch
    if (branch_to_axons_.count(branch_id)) {
        for (int axon_id : branch_to_axons_.at(branch_id)) {
            if (axons_.count(axon_id)) {
                const auto& axon_data = axons_.at(axon_id);
                glyphPoints_->InsertNextPoint(axon_data.pos.data());
                glyphTypes_->InsertNextValue(AXON_GLYPH);
                glyphOriginalIds_.push_back(axon_id);
                vtkIdType axon_point_id = points_->InsertNextPoint(axon_data.pos.data());
                createLine(branch_point_id, axon_point_id);
            }
        }
    }

    // Recursively build sub-branches
    if (branch_to_sub_branches_.count(branch_id)) {
        for (int sub_branch_id : branch_to_sub_branches_.at(branch_id)) {
            buildAxonBranch(sub_branch_id, branch_point_id);
        }
    }
}


void Visualiser::buildDendriteTree(int branch_id, vtkIdType parent_point_id, bool is_top_level) {
    if (!dendrite_branches_.count(branch_id)) return;
    const auto& branch_data = dendrite_branches_.at(branch_id);

    // Draw line from parent to this branch
    vtkIdType branch_point_id = points_->InsertNextPoint(branch_data.pos.data());
    createLine(parent_point_id, branch_point_id);

    // Process dendrites attached to this branch
    if (branch_to_dendrites_.count(branch_id)) {
        for (int dendrite_id : branch_to_dendrites_.at(branch_id)) {
            if (dendrites_.count(dendrite_id)) {
                const auto& dendrite_data = dendrites_.at(dendrite_id);
                vtkIdType dendrite_point_id = points_->InsertNextPoint(dendrite_data.pos.data());
                createLine(branch_point_id, dendrite_point_id);

                // Recursively process sub-branches attached to this dendrite
                if (dendrite_to_branches_.count(dendrite_id)) {
                    for (int sub_branch_id : dendrite_to_branches_.at(dendrite_id)) {
                        buildDendriteTree(sub_branch_id, dendrite_point_id, false);
                    }
                }
            }
        }
    }
}


void Visualiser::createLine(vtkIdType p1, vtkIdType p2) {
    vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
    line->GetPointIds()->SetId(0, p1);
    line->GetPointIds()->SetId(1, p2);
    lines_->InsertNextCell(line);
}

//------------------------------------------------------------------------------
// APPLICATION ENTRY POINT & HELPERS
//------------------------------------------------------------------------------

/**
 * @brief Reads key-value pairs from configuration files.
 * @param files A vector of file paths to read. Later files override earlier ones.
 * @return A map of configuration keys to values.
 */
std::map<std::string, std::string> read_config(const std::vector<std::string>& files) {
    std::map<std::string, std::string> config;
    for (const auto& file : files) {
        std::ifstream config_file(file);
        if (!config_file.is_open()) {
            std::cerr << "Warning: Could not open config file: " << file << std::endl;
            continue;
        }
        std::string line;
        while (std::getline(config_file, line)) {
            // Ignore comments and empty lines
            if (line.empty() || line[0] == '#') {
                continue;
            }
            std::istringstream iss(line);
            std::string key, value;
            if (std::getline(iss, key, '=') && std::getline(iss, value)) {
                // A more robust parser would trim whitespace from key and value
                config[key] = value;
            }
        }
    }
    return config;
}

/**
 * @brief Constructs a PostgreSQL connection string from a config map.
 * @param config The map of configuration key-value pairs.
 * @return A libpqxx-compatible connection string.
 */
std::string build_connection_string(const std::map<std::string, std::string>& config) {
    std::string connstr;
    // Whitelist of allowed libpqxx connection string parameters to prevent injection
    const std::set<std::string> allowed = {
            "host", "hostaddr", "port", "dbname", "user", "password",
            "connect_timeout", "client_encoding", "options", "application_name",
            "fallback_application_name", "keepalives", "keepalives_idle",
            "keepalives_interval", "keepalives_count", "sslmode", "sslcompression",
            "sslcert", "sslkey", "sslrootcert"
    };
    for (const auto& kv : config) {
        if (allowed.count(kv.first)) {
            connstr += kv.first + "=" + kv.second + " ";
        }
    }
    return connstr;
}

/**
 * @brief Main entry point for the visualiser application.
 *
 * This function sets up the application environment, including logging,
 * configuration reading, database connection, and then launches the
 * visualiser.
 */
int main(int argc, char** argv) {
    try {
        // 1. Initialise Logger
        Logger logger("visualiser.log");
        logger << "Visualiser starting up...\n";

        // 2. Read configuration files to build database connection string
        std::vector<std::string> config_files = { "simulation.conf", "Visualiser.conf" };
        auto config = read_config(config_files);
        std::string connection_str = build_connection_string(config);

        if (connection_str.empty()) {
            logger << "Error: No valid database connection parameters found in config files.\n";
            std::cerr << "Error: No valid database connection parameters found in config files. Please check Visualiser.conf and simulation.conf\n";
            return 1;
        }

        // 3. Establish database connection
        logger << "Connecting to database...\n";
        auto conn = std::make_shared<pqxx::connection>(connection_str);
        if (!conn->is_open()) {
            logger << "Error: Unable to connect to PostgreSQL database.\n";
            std::cerr << "Error: Unable to connect to PostgreSQL database.\n";
            return 1;
        }
        logger << "Successfully connected to database.\n";

        // 4. Launch WebSocket Server in a separate thread
        WebSocketServer ws_server;
        std::thread ws_thread([&ws_server]() {
            ws_server.run(9002);
        });
        logger << "WebSocket server started on port 9002.\n";

        // 5. Create and run the Visualiser instance
        Visualiser vis(conn, logger, ws_server);
        vis.visualise();

        // 6. Clean up after visualise() returns (window is closed)
        logger << "Visualisation window closed. Shutting down.\n";
        // signal the ws_server to stop gracefully.
        ws_thread.join();
    }
    catch (const std::exception& e) {
        // Log any uncaught exceptions to a dedicated crash log
        Logger logger("visualiser_crash.log");
        logger << "FATAL ERROR: " << e.what() << '\n';
        std::cerr << "A fatal error occurred. Check visualiser_crash.log for details: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
