#ifndef UTILS_H
#define UTILS_H

#include <memory>
#include <vector>
#include "Axon.h"
#include "AxonBouton.h"
#include "AxonBranch.h"
#include "SynapticGap.h"
#include "Dendrite.h"
#include "DendriteBranch.h"
#include "Position.h"
#include "Neuron.h"
#include "SensoryReceptor.h"
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/convex_hull_3.h>
#include <CGAL/Polyhedron_3.h>
#include <tuple>
#include <iostream>
#include <cmath>
#include <limits>
#include <random>
#include "vtkincludes.h"

class DendriteBranch;
class Dendrite;

const int SAMPLE_RATE = 16000;
const int FRAMES_PER_BUFFER = 256;
typedef float SAMPLE;
[[maybe_unused]] static int gNumNoInputs = 0;
using PositionPtr = std::shared_ptr<Position>;
inline std::uniform_real_distribution<> distr(-0.15, 1.0 - 0.15);
inline std::mt19937 gen(12345);

void atomic_add(std::atomic<double>& atomic_val, double value);

void addDendriteBranchToRenderer(vtkRenderer *renderer,
                                 const std::shared_ptr<DendriteBranch> &dendriteBranch,
                                 const std::shared_ptr<Dendrite> &dendrite,
                                 const vtkSmartPointer<vtkPoints> &points,
                                 const vtkSmartPointer<vtkIdList> &pointIds);

void addAxonBranchToRenderer(vtkRenderer *renderer,
                             const std::shared_ptr<AxonBranch> &axonBranch,
                             const std::shared_ptr<Axon> &axon,
                             const vtkSmartPointer<vtkPoints> &points,
                             const vtkSmartPointer<vtkIdList> &pointIds);

void addSynapticGapToRenderer(vtkRenderer *renderer, const std::shared_ptr<SynapticGap> &synapticGap);

void addDendriteBranchPositionsToPolyData(vtkRenderer *renderer,
                                          const std::shared_ptr<DendriteBranch> &dendriteBranch,
                                          const vtkSmartPointer<vtkPoints> &definePoints,
                                          vtkSmartPointer<vtkIdList> pointIDs);

void addAxonBranchPositionsToPolyData(vtkRenderer *renderer,
                                      const std::shared_ptr<AxonBranch> &axonBranch,
                                      const vtkSmartPointer<vtkPoints> &definePoints,
                                      vtkSmartPointer<vtkIdList> pointIDs);

std::tuple<double, double, double> get_coordinates(int i, int total_points, int points_per_layer);

std::string base64_encode(const unsigned char* data, size_t length);

bool convertStringToBool(const std::string& value);

void associateSynapticGap(Neuron& neuron1, Neuron& neuron2, double proximityThreshold);

void associateSynapticGap(SensoryReceptor& receptor, Neuron& neuron, double proximityThreshold);

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef K::Point_3 Point_3;
typedef CGAL::Polyhedron_3<K> Polyhedron;

#endif  // UTILS_H
