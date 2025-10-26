#include <iostream>
#include <filesystem>
#include "Parser/HeaderParser.h"
#include "Generator/HeaderGenerator.h"
#include "FileSystem/FileUtils.h"

using namespace BixTool;
namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
    std::cout << "[BixHeaderTool] Launching tool...\n";
    std::cout << "  argc = " << argc << "\n";

    for (int i = 0; i < argc; ++i)
        std::cout << "  argv[" << i << "] = " << argv[i] << "\n";

    if (argc < 4)
    {
        std::cerr << "[BixHeaderTool] Usage: BixHeaderTool <include_root> <samples_root> <output_dir>\n";
        for (int i = 0; i < argc; ++i)
            std::cerr << "argv[" << i << "] = " << argv[i] << "\n";

        return 1;
    }

    const fs::path includeRoot = argv[1];
    const fs::path samplesRoot = argv[2];
    const fs::path generatedOutputDir = argv[3];

    // ✅ Collecte les headers à partir des racines
    std::vector<fs::path> roots = { includeRoot, samplesRoot };
    for (int i = 4; i < argc; ++i)
        roots.emplace_back(argv[i]);

    std::vector<fs::path> headers = CollectHeaderFiles(roots);
    if (headers.empty())
    {
        std::cout << "[BixHeaderTool] No headers to process.\n";
        return 0;
    }

    // ✅ Nettoie le dossier avant génération
    std::error_code cleanupErr;
    fs::remove_all(generatedOutputDir, cleanupErr);
    if (cleanupErr)
        std::cerr << "[BixHeaderTool] Warning: failed to clean " << generatedOutputDir << "\n";

    // ✅ Crée le dossier s’il n’existe pas encore
    std::error_code createErr;
    fs::create_directories(generatedOutputDir, createErr);
    if (createErr)
    {
        std::cerr << "[BixHeaderTool] Error: cannot create output dir " << generatedOutputDir << "\n";
        return 2;
    }

    bool anyUpdated = false;

    // ✅ Boucle principale : parse + génère + écrit
    for (const auto& header : headers)
    {
        HeaderParseResult parseResult = ParseHeader(header);
        if (!parseResult.ContainsReflection)
        {
            std::cout << "[BixHeaderTool] Skipped (no reflection): " << header.filename().string() << "\n";
            continue;
        }

        std::string generated = GenerateHeader(header, parseResult);
        if (generated.empty())
            continue;

        fs::path generatedPath = generatedOutputDir / (header.stem().string() + ".generated.h");

        if (WriteFileIfDifferent(generatedPath, generated))
        {
            std::cout << "[BixHeaderTool] Generated "
                      << generatedPath.filename().string()
                      << " -> " << fs::absolute(generatedPath).string() << "\n";
                      
            anyUpdated = true;
        }
    }

    std::cout << "[BixHeaderTool] Done. "
              << (anyUpdated ? "Some headers were updated." : "No changes.")
              << "\n";

    return 0;
}
