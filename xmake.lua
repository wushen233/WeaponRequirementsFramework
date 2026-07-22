set_xmakever("3.0.0")

set_project("WeaponRequirementsFramework")
set_version("1.2.0")
set_languages("c++23")
set_warnings("allextra")
set_encodings("utf-8")

add_rules("mode.debug", "mode.releasedbg")
set_policy("package.requires_lock", true)

local commonlibf4_path = os.getenv("COMMONLIBF4_PATH")
if not commonlibf4_path or not os.isdir(commonlibf4_path) then
    raise("COMMONLIBF4_PATH must point to a CommonLibF4 checkout")
end

includes(commonlibf4_path)

add_requires("simpleini v4.25")
add_requires("nlohmann_json v3.12.0")

target("WeaponRequirementsFramework")
    set_version("1.2.0")
    add_rules("commonlibf4.plugin", {
        name = "WeaponRequirementsFramework",
        author = "h_wushen",
        description = "Configurable weapon strength and skill requirements for Fallout 4",
        version = "1.2.0"
    })

    add_packages("simpleini", "nlohmann_json")
    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")
    set_encodings("utf-8")
    add_cxxflags("/utf-8", { force = true, tools = { "msvc", "clang-cl" } })

    add_defines(
        'PLUGIN_NAME="WeaponRequirementsFramework"',
        "PLUGIN_VERSION_MAJOR=1",
        "PLUGIN_VERSION_MINOR=2",
        "PLUGIN_VERSION_PATCH=0"
    )
