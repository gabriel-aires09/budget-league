#include "App.h"

int main(int argc, char **argv)
{
    App app;
    if (!app.Initialize(argc, argv))
        return 1;

    app.Run();
    app.Shutdown();
    return 0;
}
