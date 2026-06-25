workspace "Deckadence"
    configurations {"Debug", "Release"}
    if (_ACTION or ""):find("vs") then
        platforms {"Win64"}
    end

project "Deckadence"
    kind "WindowedApp"
    language "C++"

    files {"src/**.cpp", "include/**.hpp", "external/NativeFileDialogs-Extended/nfd_win.cpp", "external/ImGui/*", "external/Tetrapak0/*", "external/stb/*"}
    includedirs {"include/", "external/"}
    filter {"system:windows"}
        includedirs {"external/freetype/"}
        flags {"multiprocessorcompile"}
        files {"resources/Deckadence.rc", "resources/**.ico", "resources/**.c"}
        vpaths {["resources"] = {"resources/**.rc", "resources/**.ico"}}
    filter {}
    cppdialect "c++20"
    cdialect "c17"
    if (_ACTION or ""):find("vs") then
        links {"opengl32", "ws2_32", "iphlpapi", "dwmapi"}
        libdirs {"external/**/lib"}
        links {"freetype", "glfw3"}
    else
        links {"glfw", "GL", "freetype"}
        libdirs {os.findlib("dbus-1")}
        links {"dbus-1"}
    end

    filter "configurations:Debug"
        defines {"_DEBUG"}
        symbols "On"

    filter "configurations:Release"
        optimize "On"
