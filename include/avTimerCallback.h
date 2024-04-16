#ifndef AVTIMERCALLBACK_H_INCLUDED
#define AVTIMERCALLBACK_H_INCLUDED

#include "ThreadSafeQueue.h"
#include "vtkincludes.h"

#include <memory>
#include <vector>

class avTimerCallback : public vtkCommand
{
    public:
    static avTimerCallback *New()
    {
        auto *avCB         = new avTimerCallback;
        avCB->avTimerCount = 0;
        return avCB;
    }
    void Execute(vtkObject *vtkNotUsed(caller), unsigned long eventId, void *vtkNotUsed(callData)) override
    {
        if(vtkCommand::TimerEvent == eventId)
        {
            ++this->avTimerCount;
        }

        capturedAudio = avQueue->pop();

        // Plot the frequency data.
        avLines->Reset();
        avPoints->Reset();
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
        for(const auto &point: capturedAudio)
        {
            vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
            x                             = double(std::get<0>(point)) / 160.0;
            y                             = double(std::get<1>(point)) / 160.0;
            avPoints->InsertNextPoint(x, 0.0, z);
            line->GetPointIds()->SetId(0, avPoints->GetNumberOfPoints() - 1);  // the first point of the line
            avPoints->InsertNextPoint(x, y, z);
            line->GetPointIds()->SetId(1, avPoints->GetNumberOfPoints() - 1);  // the first point of the line
            avLines->InsertNextCell(line);
            // std::cout << avPoints->GetNumberOfPoints() << std::endl;
        }

        avPolyData->SetPoints(avPoints);
        avPolyData->SetLines(avLines);
        avMapper->SetInputData(avPolyData);
        avRenderer->RemoveActor(avActor);
        avActor = vtkSmartPointer<vtkActor>::New();
        avActor->SetMapper(avMapper);
        avActor->GetProperty()->SetColor(0.7, 0.7, 0.0);
        avRenderer->AddActor(avActor);
        avRenderer->Modified();
    }

    void                                    setAudio(ThreadSafeQueue<std::vector<std::tuple<double, double>>> &aQueue);
    void                                    setPoints(const vtkSmartPointer<vtkPoints> &points);
    void                                    setLines(const vtkSmartPointer<vtkCellArray> &lines);
    void                                    setPolyData(const vtkSmartPointer<vtkPolyData> &polydata);
    void                                    setActor(const vtkSmartPointer<vtkActor> &actor);
    void                                    setMapper(const vtkSmartPointer<vtkPolyDataMapper> &mapper);
    void                                    setRenderer(const vtkSmartPointer<vtkRenderer> &renderer);
    void                                    setRenderWindow(const vtkSmartPointer<vtkRenderWindow> &renderWindow);

    // TODO: This should be private:
    int                                     avTimerCount{};
    vtkSmartPointer<vtkActor>               avActor        = vtkSmartPointer<vtkActor>::New();
    vtkSmartPointer<vtkPolyDataMapper>      avMapper       = vtkSmartPointer<vtkPolyDataMapper>::New();
    vtkSmartPointer<vtkRenderer>            avRenderer     = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderWindow>        avRenderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    vtkSmartPointer<vtkPolyData>            avPolyData     = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkPoints>              avPoints       = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray>           avLines        = vtkSmartPointer<vtkCellArray>::New();
    std::vector<std::tuple<double, double>> capturedAudio;
    ThreadSafeQueue<std::vector<std::tuple<double, double>>> *avQueue{};
};
#endif  // AVTIMERCALLBACK_H_INCLUDED