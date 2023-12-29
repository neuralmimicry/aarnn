#include <iostream>
#include "../include/vtkincludes.h"
#include "../include/avTimerCallback.h"

static avTimerCallback::avTimerCallback *New()
{
    auto *avCB = new avTimerCallback;
    avCB->avTimerCount = 0;
    return avCB;
}

void avTimerCallback::Execute(vtkObject *vtkNotUsed(caller), unsigned long eventId,
                              void *vtkNotUsed(callData)) override
{
    if (vtkCommand::TimerEvent == eventId)
    {
        ++this->avTimerCount;
    }

    capturedAudio = avQueue->pop();
    // avPolyData->SetLines(nullptr);
    // avPolyData->SetPoints(nullptr);
    std::cout << ".";
    // Plot the frequency data.
    avLines->Reset();
    avPoints->Reset();
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    for (const auto &point : capturedAudio)
    {
        vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
        x = double(std::get<0>(point)) / 160.0;
        y = double(std::get<1>(point)) / 160.0;
        avPoints->InsertNextPoint(x, 0.0, z);
        line->GetPointIds()->SetId(0, avPoints->GetNumberOfPoints() - 1); // the first point of the line
        avPoints->InsertNextPoint(x, y, z);
        line->GetPointIds()->SetId(1, avPoints->GetNumberOfPoints() - 1); // the first point of the line
        avLines->InsertNextCell(line);
        // std::cout << avPoints->GetNumberOfPoints() << std::endl;
    }

    avPolyData->SetPoints(avPoints);
    avPolyData->SetLines(avLines);
    // std::cout << avPolyData->GetNumberOfPoints() << ", " << avPolyData->GetNumberOfLines() << std::endl;
    avMapper->SetInputData(avPolyData);
    avRenderer->RemoveActor(avActor);
    avActor = vtkSmartPointer<vtkActor>::New();
    avActor->SetMapper(avMapper);
    avActor->GetProperty()->SetColor(0.7, 0.7, 0.0);
    avRenderer->AddActor(avActor);
    avRenderer->Modified();
}

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