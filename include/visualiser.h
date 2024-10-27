// visualiser.h
#ifndef AARNN_VISUALISER_H
#define AARNN_VISUALISER_H

#include <memory>
#include <string>

#include <pqxx/pqxx>

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkUnsignedCharArray.h>
#include "wss.h"

// Forward declaration of Logger class
class Logger;

/**
 * @brief Visualiser class responsible for data retrieval and visualization.
 */
class Visualiser {
public:
    /**
     * @brief Constructs a Visualiser object.
     * @param conn Shared pointer to a pqxx::connection object.
     * @param logger Reference to a Logger object for logging purposes.
     * @param ws_server Reference to a WebSocketServer object for WebSocket communication.
     */
    Visualiser(std::shared_ptr<pqxx::connection> conn, Logger& logger, WebSocketServer& ws_server);

    /**
     * @brief Starts the visualization process.
     *        Continuously fetches data and updates the visualization.
     */
    void visualise();

private:
    /**
     * @brief Sets up the VTK rendering components.
     * @param renderer Reference to a VTK renderer object.
     * @param renderWindow Reference to a VTK render window object.
     * @param interactor Reference to a VTK render window interactor object.
     */
    void setupVTK(vtkSmartPointer<vtkRenderer>& renderer,
                  vtkSmartPointer<vtkRenderWindow>& renderWindow,
                  vtkSmartPointer<vtkRenderWindowInteractor>& interactor);

    /**
     * @brief Inserts dendrite branches into VTK structures recursively.
     * @param txn Reference to the current pqxx transaction.
     * @param points VTK points object to store spatial coordinates.
     * @param lines VTK cell array to store lines between points.
     * @param glyphPoints VTK points object for glyph positioning.
     * @param glyphVectors VTK float array for glyph orientation vectors.
     * @param glyphTypes VTK unsigned char array for glyph type identifiers.
     * @param glyphColors VTK unsigned char array for glyph colors based on energy levels.
     * @param parent_soma_id ID of the parent soma (-1 if not applicable).
     * @param parent_dendrite_id ID of the parent dendrite (-1 if connected to soma).
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
     * @brief Inserts axon branches into VTK structures recursively.
     * @param txn Reference to the current pqxx transaction.
     * @param points VTK points object to store spatial coordinates.
     * @param lines VTK cell array to store lines between points.
     * @param glyphPoints VTK points object for glyph positioning.
     * @param glyphVectors VTK float array for glyph orientation vectors.
     * @param glyphTypes VTK unsigned char array for glyph type identifiers.
     * @param glyphColors VTK unsigned char array for glyph colors based on energy levels.
     * @param parent_axon_id ID of the parent axon (-1 if not applicable).
     * @param parent_axon_branch_id ID of the parent axon branch (-1 if not applicable).
     * @param parent_axon_hillock_id ID of the parent axon hillock (-1 if connected directly).
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

    // Data Members
    std::shared_ptr<pqxx::connection> conn_; ///< Shared pointer to the PostgreSQL connection.
    Logger& logger_;                         ///< Reference to the Logger object for logging.
    WebSocketServer& ws_server_;             ///< Reference to the WebSocketServer object for communication.
};

#endif // AARNN_VISUALISER_H
