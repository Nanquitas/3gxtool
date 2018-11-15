#include "types.hpp"
#include "3gx.hpp"
#include "yaml.h"
#include "cxxopts.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>

#define MAKE_VERSION(major, minor, revision) (((major)<<24)|((minor)<<16)|((revision)<<8))

static bool                 g_silentMode = false;
static std::string          g_author;
static std::string          g_title;
static std::string          g_summary;
static std::string          g_description;
static std::vector<u32>     g_targets;

void    CheckOptions(int& argc, const char **argv)
{
    cxxopts::Options options(argv[0], "");

    options.add_options()
        ("s,silent", "Don't display the text (except errors)")
        ("h,help", "Print help");

    auto result = options.parse(argc, argv);

    if (result.count("help"))
    {
      std::cout <<  " - Builds plugin files to be used by Luma3DS\n" \
                    "Usage:\n"
                <<  argv[0] << " [OPTION...] <input.bin> <settings.plgInfo> <output.3dsgx>" << std::endl;
      exit(0);
    }

    g_silentMode = result.count("silent");
}

u32     GetVersion(YAML::Node& settings)
{
    u32 major = 0, minor = 0, revision = 0;

    if (settings["Version"])
    {
        auto map = settings["Version"];

        if (map["Major"]) major = map["Major"].as<u32>();
        if (map["Minor"]) minor = map["Minor"].as<u32>();
        if (map["Revision"]) revision = map["Revision"].as<u32>();
    }

    return MAKE_VERSION(major, minor, revision);
}

void    GetInfos(YAML::Node& settings)
{
    if (settings["Author"])
        g_author = settings["Author"].as<std::string>();

    if (settings["Title"])
        g_title = settings["Title"].as<std::string>();

    if (settings["Summary"])
        g_summary = settings["Summary"].as<std::string>();

    if (settings["Description"])
        g_description = settings["Description"].as<std::string>();
}

void    GetTitles(YAML::Node& settings)
{
    if (settings["Targets"])
    {
        auto list =  settings["Targets"];

        if (list.size() >= 1 && list[0].as<u32>() != 0)
        {
            for (u32 i = 0; i < list.size(); ++i)
                g_targets.push_back(list[i].as<u32>());
        }
    }
}

int     main(int argc, const char **argv)
{
    try
    {
        CheckOptions(argc, argv);

        if (!g_silentMode)
            std::cout   <<  "\n" \
                            "3DS Game eXtension Tool\n" \
                            "--------------------------\n\n";

        if (argc < 4)
        {
            if (!g_silentMode)
                std::cout   <<  " - Builds plugin files to be used by Luma3DS\n" \
                                "Usage:\n"
                            <<  argv[0] << " [OPTION...] <input.bin> <settings.plgInfo> <output.3gx>" << std::endl;
            return -1;
        }

        char *          code;
        _3gx_Header     header;
        YAML::Node      settings;
        std::ifstream   settingsFile;
        std::ifstream   codeFile;
        std::ofstream   outputFile;

        // Open files
        codeFile.open(argv[1], std::ios::in | std::ios::binary);
        settingsFile.open(argv[2], std::ios::in);
        outputFile.open(argv[3], std::ios::out | std::ios::trunc | std::ios::binary);

        if (!codeFile.is_open())
        {
            std::cerr << "couldn't open: " << argv[1] << std::endl;
            return -1;
        }

        if (!settingsFile.is_open())
        {
            std::cerr << "couldn't open: " << argv[2] << std::endl;
            return -1;
        }

        if (!outputFile.is_open())
        {
            std::cerr << "couldn't open: " << argv[3] << std::endl;
            return -1;
        }

        // Get code size
        codeFile.seekg(0, std::ios::end);
        header.codeSize = (u32)codeFile.tellg();
        codeFile.seekg(0, std::ios::beg);

        // Read code into buffer
        code = new char[header.codeSize]();
        codeFile.read(code, header.codeSize);
        codeFile.close();

        if (!g_silentMode)
            std::cout << "Processing settings..." << std::endl;

        // Parse yaml
        settings = YAML::Load(settingsFile);

        // Fetch version
        header.version = GetVersion(settings);

        // Fetch Infos
        GetInfos(settings);

        // Fetch titles
        GetTitles(settings);

        if (!g_silentMode)
            std::cout << "Creating file..." << std::endl;

        // Reserve header place
        outputFile.write((const char *)&header, sizeof(_3gx_Header));

        if (!g_title.empty())
        {
            header.infos.titleLen = g_title.size() + 1;
            header.infos.titleMsg = (u32)outputFile.tellp();
            outputFile << g_title << '\0';
            outputFile.flush();
        }

        if (!g_author.empty())
        {
            header.infos.authorLen = g_author.size() + 1;
            header.infos.authorMsg = (u32)outputFile.tellp();
            outputFile << g_author << '\0';
            outputFile.flush();
        }

        if (!g_summary.empty())
        {
            header.infos.summaryLen = g_summary.size() + 1;
            header.infos.summaryMsg = (u32)outputFile.tellp();
            outputFile << g_summary << '\0';
            outputFile.flush();
        }

        if (!g_description.empty())
        {
            header.infos.descriptionLen = g_description.size() + 1;
            header.infos.descriptionMsg = (u32)outputFile.tellp();
            outputFile << g_description << '\0';
            outputFile.flush();
        }

        if (!g_targets.empty())
        {
            header.targets.count = g_targets.size();
            header.targets.titles = (u32)outputFile.tellp();
            outputFile.write((const char *)g_targets.data(), 4 * g_targets.size());
            outputFile.flush();
        }

        header.code = (u32)outputFile.tellp();
        u32 padding = 8 - (header.code & 7);
        char zeroes[8] = {0};
        outputFile.write(zeroes, padding);
        header.code += padding;
        outputFile.write(code, header.codeSize);
        outputFile.flush();
        delete[] code;

        // Write updated header to file
        outputFile.seekp(0, std::ios::beg);
        outputFile.write((const char *)&header, sizeof(_3gx_Header));
        outputFile.flush();
        codeFile.close();
        outputFile.close();

        if (!g_silentMode)
            std::cout << "Done" << std::endl;
    }
    catch (std::exception& e)
    {
        std::cerr << "An exception occured: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}
