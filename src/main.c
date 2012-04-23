#include <stdlib.h>
#include <rime_api.h>

int main(int argc, char** argv)
{
    RimeDeployerInitialize(NULL);
    return RimeDeployWorkspace() ? 0 : 1;
}