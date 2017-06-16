#pragma once

#include <Box2D/Box2D.h>
#include <Box2D/Common/b2Draw.h>
#include <Box2D/Common/b2Math.h>

#include <GL/gl3w.h>
#include <ImGui/imgui.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#undef min
#undef max

template< class T > void SafeDelete(T*& pVal)
{
	if (pVal != nullptr)
	{
		delete pVal;
		pVal = nullptr;
	}
}
