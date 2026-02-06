#pragma once

#if defined(_WIN32)

#include <ShlObj_core.h>
#include <string>
#include <vector>
#include <wchar.h>
#include <windows.h>

namespace WinDND
{
    // ------------------------------------------------------------
    // IDropSource
    // ------------------------------------------------------------
    class DropSource : public IDropSource
    {
    public:
        ULONG __stdcall AddRef() override { return 1; }
        ULONG __stdcall Release() override { return 1; }

        HRESULT __stdcall QueryInterface(REFIID riid, void** ppv) override
        {
            if (riid == IID_IUnknown || riid == IID_IDropSource)
            {
                *ppv = this;
                return S_OK;
            }
            *ppv = nullptr;
            return E_NOINTERFACE;
        }

        HRESULT __stdcall QueryContinueDrag(BOOL esc, DWORD keyState) override
        {
            if (esc) return DRAGDROP_S_CANCEL;
            if (!(keyState & MK_LBUTTON)) return DRAGDROP_S_DROP;
            return S_OK;
        }

        HRESULT __stdcall GiveFeedback(DWORD) override
        {
            return DRAGDROP_S_USEDEFAULTCURSORS;
        }
    };

    // ------------------------------------------------------------
    // IDataObject（CF_HDROP 専用）
    // ------------------------------------------------------------
    class FileDataObject : public IDataObject
    {
    public:
        explicit FileDataObject(const std::wstring& path)
            : refCount(1)
        {
            files.push_back(path);
        }

        // IUnknown
        HRESULT __stdcall QueryInterface(REFIID riid, void** ppv)
        {
            if (riid == IID_IUnknown || riid == IID_IDataObject)
            {
                *ppv = static_cast<IDataObject*>(this);
                AddRef();
                return S_OK;
            }
            *ppv = nullptr;
            return E_NOINTERFACE;
        }

        ULONG __stdcall AddRef() { return ++refCount; }
        ULONG __stdcall Release()
        {
            ULONG r = --refCount;
            if (r == 0) delete this;
            return r;
        }

        // IDataObject
        HRESULT __stdcall GetData(FORMATETC* fmt, STGMEDIUM* med)
        {
            if (fmt->cfFormat != CF_HDROP) return DV_E_FORMATETC;

            const std::wstring& path = files[0];

            SIZE_T size =
                sizeof(DROPFILES) +
                (path.size() + 2) * sizeof(wchar_t);

            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, size);
            if (!hMem) return E_OUTOFMEMORY;

            auto* df = static_cast<DROPFILES*>(GlobalLock(hMem));
            if (!df)
            {
                GlobalFree(hMem);
                return E_OUTOFMEMORY;
            }

            df->pFiles = sizeof(DROPFILES);
            df->fWide = TRUE;

            wchar_t* buf =
                reinterpret_cast<wchar_t*>(
                    reinterpret_cast<BYTE*>(df) + sizeof(DROPFILES));

            wcscpy_s(buf, path.size() + 1, path.c_str());
            buf[path.size() + 1] = L'\0';

            GlobalUnlock(hMem);

            med->tymed = TYMED_HGLOBAL;
            med->hGlobal = hMem;
            med->pUnkForRelease = nullptr;

            return S_OK;
        }

        HRESULT __stdcall QueryGetData(FORMATETC* fmt)
        {
            return (fmt->cfFormat == CF_HDROP)
                ? S_OK
                : DV_E_FORMATETC;
        }

        HRESULT __stdcall GetCanonicalFormatEtc(
            FORMATETC*,
            FORMATETC*
        )
        {
            return E_NOTIMPL;
        }

        HRESULT __stdcall EnumFormatEtc(
            DWORD dwDirection,
            IEnumFORMATETC** ppenumFormatEtc) override
        {
            if (dwDirection != DATADIR_GET) return E_NOTIMPL;

            FORMATETC fmt = {};
            fmt.cfFormat = CF_HDROP;
            fmt.dwAspect = DVASPECT_CONTENT;
            fmt.lindex = -1;
            fmt.tymed = TYMED_HGLOBAL;

            return SHCreateStdEnumFmtEtc(1, &fmt, ppenumFormatEtc);
        }

        HRESULT __stdcall GetDataHere(FORMATETC*, STGMEDIUM*) { return E_NOTIMPL; }
        HRESULT __stdcall SetData(FORMATETC*, STGMEDIUM*, BOOL) { return E_NOTIMPL; }
        HRESULT __stdcall DAdvise(FORMATETC*, DWORD, IAdviseSink*, DWORD*) { return OLE_E_ADVISENOTSUPPORTED; }
        HRESULT __stdcall DUnadvise(DWORD) { return OLE_E_ADVISENOTSUPPORTED; }
        HRESULT __stdcall EnumDAdvise(IEnumSTATDATA**) { return OLE_E_ADVISENOTSUPPORTED; }

    private:
        ULONG refCount;
        std::vector<std::wstring> files;
    };

    // ------------------------------------------------------------
    // ファイル D&D 実行関数
    // ------------------------------------------------------------
    inline void startFileDrag(const std::wstring& path)
    {
        auto* data = new FileDataObject(path);
        DropSource source;

        DWORD effect = DROPEFFECT_COPY;
        DoDragDrop(data, &source, effect, &effect);

        data->Release();
    }
}

#endif
