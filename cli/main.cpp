#include "imp.hpp"

int main(int argc, char* argv[])
{
    std::filesystem::path path;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        path = arg;
    }

    if (!std::filesystem::exists(path)) {
        std::cout << "Did not pass a valid path in\n";
        return 1;
    }

    imp::Importer importer;
    importer.SetBaseDir(path.parent_path());
    importer.LoadFile(path);
    importer.ReportStatistics();
}