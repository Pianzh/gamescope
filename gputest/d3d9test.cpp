// D3D9 GPU test for validating gamescope + wine + DXVK on Anland containers.
// Spinning RGB triangle, prints FPS once a second, auto-exits after 30s.
#include <windows.h>
#include <d3d9.h>
#include <stdio.h>
#include <math.h>

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcA(h, m, w, l);
}

int main(int argc, char **argv) {
    DWORD sleepMs = 8;
    if (argc > 1) sleepMs = (DWORD)atoi(argv[1]);
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "d3d9gputest";
    RegisterClassA(&wc);
    HWND hwnd = CreateWindowA("d3d9gputest", "D3D9 GPU Test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              40, 40, 656, 519, NULL, NULL, wc.hInstance, NULL);

    LPDIRECT3D9 d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d) { printf("FAIL: Direct3DCreate9\n"); return 1; }

    D3DPRESENT_PARAMETERS pp = {0};
    pp.BackBufferWidth = 640;
    pp.BackBufferHeight = 480;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 1;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow = hwnd;
    pp.Windowed = TRUE;
    pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    LPDIRECT3DDEVICE9 dev = NULL;
    HRESULT hr = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
                                   D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &dev);
    if (FAILED(hr)) {
        hr = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
                               D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &dev);
        if (SUCCEEDED(hr)) printf("NOTE: fell back to software vertex processing\n");
    }
    if (FAILED(hr)) { printf("FAIL: CreateDevice 0x%08lx\n", (unsigned long)hr); return 1; }
    printf("OK: D3D9 HAL device created (frame pacing: %lu ms)\n", (unsigned long)sleepMs);

    struct V { float x, y, z, rhw; DWORD c; } tri[3];
    DWORD start = GetTickCount(), frames = 0, last = start;
    MSG msg;
    for (;;) {
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) goto done;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        DWORD now = GetTickCount();
        if (now - start > 30000) break;

        float t = (now % 4000) * 3.14159265f / 2000.0f;
        for (int i = 0; i < 3; i++) {
            float a = t + i * 2.0944f;
            tri[i].x = 320 + 180 * cosf(a);
            tri[i].y = 240 + 180 * sinf(a);
            tri[i].z = 0.5f;
            tri[i].rhw = 1.0f;
            tri[i].c = (i == 0) ? 0xffff0000 : (i == 1) ? 0xff00ff00 : 0xff0000ff;
        }

        dev->Clear(0, NULL, D3DCLEAR_TARGET, 0xff202040, 0.0f, 0);
        if (SUCCEEDED(dev->BeginScene())) {
            dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
            dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, tri, sizeof(struct V));
            dev->EndScene();
        }
        if (FAILED(dev->Present(NULL, NULL, NULL, NULL))) { printf("FAIL: Present\n"); break; }
        frames++;
        if (now - last >= 1000) {
            printf("FPS: %lu\n", (unsigned long)(frames * 1000 / (now - last)));
            frames = 0;
            last = now;
        }
        if (sleepMs)
            Sleep(sleepMs);
    }
done:
    printf("DONE: rendered %lu frames\n", (unsigned long)frames);
    return 0;
}
