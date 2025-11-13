#include <iostream>
#include <filesystem>
#include <vector>
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
            std::cout << "[BixHeaderTool] ✅ Generated " << outputPath << "\n";
        else
            std::cout << "[BixHeaderTool] ⏩ No changes for " << outputPath << "\n";

        return 0;
    }

    // ─────────────────────────────────────────────
    // 🎯 Mode 2 : Génération complète (multi dossiers)
    // ─────────────────────────────────────────────
    if (argc < 4)
    {
        std::cerr << "[BixHeaderTool] Usage:\n";
        std::cerr << "  BixHeaderTool <include_root> <samples_root> [extra_roots...] <output_dir>\n";
        std::cerr << "  or BixHeaderTool --single <header_path>\n";
        return 1;
    }

    // Tous les arguments sauf le dernier sont des dossiers sources
    const int outputIndex = argc - 1;
    const fs::path generatedOutputDir = fs::weakly_canonical(argv[outputIndex]);

    std::vector<fs::path> roots;
    for (int i = 1; i < outputIndex; ++i)
    {
        fs::path root = fs::weakly_canonical(argv[i]);
        if (fs::exists(root))
        {
            roots.push_back(root);
            std::cout << "[BixHeaderTool] + Include root: " << root << "\n";
        }
        else
        {
            std::cout << "[BixHeaderTool] ⚠️ Skipped missing root: " << root << "\n";
        }
    }

    if (roots.empty())
    {
        std::cerr << "[BixHeaderTool] ❌ No valid header roots to process.\n";
        return 2;
    }

    std::cout << "[BixHeaderTool] → Output directory: " << generatedOutputDir << "\n";

    // 🔄 Prépare le dossier de sortie
    std::error_code ec;
    fs::remove_all(generatedOutputDir, ec);
    fs::create_directories(generatedOutputDir, ec);

    // 🔍 Collecte les headers
    std::vector<fs::path> headers = CollectHeaderFiles(roots);
    if (headers.empty())
    {
        std::cout << "[BixHeaderTool] No headers to process.\n";
        return 0;
    }

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
