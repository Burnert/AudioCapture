workspace "AudioCapture"
	startproject "AudioCapture"
	
	configurations {
		"Debug",
		"Release"
	}
	platforms {
		"x86",
		"x64"
	}
	
	filter "platforms:*64"
		architecture "x64"
		
	filter "platforms:*86"
		architecture "x86"

outputdir = "%{cfg.system}-%{cfg.architecture}-%{cfg.buildcfg}"

project "AudioCapture"
	location "AudioCapture"
	kind "ConsoleApp"
	language "C++"
	characterset "MBCS"
	cppdialect "C++17"

	targetdir ("Build/" .. outputdir .. "/%{prj.name}")
	objdir ("Intermediate/" .. outputdir .. "/%{prj.name}")

	files {
		"%{prj.name}/Source/**.h",
		"%{prj.name}/Source/**.hpp",
		"%{prj.name}/Source/**.cpp",
	}

	includedirs {
		"%{prj.name}/Source",
		"AudioCaptureCore/Include"
	}
	
	dependson {
		"AudioCaptureModule"
	}
	
	linkoptions {
		"/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\""
	}

	filter "system:windows"
		staticruntime "On"
		systemversion "latest"

	filter "configurations:Debug"
		symbols "On"
		debugargs { "-im vlc.exe" }

	filter "configurations:Release"
		optimize "On"
		
	filter "platforms:x64"
		defines { "PLATFORM64" }
		
	filter "platforms:x86"
		defines { "PLATFORM32" }
		
project "AudioCaptureModule"
	location "AudioCaptureModule"
	kind "SharedLib"
	language "C++"
	characterset "MBCS"
	cppdialect "C++17"
	
	targetdir ("Build/" .. outputdir .. "/%{prj.name}")
	objdir ("Intermediate/" .. outputdir .. "/%{prj.name}")

	files {
		"%{prj.name}/Source/**.h",
		"%{prj.name}/Source/**.hpp",
		"%{prj.name}/Source/**.cpp",
		"%{prj.name}/Source/**.asm",
	}

	includedirs {
		"%{prj.name}/Source",
		"AudioCaptureCore/Include"
	}
	
	postbuildcommands {
		"{MKDIR} ../Build/" .. outputdir .. "/AudioCapture",
		"{COPY} %{cfg.buildtarget.relpath} ../AudioCapture"
	}

	filter "system:windows"
		staticruntime "Off"
		systemversion "latest"

	filter "configurations:Debug"
		symbols "On"

	filter "configurations:Release"
		optimize "On"
		
	filter "platforms:x64"
		defines { "PLATFORM64" }
		
	filter "platforms:x86"
		defines { "PLATFORM32" }
		
project "AudioCaptureCore"
	location "AudioCaptureCore"
	kind "None"
	
	-- targetdir ("Build/" .. outputdir .. "/%{prj.name}")
	-- objdir ("Intermediate/" .. outputdir .. "/%{prj.name}")

	files {
		"%{prj.name}/Include/**.hpp"
	}

	includedirs {
		"%{prj.name}/Include"
	}

	-- filter "system:windows"
		-- staticruntime "Off"
		-- systemversion "latest"

	-- filter "configurations:Debug"
		-- symbols "On"

	-- filter "configurations:Release"
		-- optimize "On"
		
	-- filter "platforms:x64"
		-- defines { "PLATFORM64" }
		
	-- filter "platforms:x86"
		-- defines { "PLATFORM32" }