#ifndef AVTIMERCALLBACK_H
#define AVTIMERCALLBACK_H

#include <memory>
#include <vector>
#include "ThreadSafeQueue.h"
#include "vtkincludes.h"

class avTimerCallback : public vtkCommand
{
public:
    static avTimerCallback *New();
    void Execute(vtkObject *vtkNotUsed(caller), unsigned long eventId, void *vtkNotUsed(callData)) override ;
    void setAudio(ThreadSafeQueue<std::vector<std::tuple<double, double>>>& aQueue) ;
    void setPoints(const vtkSmartPointer<vtkPoints>& points);
    void setLines(const vtkSmartPointer<vtkCellArray>& lines);
    void setPolyData(const vtkSmartPointer<vtkPolyData>& polydata);
    void setActor(const vtkSmartPointer<vtkActor>& actor);
    void setMapper(const vtkSmartPointer<vtkPolyDataMapper>& mapper) ;
    void setRenderer(const vtkSmartPointer<vtkRenderer>& renderer){}
    void setRenderWindow(const vtkSmartPointer<vtkRenderWindow>& renderWindow);
    int avTimerCount{};
    vtkSmartPointer<vtkActor> avActor = vtkSmartPointer<vtkActor>::New();
    vtkSmartPointer<vtkPolyDataMapper> avMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    vtkSmartPointer<vtkRenderer> avRenderer = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderWindow> avRenderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    vtkSmartPointer<vtkPolyData> avPolyData = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkPoints> avPoints = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> avLines = vtkSmartPointer<vtkCellArray>::New();
    std::vector<std::tuple<double, double>> capturedAudio;
    ThreadSafeQueue<std::vector<std::tuple<double, double>>> *avQueue{};
};
#endif