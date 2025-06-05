#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

int main(int, char*[])
{
    // 1) Create renderer + window + interactor
    vtkSmartPointer<vtkRenderer> renderer =
            vtkSmartPointer<vtkRenderer>::New();

    vtkSmartPointer<vtkRenderWindow> window =
            vtkSmartPointer<vtkRenderWindow>::New();
    window->AddRenderer(renderer);
    window->SetSize(400, 300);
    window->SetOffScreenRendering(0);

    vtkSmartPointer<vtkRenderWindowInteractor> iren =
            vtkSmartPointer<vtkRenderWindowInteractor>::New();
    iren->SetRenderWindow(window);

    // 2) Draw a background color so we know the window is there
    renderer->SetBackground(0.1, 0.2, 0.3);

    // 3) Render once, then start interaction
    window->Render();
    iren->Initialize();
    iren->Start();

    return 0;
}
