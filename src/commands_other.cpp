#include "vcpkg_Commands.h"
#include <iostream>
#include "vcpkg_Environment.h"
#include "vcpkg.h"
#include "vcpkg_System.h"
#include "vcpkg_Files.h"

namespace vcpkg
{
    void print_usage()
    {
        std::cout << "Commands:\n"
            "  vcpkg search [pat]              Search for packages available to be built\n"
            "  vcpkg install <pkg>             Install a package\n"
            "  vcpkg remove <pkg>              Uninstall a package. \n"
            "  vcpkg remove --purge <pkg>      Uninstall and delete a package. \n"
            "  vcpkg list                      List installed packages\n"
            "  vcpkg update                    Display list of packages for updating\n"
            "\n"
            << INTEGRATE_COMMAND_HELPSTRING <<
            "\n"
            "  vcpkg edit <pkg>                Open up a port for editing (uses %EDITOR%, default 'code')\n"
            "  vcpkg import <pkg>              Import a pre-built library\n"
            "  vcpkg create <pkg> <url>\n"
            "             [archivename]        Create a new package\n"
            "  vcpkg owns <pat>                Search for files in installed packages\n"
            "  vcpkg cache                     List cached compiled packages\n"
            "  vcpkg version                   Display version information\n"
            "  vcpkg contact                   Display contact information to send feedback\n"
            "\n"
            //"internal commands:\n"
            //"  --check-build-deps <controlfile>\n"
            //"  --create-binary-control <controlfile>\n"
            //"\n"
            "Options:\n"
            "  --triplet <t>                   Specify the target architecture triplet.\n"
            "                                  (default: x86-windows, see 'vcpkg help triplet')\n"
            "\n"
            "  --vcpkg-root <path>             Specify the vcpkg root directory\n"
            "                                  (default: %VCPKG_ROOT%)\n"
            "\n"
            "For more help (including examples) see the accompanying README.md."
            "\n";
    }

    void print_example(const char* command_and_arguments)
    {
        std::cout <<
            "Example:\n"
            "  vcpkg " << command_and_arguments << "\n";
    }

    void edit_command(const vcpkg_cmd_arguments& args, const vcpkg_paths& paths, const triplet& default_target_triplet)
    {
        static auto example = "edit zlib";
        args.check_max_args(1, example);
        package_spec spec = args.parse_all_arguments_as_package_specs(default_target_triplet, example).at(0);

        // Find editor
        std::wstring env_EDITOR = System::wdupenv_str(L"EDITOR");
        if (env_EDITOR.empty())
            env_EDITOR = LR"(C:\Program Files (x86)\Microsoft VS Code\Code.exe)";

        auto portpath = paths.ports / spec.name;
        std::wstring cmdLine = Strings::format(LR"("%s" "%s" "%s")", env_EDITOR, portpath.native(), (portpath / "portfile.cmake").native());
        exit(System::cmd_execute(cmdLine));
    }

    void create_command(const vcpkg_cmd_arguments& args, const vcpkg_paths& paths, const triplet& default_target_triplet)
    {
        args.check_max_args(3);
        package_spec spec = args.parse_all_arguments_as_package_specs(default_target_triplet).at(0);
        if (args.command_arguments.size() < 2)
        {
            System::println(System::color::error, "Error: create requires the archive's URL as the second argument.");
            print_usage();
            exit(EXIT_FAILURE);
        }
        Environment::ensure_utilities_on_path(paths);

        // Space OR define the FILENAME with proper spacing
        std::wstring custom_filename = L" ";
        if (args.command_arguments.size() >= 3)
        {
            custom_filename = Strings::format(L" -DFILENAME=%s ", Strings::utf8_to_utf16(args.command_arguments.at(2)));
        }

        const std::wstring cmdline = Strings::format(LR"(cmake -DCMD=SCAFFOLD -DPORT=%s -DTARGET_TRIPLET=%s -DURL=%s%s-P "%s")",
                                                     Strings::utf8_to_utf16(spec.name),
                                                     Strings::utf8_to_utf16(spec.target_triplet.value),
                                                     Strings::utf8_to_utf16(args.command_arguments.at(1)),
                                                     custom_filename,
                                                     paths.ports_cmake.generic_wstring());

        exit(System::cmd_execute(cmdline));
    }

    void import_command(const vcpkg_cmd_arguments& args, const vcpkg_paths& paths)
    {
        if (args.command_arguments.size() != 3)
        {
            System::println(System::color::error, "Error: %s requires 3 parameters", args.command);
            print_example(Strings::format(R"(%s C:\path\to\CONTROLfile C:\path\to\includedir C:\path\to\projectdir)", args.command).c_str());
            exit(EXIT_FAILURE);
        }

        const fs::path control_file_path(args.command_arguments[0]);
        const fs::path include_directory(args.command_arguments[1]);
        const fs::path project_directory(args.command_arguments[2]);

        auto pghs = get_paragraphs(control_file_path);
        Checks::check_throw(pghs.size() == 1, "Invalid control file for package");

        StatusParagraph spgh;
        spgh.package = BinaryParagraph(pghs[0]);
        auto& control_file_data = spgh.package;

        vcpkg::binary_import(paths, include_directory, project_directory, control_file_data);
        exit(EXIT_SUCCESS);
    }

    void owns_command(const vcpkg_cmd_arguments& args, const vcpkg_paths& paths)
    {
        args.check_max_args(1);
        if (args.command_arguments.size() == 0)
        {
            System::println(System::color::error, "Error: owns requires a pattern to search for as the first argument.");
            std::cout <<
                "example:\n"
                "    vcpkg owns .dll\n";
            exit(EXIT_FAILURE);
        }
        StatusParagraphs status_db = database_load_check(paths);
        search_file(paths, args.command_arguments[0], status_db);
        exit(EXIT_SUCCESS);
    }

    void internal_test_command(const vcpkg_cmd_arguments& /*args*/, const vcpkg_paths& /*paths*/)
    {
        //        auto data = FormatEventData("test");
        //        Track(data);
        exit(EXIT_SUCCESS);
    }

    const std::vector<package_name_and_function<command_type_a>>& get_available_commands_type_a()
    {
        static std::vector<package_name_and_function<command_type_a>> t = {
            {"install", install_command},
            {"remove", remove_command},
            {"build", build_command},
            {"edit", edit_command},
            {"create", create_command},
            {"build_external", build_external_command}
        };
        return t;
    }

    const std::vector<package_name_and_function<command_type_b>>& get_available_commands_type_b()
    {
        static std::vector<package_name_and_function<command_type_b>> t = {
            {"help", help_command},
            {"search", search_command},
            {"list", list_command},
            {"integrate", integrate_command},
            {"owns", owns_command},
            {"update", update_command},
            {"import", import_command},
            {"cache", cache_command},
            {"internal_test", internal_test_command},
        };
        return t;
    }

    const std::vector<package_name_and_function<command_type_c>>& get_available_commands_type_c()
    {
        static std::vector<package_name_and_function<command_type_c>> t = {
            {"version", &version_command},
            {"contact", &contact_command}
        };
        return t;
    }
}
