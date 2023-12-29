#ifndef NVTIMERCALLBACK_H
#define NVTIMERCALLBACK_H

#include <iostream>
#include <mutex>
#include <string_view>
#include <syncstream>
#include <thread>

#include "vtkincludes.h"
#include "Neuron.h"

class nvTimerCallback : public vtkCommand
{
public:
    static nvTimerCallback *New();
    void Execute(vtkObject *vtkNotUsed(caller), unsigned long eventId, void *vtkNotUsed(callData)) override ;
    void setNeurons(const std::vector<std::shared_ptr<Neuron>>& visualiseNeurons) ;
    void setMutex(std::mutex& mutex) ;
    void setPoints(const vtkSmartPointer<vtkPoints>& points);
    void setLines(const vtkSmartPointer<vtkCellArray>& lines);
    void setPolyData(const vtkSmartPointer<vtkPolyData>& polydata);
    void setActor(const vtkSmartPointer<vtkActor>& actor) ;
    void setMapper(const vtkSmartPointer<vtkPolyDataMapper>& mapper) ;
    void setRenderer(const vtkSmartPointer<vtkRenderer>& renderer) ;
    void setRenderWindow(const vtkSmartPointer<vtkRenderWindow>& renderWindow);
    std::vector<std::shared_ptr<Neuron>> nvNeurons;
    int nvTimerCount{};
    vtkSmartPointer<vtkActor> nvActor = vtkSmartPointer<vtkActor>::New();
    vtkSmartPointer<vtkPolyDataMapper> nvMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    vtkSmartPointer<vtkRenderer> nvRenderer = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderWindow> nvRenderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    vtkSmartPointer<vtkPolyData> nvPolyData = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkPoints> nvPoints = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> nvLines = vtkSmartPointer<vtkCellArray>::New();
    std::mutex* nvMutex{};
};

#endif