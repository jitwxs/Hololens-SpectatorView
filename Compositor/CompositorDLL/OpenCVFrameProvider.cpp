// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "stdafx.h"
#include "OpenCVFrameProvider.h"

#if USE_OPENCV

OpenCVFrameProvider::OpenCVFrameProvider()
{
    QueryPerformanceFrequency(&freq);
    rgbaFrame = cv::Mat(FRAME_HEIGHT, FRAME_WIDTH, CV_8UC4);

    for (int i = 0; i < 4; i++)
    {
        rgbaConversion[i * 2] = i;
        rgbaConversion[(i * 2) + 1] = i;
    }

    for (int i = 0; i < MAX_NUM_CACHED_BUFFERS; i++)
    {
        bufferCache[i].buffer = new BYTE[FRAME_BUFSIZE];
        bufferCache[i].timeStamp = 0;
    }

    captureFrameIndex = 0;
}

OpenCVFrameProvider::~OpenCVFrameProvider()
{
    for (int i = 0; i < MAX_NUM_CACHED_BUFFERS; i++)
    {
        delete[] bufferCache[i].buffer;
    }
}

HRESULT OpenCVFrameProvider::Initialize(ID3D11ShaderResourceView* srv)
{
    if (IsEnabled())
    {
        return S_OK;
    }

    _colorSRV = srv;
    if (_colorSRV != nullptr)
    {
        _colorSRV->GetDevice(&_device);
    }

    HRESULT hr = E_PENDING;
    videoCapture = new cv::VideoCapture(CAMERA_ID);

    for (int i = 0; i < MAX_NUM_CACHED_BUFFERS; i++)
    {
        ZeroMemory(bufferCache[i].buffer, FRAME_BUFSIZE);
    }

    captureFrameIndex = 0;

    if (videoCapture->open(CAMERA_ID))
    {
        // Attempt to update camera resolution to desired resolution.
        // This must be called after opening.
        // Note: This may fail, and your capture will resume at the camera's native resolution.
        // In this case, the Update loop will print an error with the expected frame resolution.
        videoCapture->set(cv::CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
        videoCapture->set(cv::CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);

        if (IsEnabled())
        {
            hr = S_OK;
        }
    }

    return hr;
}

bool OpenCVFrameProvider::IsEnabled()
{
    if (videoCapture != nullptr)
    {
        return videoCapture->isOpened();
    }

    return false;
}

void OpenCVFrameProvider::Update(int compositeFrameIndex)
{
    if (!IsEnabled() ||
        _colorSRV == nullptr ||
        _device == nullptr)
    {
        return;
    }

    concurrency::create_task([=]
    {
        if (videoCapture->grab())
        {
            LARGE_INTEGER time;
            QueryPerformanceCounter(&time);

            if (videoCapture->retrieve(frame))
            {
                latestTimeStamp = time.QuadPart;

                double width = videoCapture->get(cv::CAP_PROP_FRAME_WIDTH);
                double height = videoCapture->get(cv::CAP_PROP_FRAME_HEIGHT);

                if (width != FRAME_WIDTH)
                {
                    OutputDebugString(L"ERROR: captured width does not equal FRAME_WIDTH.  Expecting: ");
                    OutputDebugString(std::to_wstring(width).c_str());
                    OutputDebugString(L"\n");
                }

                if (height != FRAME_HEIGHT)
                {
                    OutputDebugString(L"ERROR: captured height does not equal FRAME_HEIGHT.  Expecting: ");
                    OutputDebugString(std::to_wstring(height).c_str());
                    OutputDebugString(L"\n");
                }

                // Convert from rgb to rgba
                mixChannels(&frame, 2, &rgbaFrame, 1, rgbaConversion, 4);

                captureFrameIndex++;
                BYTE* buffer = bufferCache[captureFrameIndex % MAX_NUM_CACHED_BUFFERS].buffer;
                memcpy(buffer, rgbaFrame.data, FRAME_BUFSIZE);
                bufferCache[captureFrameIndex % MAX_NUM_CACHED_BUFFERS].timeStamp = (latestTimeStamp * S2HNS) / freq.QuadPart;
            }
        }
    });

    const BufferCache& buffer = bufferCache[compositeFrameIndex % MAX_NUM_CACHED_BUFFERS];
    if (buffer.buffer != nullptr)
    {
        DirectXHelper::UpdateSRV(_device, _colorSRV, buffer.buffer, FRAME_WIDTH * FRAME_BPP);
    }
}

void OpenCVFrameProvider::Dispose()
{
    if (videoCapture != nullptr)
    {
        videoCapture->release();
        videoCapture = nullptr;
    }

    captureFrameIndex = 0;
}
#endif

