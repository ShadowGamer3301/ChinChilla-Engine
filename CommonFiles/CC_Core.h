#pragma once

//Include headers from STD library
#include <vector>
#include <map>
#include <thread>
#include <future>
#include <algorithm>
#include <memory>
#include <exception>
#include <filesystem>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <source_location>

#if defined WIN32 || defined _WIN32
	#define PLAT_WIN32 //Use singular macro to define Win32 platform

	#ifdef _WINDLL
		#define CCAPI __declspec(dllexport)
	#else
		#define CCAPI __declspec(dllimport)
	#endif

	#define WIN32_LEAN_AND_MEAN
	#define GLFW_EXPOSE_NATIVE_WIN32

	#include <Windows.h>
	#include <windowsx.h>
	#include <wrl.h>

	#ifdef GAPI_DX
		//Include DirectX headers
		#include <d3d11.h>
		#include <dxgi1_6.h>
		#include <d3dcompiler.h>
		#include <DirectXMath.h>
	#else
		//Include vulkan headers
		#include <vulkan/vulkan.h>
	#endif

#endif

//Include GLFW
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

//Include Loguru
#include <loguru/loguru.hpp>

//Include GLM
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

//Include Assimp
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

//Include LodePNG
#include <lodepng.h>