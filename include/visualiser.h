#ifndef AARNN_VISUALISER_H
#define AARNN_VISUALISER_H

#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <map>

#include <pqxx/pqxx>

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkUnsignedCharArray.h>
#include <vtkPolyData.h>
#include <vtkSphereSource.h>
#include <vtkCubeSource.h>
#include <vtkCylinderSource.h>
#include <vtkGlyph3D.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>

#include "wss.h"  // For WebSocketServer

// Forward declaration of Logger
class Logger;

// Glyph type identifiers
constexpr unsigned char NEURON_GLYPH           = 0;
constexpr unsigned char DENDRITE_GLYPH         = 1;
constexpr unsigned char DENDRITE_BRANCH_GLYPH  = 2;
constexpr unsigned char DENDRITE_BOUTON_GLYPH  = 3;
constexpr unsigned char SOMA_GLYPH             = 4;
constexpr unsigned char AXON_HILLOCK_GLYPH     = 5;
constexpr unsigned char AXON_BRANCH_GLYPH      = 6;
constexpr unsigned char AXON_GLYPH             = 7;
constexpr unsigned char AXON_BOUTON_GLYPH      = 8;
constexpr unsigned char SYNAPTIC_GAP_GLYPH     = 9;

/**
 * @brief Holds position and energy data for a neural component.
 */
struct ComponentData {
    std::array<double,3> pos;
    double energy;
    double max_energy;
};

/**
 * @brief Visualiser: bulk-fetches neural data, renders in VTK, and broadcasts JSON frames.
 */
class Visualiser {
public:
    Visualiser(std::shared_ptr<pqxx::connection> conn,
               Logger& logger,
               WebSocketServer& ws_server);
    ~Visualiser();

    /**
     * @brief Launches VTK window and begins periodic updates.
     */
    void visualise();

    /**
     * @brief Rebuilds scene from DB and re-renders; broadcasts JSON.
     */
    void buildAndRenderFrame();

    /**
     * @brief Returns current update interval in milliseconds.
     */
    int getUpdateInterval() const { return update_interval_ms_; }

private:
    // Core steps
    void initializeVTKPipeline();
    void fetchAllData(pqxx::transaction_base& txn);
    static unsigned char* computeColor(double energy, double max_energy);

    // Recursive builders
    void buildAxonTree(int hillock_id, vtkIdType parent_pt);
    void buildAxonBranch(int branch_id, vtkIdType parent_pt);
// In visualiser.h
    void buildDendriteTree(int branch_id,
                           vtkIdType parent_pt,
                           bool is_top_level = true);

    void createLine(vtkIdType p1, vtkIdType p2);

    // Dependencies
    std::shared_ptr<pqxx::connection> conn_;
    Logger&                           logger_;
    WebSocketServer&                  ws_server_;

    // Update timing
    int update_interval_ms_;

    // VTK pipeline objects
    vtkSmartPointer<vtkPoints>            points_;
    vtkSmartPointer<vtkCellArray>         lines_;
    vtkSmartPointer<vtkPoints>            glyphPoints_;
    vtkSmartPointer<vtkFloatArray>        glyphVectors_;
    vtkSmartPointer<vtkUnsignedCharArray> glyphTypes_;
    vtkSmartPointer<vtkUnsignedCharArray> glyphColors_;
    vtkSmartPointer<vtkPolyData>          polyData_;
    vtkSmartPointer<vtkPolyData>          glyphPolyData_;
    vtkSmartPointer<vtkSphereSource>      sphereSource_;
    vtkSmartPointer<vtkCubeSource>        cubeSource_;
    vtkSmartPointer<vtkCylinderSource>    cylinderSource_;
    vtkSmartPointer<vtkGlyph3D>           glyph3D_;
    vtkSmartPointer<vtkPolyDataMapper>    lineMapper_;
    vtkSmartPointer<vtkPolyDataMapper>    glyphMapper_;
    vtkSmartPointer<vtkActor>             lineActor_;
    vtkSmartPointer<vtkActor>             glyphActor_;
    vtkSmartPointer<vtkRenderer>          renderer_;
    vtkSmartPointer<vtkRenderWindow>      renderWindow_;
    vtkSmartPointer<vtkRenderWindowInteractor> interactor_;

    // Thread-safety for rebuilds
    std::mutex pipelineMutex_;

    // In-memory data stores
    std::map<int, ComponentData> somas_;
    std::map<int, ComponentData> hillocks_;
    std::map<int, ComponentData> axons_;
    std::map<int, ComponentData> axon_branches_;
    std::map<int, ComponentData> axon_boutons_;
    std::map<int, ComponentData> dendrite_branches_;
    std::map<int, ComponentData> dendrites_;
    std::map<int, ComponentData> dendrite_boutons_;

    std::map<int,int> soma_to_hillock_;
    std::map<int,int> hillock_to_axon_;
    std::map<int,std::vector<int>> axon_to_branches_;
    std::map<int,std::vector<int>> branch_to_sub_branches_;
    std::map<int,std::vector<int>> branch_to_axons_;
    std::map<int,std::vector<int>> axon_to_boutons_;
    std::map<int,std::vector<int>> bouton_to_synaptic_gap_;
    std::map<int,std::vector<int>> soma_to_dendrite_branches_;
    std::map<int,std::vector<int>> dendrite_to_branches_;
    std::map<int,std::vector<int>> branch_to_dendrites_;
    std::map<int,std::vector<int>> dendrite_to_boutons_;
    std::vector<int> glyphOriginalIds_; // To store the DB ID for each glyph
};

#endif // AARNN_VISUALISER_H
