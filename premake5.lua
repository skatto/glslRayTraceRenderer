
sources = {
  "./src/**.h", "./src/**.cpp",
  "./gl_src/**.h", "./gl_src/**.cpp"
}

exec_source = {
  "./src/main.cpp",
  "./src/test.cpp"
}

workspace "GlslRayTracerWorkspace"
	configurations { "debug", "release" }
	language "C++"
	includedirs { "/usr/local/include" }
	libdirs { "/usr/local/lib" }

	basedir "build"

	toolset "clang"

	buildoptions { "-std=c++14" }

	configuration { "macosx", "gmake" }
		links { "glfw", "glew" }
		linkoptions { "-framework OpenGL" }

	configuration { "linux", "gmake" }
		links { "glfw", "GLEW", "GL" }
		defines { "LINUX" }

	configuration "debug" 
		defines { "DEBUG" }
		buildoptions { "-Wextra", "-Wall",
					   "-Wno-c++98-compat-pedantic", "-Wno-padded",
					   "-Wno-sign-conversion", "-Wno-undef" }

	configuration "Release"
		defines { "NDEBUG" } 
		optimize "Full"

project "GlslRender"
	kind "ConsoleApp"
	files { sources }
	removefiles { exec_source }
	files { "./src/main.cpp" }
