// visualiser.h

#ifndef AARNN_VISUALISER_H
#define AARNN_VISUALISER_H

#include <memory>
#include <mutex>
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>

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

#include "wss.h"  // For WebSocketServer

// Forward declaration of Logger class
class Logger;

/**
 * @brief Visualiser class is responsible for fetching data from PostgreSQL
 *        and rendering it in a VTK window, with periodic updates driven by a timer.
 */
class Visualiser {
public:
    /**
     * @brief Constructs a Visualiser object.
     * @param conn Shared pointer to a pqxx::connection (PostgreSQL) object.
     * @param logger Reference to a Logger instance for logging.
     * @param ws_server Reference to a WebSocketServer instance for WebSocket communication.
     */
    Visualiser(std::shared_ptr<pqxx::connection> conn,
               Logger& logger,
               WebSocketServer& ws_server);

    /**
     * @brief Destructor for Visualiser. Cleans up VTK objects automatically via smart pointers.
     */
    ~Visualiser();

    /**
     * @brief Starts the VTK rendering and update loop.
     *        This will open an on-screen window and begin periodic data refreshes.
     */
    void visualise();

/**
 * @brief Builds a single frame: queries the database, rebuilds the VTK pipeline,
 *        and triggers a render. Invoked on each timer event.
 */
void buildAndRenderFrame();

private:

    /**
     * @brief Recursively inserts dendrite branches (and their child dendrites/boutons)
     *        into the VTK point/line/glyph arrays.
     * @param txn The active pqxx transaction for database queries.
     * @param points VTK points container for spatial coordinates.
     * @param lines VTK cell array for line segments.
     * @param glyphPoints VTK points for glyph placement.
     * @param glyphVectors VTK float array storing orientation vectors for glyph scaling.
     * @param glyphTypes VTK unsigned char array storing integer glyph-type indices.
     * @param glyphColors VTK unsigned char array storing RGB colors per glyph.
     * @param parent_soma_id The soma ID under which to query dendrite branches, or -1 if not used.
     * @param parent_dendrite_id The dendrite ID under which to query further branches, or -1 if not used.
     */
    void insertDendriteBranches(pqxx::transaction_base& txn,
                                vtkSmartPointer<vtkPoints>& points,
                                vtkSmartPointer<vtkCellArray>& lines,
                                vtkSmartPointer<vtkPoints>& glyphPoints,
                                vtkSmartPointer<vtkFloatArray>& glyphVectors,
                                vtkSmartPointer<vtkUnsignedCharArray>& glyphTypes,
                                vtkSmartPointer<vtkUnsignedCharArray>& glyphColors,
                                int parent_soma_id = -1,
                                int parent_dendrite_id = -1);

    /**
     * @brief Recursively inserts axon branches (and their child axons, boutons, gaps)
     *        into the VTK point/line/glyph arrays.
     * @param txn The active pqxx transaction for database queries.
     * @param points VTK points container for spatial coordinates.
     * @param lines VTK cell array for line segments.
     * @param glyphPoints VTK points for glyph placement.
     * @param glyphVectors VTK float array storing orientation vectors for glyph scaling.
     * @param glyphTypes VTK unsigned char array storing integer glyph-type indices.
     * @param glyphColors VTK unsigned char array storing RGB colors per glyph.
     * @param parent_axon_id The axon ID under which to query branches, or -1 if not used.
     * @param parent_axon_branch_id The axon branch ID under which to query further branches, or -1 if not used.
     * @param parent_axon_hillock_id The axon hillock ID under which to query branches, or -1 if not used.
     */
    void insertAxons(pqxx::transaction_base& txn,
                     vtkSmartPointer<vtkPoints>& points,
                     vtkSmartPointer<vtkCellArray>& lines,
                     vtkSmartPointer<vtkPoints>& glyphPoints,
                     vtkSmartPointer<vtkFloatArray>& glyphVectors,
                     vtkSmartPointer<vtkUnsignedCharArray>& glyphTypes,
                     vtkSmartPointer<vtkUnsignedCharArray>& glyphColors,
                     int parent_axon_id = -1,
                     int parent_axon_branch_id = -1,
                     int parent_axon_hillock_id = -1);

    //=== Data Members ===//

    std::shared_ptr<pqxx::connection> conn_;  ///< PostgreSQL connection.
    Logger&                           logger_; ///< Logger instance for error/info messages.
    WebSocketServer&                  ws_server_; ///< WebSocket server for real-time updates (unused in this version).

    // VTK pipeline objects:
    vtkSmartPointer<vtkPoints>             points_;         ///< Stores spatial coordinates (all line endpoints).
    vtkSmartPointer<vtkCellArray>          lines_;          ///< Stores line segments between points.
    vtkSmartPointer<vtkPoints>             glyphPoints_;    ///< Stores glyph anchor positions.
    vtkSmartPointer<vtkFloatArray>         glyphVectors_;   ///< Orientation vectors for glyph scaling.
    vtkSmartPointer<vtkUnsignedCharArray>  glyphTypes_;     ///< Integer glyph-type index per point.
    vtkSmartPointer<vtkUnsignedCharArray>  glyphColors_;    ///< RGB color tuples per glyph.

    vtkSmartPointer<vtkPolyData>           polyData_;       ///< PolyData for lines.
    vtkSmartPointer<vtkPolyData>           glyphPolyData_;  ///< PolyData for glyphs.

    vtkSmartPointer<vtkSphereSource>       sphereSource_;   ///< Glyph source: sphere.
    vtkSmartPointer<vtkCubeSource>         cubeSource_;     ///< Glyph source: cube.
    vtkSmartPointer<vtkCylinderSource>     cylinderSource_; ///< Glyph source: cylinder.

    vtkSmartPointer<vtkGlyph3D>            glyph3D_;        ///< Glyph3D mapper dispatching to sources.

    vtkSmartPointer<vtkPolyDataMapper>      lineMapper_;     ///< Mapper for line PolyData.
    vtkSmartPointer<vtkPolyDataMapper>      glyphMapper_;    ///< Mapper for glyph PolyData.
    vtkSmartPointer<vtkActor>               lineActor_;      ///< Actor for rendering lines.
    vtkSmartPointer<vtkActor>               glyphActor_;     ///< Actor for rendering glyphs.

    vtkSmartPointer<vtkRenderWindow>        renderWindow_;   ///< Shared render window.

    std::mutex                              pipelineMutex_;  ///< Guards pipeline rebuilding between timer callbacks.
};

#endif // AARNN_VISUALISER_H
