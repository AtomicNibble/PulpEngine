#include "stdafx.h"
#include "Game.h"
#include "EngineApp.h"

#include <IFileSys.h>

#include <String\Path.h>
#include <Platform\MessageBox.h>

#include <tchar.h>

#define _LAUNCHER

// #undef X_LIB
#include <ModuleExports.h>

#ifdef X_LIB
struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_ENGINE_LIB("Input")
X_LINK_ENGINE_LIB("Font")
X_LINK_ENGINE_LIB("Core")
X_LINK_ENGINE_LIB("Script")
X_LINK_ENGINE_LIB("Sound")
X_LINK_ENGINE_LIB("RenderDx12")
X_LINK_ENGINE_LIB("3DEngine")
X_LINK_ENGINE_LIB("Physics")
X_LINK_ENGINE_LIB("GameDLL")
X_LINK_ENGINE_LIB("Network")
X_LINK_ENGINE_LIB("Video")

X_FORCE_LINK_FACTORY("XEngineModule_Input")
X_FORCE_LINK_FACTORY("XEngineModule_Font")
X_FORCE_LINK_FACTORY("XEngineModule_Script")
X_FORCE_LINK_FACTORY("XEngineModule_Sound")
X_FORCE_LINK_FACTORY("XEngineModule_3DEngine")
X_FORCE_LINK_FACTORY("XEngineModule_Physics")
X_FORCE_LINK_FACTORY("XEngineModule_Game")
X_FORCE_LINK_FACTORY("XEngineModule_Network")
X_FORCE_LINK_FACTORY("XEngineModule_Video")

X_FORCE_SYMBOL_LINK("?s_factory@XEngineModule_Render@render@Potato@@0V?$XSingletonFactory@VXEngineModule_Render@render@Potato@@@@A");
// X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Render@render@Potato@@0V1234@A");

// some libs that w link against.
X_LINK_ENGINE_LIB("ImgLib")
X_LINK_ENGINE_LIB("ShaderLib")
X_LINK_ENGINE_LIB("ModelLib")
X_LINK_ENGINE_LIB("AnimLib")
X_LINK_ENGINE_LIB("FontLib")

X_FORCE_LINK_FACTORY("XConverterLib_Img")
X_FORCE_LINK_FACTORY("XConverterLib_Shader")
X_FORCE_LINK_FACTORY("XConverterLib_Model")
X_FORCE_LINK_FACTORY("XConverterLib_Anim")
X_FORCE_LINK_FACTORY("XConverterLib_Font")

#endif // !X_LIB

void InitRootDir(void)
{
#ifdef WIN32
    WCHAR szExeFileName[_MAX_PATH] = {0};
    GetModuleFileNameW(GetModuleHandleW(NULL), szExeFileName, sizeof(szExeFileName));

    core::Path<wchar_t> path(szExeFileName);

    path.removeFileName();
    path.removeTrailingSlash();

    if (!SetCurrentDirectoryW(path.c_str())) {
        core::msgbox::show("Failed to set current directory",
            X_ENGINE_NAME " Fatal Error",
            core::msgbox::Style::Error | core::msgbox::Style::Topmost | core::msgbox::Style::DefaultDesktop,
            core::msgbox::Buttons::OK);

        ExitProcess(static_cast<uint32_t>(-1));
    }
#endif
}

namespace
{
    core::MallocFreeAllocator gAlloc;

} // namespace

void* operator new(size_t sz) throw()
{
    return gAlloc.allocate(sz, sizeof(uintptr_t), 0);
}

void* operator new[](size_t sz) throw()
{
    return gAlloc.allocate(sz, sizeof(uintptr_t), 0);
}

void operator delete(void* m) throw()
{
    if (m) {
        gAlloc.free(m);
    }
}

void operator delete[](void* m) throw()
{
    if (m) {
        gAlloc.free(m);
    }
}

void operator delete(void* m, size_t sz) throw()
{
    if (m) {
        gAlloc.free(m, sz);
    }
}

void operator delete[](void* m, size_t sz) throw()
{
    if (m) {
        gAlloc.free(m, sz);
    }
}

#include <Math\XMatrixAlgo.h>
#include <Math\QuatAlgo.h>

X_INLINE void _MatrixOrthoOffCenterRH(Matrix44f* pMat, float32_t left, float32_t right,
    float32_t bottom, float32_t top, float32_t zn, float32_t zf)
{
    core::zero_this(pMat);

    pMat->m00 = 2.0f / (right - left);
    pMat->m11 = 2.0f / (top - bottom);
    pMat->m22 = 1.0f / (zf - zn);
    pMat->m33 = 1.0f;
    
    pMat->m03 = (left + right) / (left - right);
    pMat->m13 = (top + bottom) / (bottom - top);
    pMat->m23 = zn / (zn - zf);
}

X_INLINE void _MatrixOrthoRH(Matrix44f* pMat, float32_t width, float32_t height,
    float32_t zn, float32_t zf)
{
    core::zero_this(pMat);

    pMat->m00 = 2.0f / width;
    pMat->m11 = 2.0f / height;
    pMat->m22 = 1.0f / (zf - zn);
    pMat->m33 = 1.0f;

    pMat->m23 = zn / (zn - zf);
}

void test()
{
    glm::mat3x3 test;

    glm::vec3 eye, center, up;
	eye = glm::vec3(-40, 65, 90); // 10, 5, 10);
	center = glm::vec3(-80,0, 59); //1, 2, 1);
    up = glm::vec3(0, 0, 1);

    auto lookAtMat = glm::lookAtRH(eye, center, up);

    Vec3f dir = Vec3f(10, 5, 10);
    Vec3f at = Vec3f(1, 2, 1);
    Matrix33f mat;
    MatrixLookAtRH(&mat, dir, at, Vec3f(0, 0, 1));

	MatrixLookAtRH(&mat, Vec3f(-40, 65, 90), Vec3f(-80, 0, 59), Vec3f::zAxis());


    X_UNUSED(mat, test);
}

void test2()
{
    const float FOV = toRadians(75.0f);
    const float NEAR = 0.25f;
    const float FAR = 1024.0f;

    auto glmProj = glm::perspectiveFovRH(DEFAULT_FOV, 1680.f, 1050.f, DEFAULT_NEAR, DEFAULT_FAR);

    // proj
    Matrix44f proj;
    MatrixPerspectiveFovRH(&proj, DEFAULT_FOV, 1680.f / 1050.f, DEFAULT_NEAR, DEFAULT_FAR, false);
}

void test3()
{
    const float FOV = toRadians(75.0f);
    const float NEAR = 0.25f;
    const float FAR = 1024.0f;

    auto glmProj = glm::perspectiveFovRH(DEFAULT_FOV, 1680.f, 1050.f, DEFAULT_NEAR, DEFAULT_FAR);

    // proj
    Matrix44f proj;
//    _MatrixOrthoOffCenterRH(&proj, DEFAULT_FOV, 1680.f / 1050.f, DEFAULT_NEAR, DEFAULT_FAR, false);
}



#include <glm\gtx\quaternion.hpp>
glm::quat quatLookAtRH(glm::vec3 const& direction, glm::vec3 const& up)
{
    glm::mat3 Result;

    Result[2] = -glm::normalize(direction);
    Result[0] = glm::normalize(glm::cross(up, Result[2]));
    Result[1] = glm::cross(Result[2], Result[0]);

    return glm::quat_cast(Result);
}


void test4()
{
    glm::quat;

    auto test = QuatLookAt(Vec3f(100.f, 20.f, 1.f), Vec3f::zAxis());

    auto direction = glm::vec3(100.f, 20.f, 1.f);
    auto up = glm::vec3(0, 0, 1);

    glm::mat3 Result;

    Result[2] = -normalize(direction);
    Result[0] = normalize(cross(up, Result[2]));
    Result[1] = cross(Result[2], Result[0]);

    auto goat = glm::quat_cast(Result);
}


int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPTSTR lpCmdLine, int nCmdShow)
{
    X_UNUSED(hPrevInstance);
    X_UNUSED(lpCmdLine);
    X_UNUSED(nCmdShow);
    InitRootDir();

    int nRes = 0;

    test();
    test2();
    test4();

    Matrix44f m0;
    m0.setToIdentity();
    m0.rotate(Vec3f(1, 0, 0), toRadians(32.758f));

    const Vec3f tv(1, 0, 0.5);
    auto goat0 = m0.postMultiply(tv);
    auto goat1 = m0.preMultiply(tv);

    auto goat4 = m0 * tv;


    Matrix44f m1(
        1.f, 2.f, 3.f, 4.f,
        5.f, 6.f, 7.f, 8.f,
        9.f, 10.f, 11.f, 12.f,
        13.f, 14.f, 15.f, 16.f
    );

    m1.setTranslate(Vec3f(777, 888,999));

    glm::mat4x4 mat;
    auto rotated = glm::rotate(mat, ::toRadians(32.758f), glm::vec3(1,0,0));

    auto vec = rotated * glm::vec4(1, 0, 0.5, 1);
    auto vec1 = glm::vec4(1,0,0.5,1) * rotated;

    auto goat = glm::translate(mat, glm::vec3(10, 50, 255));

    rotated.length();


    { // scope it for leak tests.
        EngineApp engine;

        if (engine.Init(hInstance, lpCmdLine)) {
            nRes = engine.MainLoop();
        }
    }

    return nRes;
}
