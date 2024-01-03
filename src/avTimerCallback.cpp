#include "../include/avTimerCallback.h"

void avTimerCallback::setAudio(ThreadSafeQueue<std::vector<std::tuple<double, double>>> &aQueue)
{
    this->avQueue = &aQueue;
}

void avTimerCallback::setPoints(const vtkSmartPointer<vtkPoints> &points)
{
    this->avPoints = points;
}

void avTimerCallback::setLines(const vtkSmartPointer<vtkCellArray> &lines)
{
    this->avLines = lines;
}

void avTimerCallback::setPolyData(const vtkSmartPointer<vtkPolyData> &polydata)
{
    this->avPolyData = polydata;
}

void avTimerCallback::setActor(const vtkSmartPointer<vtkActor> &actor)
{
    this->avActor = actor;
}

void avTimerCallback::setMapper(const vtkSmartPointer<vtkPolyDataMapper> &mapper)
{
    this->avMapper = mapper;
}

void avTimerCallback::setRenderer(const vtkSmartPointer<vtkRenderer> &renderer)
{
    this->avRenderer = renderer;
}

void avTimerCallback::setRenderWindow(const vtkSmartPointer<vtkRenderWindow> &renderWindow)
{
    this->avRenderWindow = renderWindow;
}