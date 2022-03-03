#pragma once
#include "d3d9Wrapper.h"
#include "HookedStateBlock.h"

inline void D3D9Wrapper::IDirect3DStateBlock9::Delete()
{
    LOG_INFO("IDirect3DStateBlock9::Delete\n");
    LOG_INFO("  deleting self\n");
    if (mDirectModeStateBlockDuplication)
        mDirectModeStateBlockDuplication->Release();
    if (m_pRealUnk) m_List.DeleteMember(m_pRealUnk);
    m_pUnk = 0;
    m_pRealUnk = 0;
    delete this;
}

void D3D9Wrapper::IDirect3DStateBlock9::HookStateBlock()
{
    m_pUnk = hook_stateblock(GetD3DStateBlock9(), (::IDirect3DStateBlock9*)this);
}

inline D3D9Wrapper::IDirect3DStateBlock9::IDirect3DStateBlock9(::LPDIRECT3DSTATEBLOCK9 pStateBlock, D3D9Wrapper::IDirect3DDevice9 *hackerDevice)
    : D3D9Wrapper::IDirect3DUnknown((IUnknown*)pStateBlock),
    mDirectModeStateBlockDuplication(NULL),
    hackerDevice(hackerDevice)
{
    if ((G->enable_hooks >= EnableHooksDX9::ALL) && pStateBlock) {
        this->HookStateBlock();
    }
}

inline D3D9Wrapper::IDirect3DStateBlock9 * D3D9Wrapper::IDirect3DStateBlock9::GetDirect3DStateBlock9(::LPDIRECT3DSTATEBLOCK9 pStateBlock, D3D9Wrapper::IDirect3DDevice9 *hackerDevice)
{
    D3D9Wrapper::IDirect3DStateBlock9* p = new D3D9Wrapper::IDirect3DStateBlock9(pStateBlock, hackerDevice);
    if (pStateBlock) m_List.AddMember(pStateBlock, p);
    return p;
}

STDMETHODIMP D3D9Wrapper::IDirect3DStateBlock9::QueryInterface(THIS_ REFIID riid, void ** ppvObj)
{
    LOG_DEBUG("D3D9Wrapper::IDirect3DStateBlock9::QueryInterface called\n");// at 'this': %s\n", type_name_dx9((IUnknown*)this));
    HRESULT hr = NULL;
    if (QueryInterface_DXGI_Callback(riid, ppvObj, &hr))
        return hr;
    LOG_INFO("QueryInterface request for %s on %p\n", name_from_IID(riid), this);
    hr = m_pUnk->QueryInterface(riid, ppvObj);
    if (hr == S_OK) {
        if ((*ppvObj) == GetRealOrig()) {
            if (!(G->enable_hooks >= EnableHooksDX9::ALL)) {
                *ppvObj = this;
                ++m_ulRef;
                LOG_INFO("  interface replaced with IDirect3DStateBlock9 wrapper.\n");
                LOG_INFO("  result = %x, handle = %p\n", hr, *ppvObj);
                return hr;
            }
        }
        D3D9Wrapper::IDirect3DUnknown *unk = QueryInterface_Find_Wrapper(*ppvObj);
        if (unk)
            *ppvObj = unk;
    }
    LOG_INFO("  result = %x, handle = %p\n", hr, *ppvObj);
    return hr;
}

inline STDMETHODIMP_(ULONG __stdcall) D3D9Wrapper::IDirect3DStateBlock9::AddRef(void)
{
    ++m_ulRef;
    return m_pUnk->AddRef();
}

inline STDMETHODIMP_(ULONG __stdcall) D3D9Wrapper::IDirect3DStateBlock9::Release(void)
{
    LOG_DEBUG("IDirect3DStateBlock9::Release handle=%p, counter=%d, this=%p\n", m_pUnk, m_ulRef, this);

    ULONG ulRef = m_pUnk ? m_pUnk->Release() : 0;
    LOG_DEBUG("  internal counter = %d\n", ulRef);

    --m_ulRef;

    if (ulRef == 0)
    {
        if (!gLogDebug) LOG_INFO("IDirect3DStateBlock9::Release handle=%p, counter=%d, internal counter = %d\n", m_pUnk, m_ulRef, ulRef);

        Delete();
    }
    return ulRef;
}

inline STDMETHODIMP_(HRESULT __stdcall) D3D9Wrapper::IDirect3DStateBlock9::Apply()
{
    LOG_DEBUG("IDirect3DStateBlock9::Apply called\n");
    return GetD3DStateBlock9()->Apply();
}

inline STDMETHODIMP_(HRESULT __stdcall) D3D9Wrapper::IDirect3DStateBlock9::Capture()
{
    LOG_DEBUG("IDirect3DStateBlock9::Capture called\n");
    return GetD3DStateBlock9()->Capture();
}

inline STDMETHODIMP_(HRESULT __stdcall) D3D9Wrapper::IDirect3DStateBlock9::GetDevice(D3D9Wrapper::IDirect3DDevice9 ** ppDevice)
{
    LOG_DEBUG("IDirect3DStateBlock9::GetDevice called\n");
    if (hackerDevice) {
        hackerDevice->GetD3D9Device()->AddRef();
    }
    else {
        return D3DERR_NOTFOUND;
    }
    if (!(G->enable_hooks & EnableHooksDX9::DEVICE)) {
        *ppDevice = hackerDevice;
    }
    else {
        *ppDevice = reinterpret_cast<D3D9Wrapper::IDirect3DDevice9*>(hackerDevice->GetD3D9Device());
    }
    return D3D_OK;
}