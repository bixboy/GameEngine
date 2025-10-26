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

    for (int i = 0; i < argc; ++i)
        std::cout << "  argv[" << i << "] = " << argv[i] << "\n";

    // ─────────────────────────────────────────────
    // 🎯 Mode 1 : Génération d'un seul header (--single <path>)
    // ─────────────────────────────────────────────
    if (argc == 3 && std::string(argv[1]) == "--single")
    {
        fs::path headerPath = fs::weakly_canonical(argv[2]);
        fs::path exeDir = fs::path(argv[0]).parent_path();
        fs::path generatedDir = exeDir / "Intermediate" / "GeneratedHeaders";

        std::cout << "[BixHeaderTool] Single-file mode enabled\n";
        std::cout << "  Header: " << headerPath << "\n";
        std::cout << "  Output: " << generatedDir << "\n";

        if (!fs::exists(headerPath))
        {
            std::cerr << "[BixHeaderTool] ❌ Header not found: " << headerPath << "\n";
            return 2;
        }

        // ✅ Crée le dossier si nécessaire
        std::error_code ec;
        fs::create_directories(generatedDir, ec);
        if (ec)
        {
            std::cerr << "[BixHeaderTool] ❌ Cannot create output dir: " << generatedDir << "\n";
            return 3;
        }

        HeaderParseResult parseResult = ParseHeader(headerPath);
        if (!parseResult.ContainsReflection)
        {
            std::cout << "[BixHeaderTool] Skipped (no reflection): " << headerPath.filename().string() << "\n";
            return 0;
        }

        std::string generated = GenerateHeader(headerPath, parseResult);
        if (generated.empty())
        {
            std::cout << "[BixHeaderTool] No content generated for: " << headerPath.filename().string() << "\n";
            return 0;
        }

        fs::path outputPath = generatedDir / (headerPath.stem().string() + ".generated.h");

        if (WriteFileIfDifferent(outputPath, generated))
        {
            std::cout << "[BixHeaderTool] ✅ Generated " << outputPath << "\n";
        }
        else
        {
            std::cout << "[BixHeaderTool] ⏩ No changes for " << outputPath << "\n";
        }

        return 0;
    }

    // ─────────────────────────────────────────────
    // 🎯 Mode 2 : Génération complète
    // ─────────────────────────────────────────────
    if (argc < 4)
    {
        std::cerr << "[BixHeaderTool] Usage:\n";
        std::cerr << "  BixHeaderTool <include_root> <samples_root> <output_dir>\n";
        std::cerr << "  or BixHeaderTool --single <header_path>\n";
        return 1;
    }

    const fs::path includeRoot = argv[1];
    const fs::path samplesRoot = argv[2];
    const fs::path generatedOutputDir = argv[3];

    std::vector roots = { includeRoot, samplesRoot };
    for (int i = 4; i < argc; ++i)
        roots.emplace_back(argv[i]);

    std::vector<fs::path> headers = CollectHeaderFiles(roots);
    if (headers.empty())
    {
        std::cout << "[BixHeaderTool] No headers to process.\n";
        return 0;
    }

    std::error_code cleanupErr;
    fs::remove_all(generatedOutputDir, cleanupErr);
    fs::create_directories(generatedOutputDir, cleanupErr);

    bool anyUpdated = false;
    for (const auto& header : headers)
    {
        HeaderParseResult parseResult = ParseHeader(header);
        if (!parseResult.ContainsReflection)
            continue;

        std::string generated = GenerateHeader(header, parseResult);
        if (generated.empty())
            continue;

        fs::path generatedPath = generatedOutputDir / (header.stem().string() + ".generated.h");

        if (WriteFileIfDifferent(generatedPath, generated))
        {
            std::cout << "[BixHeaderTool] Generated " << generatedPath.filename().string() << "\n";
            anyUpdated = true;
        }
    }

    std::cout << "[BixHeaderTool] Done. "
              << (anyUpdated ? "Some headers were updated." : "No changes.")
              << "\n";

    return 0;
}
