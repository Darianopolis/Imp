#include "imp.hpp"

int main()
{
    imp::Importer importer;
    importer.SetBaseDir("model_dir");
    importer.LoadFile("model_dir/scene.gltf");
    importer.ReportStatistics();
}