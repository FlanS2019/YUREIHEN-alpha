// sound.cpp
#include "sound.h"
#include <vector>
#include <Windows.h>

// グローバル変数
static IXAudio2* g_pXAudio2 = nullptr;
static IXAudio2MasteringVoice* g_pMasterVoice = nullptr;

// SafeRelease
template<class T> void SafeRelease(T** pp) {
    if (*pp) {
        (*pp)->Release();
        *pp = nullptr;
    }
}

// 初期化
void InitSound() {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    MFStartup(MF_VERSION);
    XAudio2Create(&g_pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    g_pXAudio2->CreateMasteringVoice(&g_pMasterVoice);
}

// 終了
void UninitSound() {
    if (g_pMasterVoice) {
        g_pMasterVoice->DestroyVoice();
        g_pMasterVoice = nullptr;
    }
    SafeRelease(&g_pXAudio2);
    MFShutdown();
    CoUninitialize();
}

// MP3読み込み
SoundData* LoadMP3(const wchar_t* filename) {
    SoundData* data = new SoundData();

    // SourceReader作成
    HRESULT hr = MFCreateSourceReaderFromURL(filename, NULL, &data->pReader);
    if (FAILED(hr)) {
        delete data;
        return nullptr;
    }

    // オーディオストリームのみ選択
    data->pReader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE);
    data->pReader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);

    // PCM形式に設定
    IMFMediaType* pPartialType = nullptr;
    MFCreateMediaType(&pPartialType);
    pPartialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    pPartialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    data->pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, pPartialType);
    SafeRelease(&pPartialType);

    // WAVEFORMAT取得
    IMFMediaType* pType = nullptr;
    data->pReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pType);
    UINT32 wfxSize = 0;
    MFCreateWaveFormatExFromMFMediaType(pType, &data->pWfx, &wfxSize);
    SafeRelease(&pType);

    // 全サンプル読み込み
    std::vector<BYTE> audioData;
    while (true) {
        DWORD streamFlags = 0;
        IMFSample* pSample = nullptr;
        hr = data->pReader->ReadSample(
            (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
            0, NULL, &streamFlags, NULL, &pSample);

        if (FAILED(hr) || (streamFlags & MF_SOURCE_READERF_ENDOFSTREAM)) {
            if (pSample) pSample->Release();
            break;
        }

        if (pSample) {
            IMFMediaBuffer* pBuffer = nullptr;
            pSample->ConvertToContiguousBuffer(&pBuffer);

            BYTE* pAudioData = nullptr;
            DWORD cbBuffer = 0;
            pBuffer->Lock(&pAudioData, NULL, &cbBuffer);
            audioData.insert(audioData.end(), pAudioData, pAudioData + cbBuffer);
            pBuffer->Unlock();

            SafeRelease(&pBuffer);
            pSample->Release();
        }
    }

    // バッファコピー
    data->bufferSize = (UINT32)audioData.size();
    data->pBuffer = new BYTE[data->bufferSize];
    memcpy(data->pBuffer, audioData.data(), data->bufferSize);

    // SourceVoice作成
    g_pXAudio2->CreateSourceVoice(&data->pSourceVoice, data->pWfx);

    return data;
}

// MP3読み込み (string版)
SoundData* LoadMP3(const std::string& filename) {
    // stringからwstringへ変換
    int size = MultiByteToWideChar(CP_UTF8, 0, filename.c_str(), -1, NULL, 0);
    std::wstring wfilename(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, filename.c_str(), -1, &wfilename[0], size);
    return LoadMP3(wfilename.c_str());
}

// サウンド解放
void UnloadSound(SoundData* data) {
    if (!data) return;

    if (data->pSourceVoice) {
        data->pSourceVoice->Stop();
        data->pSourceVoice->DestroyVoice();
    }
    SafeRelease(&data->pReader);
    if (data->pWfx) {
        CoTaskMemFree(data->pWfx);
    }
    if (data->pBuffer) {
        delete[] data->pBuffer;
    }
    delete data;
}

// 再生
void PlaySound(SoundData* data, bool loop) {
    if (!data || !data->pSourceVoice) return;

    data->loop = loop;
    data->pSourceVoice->Stop();
    data->pSourceVoice->FlushSourceBuffers();

    XAUDIO2_BUFFER buffer = { 0 };
    buffer.AudioBytes = data->bufferSize;
    buffer.pAudioData = data->pBuffer;
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    if (loop) {
        buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
    }

    data->pSourceVoice->SubmitSourceBuffer(&buffer);
    data->pSourceVoice->Start(0);
}

// 停止
void StopSound(SoundData* data) {
    if (!data || !data->pSourceVoice) return;

    data->pSourceVoice->Stop();
    data->pSourceVoice->FlushSourceBuffers();
}
