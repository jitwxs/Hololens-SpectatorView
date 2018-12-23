// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#if USE_DECKLINK || USE_DECKLINK_SHUTTLE

#include "IFrameProvider.h"
#include "DeckLinkDevice.h"

class DeckLinkManager : public IFrameProvider
{
public:
    DeckLinkManager();
    ~DeckLinkManager();

    // Inherited via IFrameProvider
    HRESULT Initialize(ID3D11ShaderResourceView* srv);
    virtual LONGLONG GetTimestamp(int frame) override;
    virtual LONGLONG GetDurationHNS() override;

    virtual bool IsEnabled() override;
    virtual void Update(int compositeFrameIndex) override;
    virtual void Dispose() override;

    virtual bool OutputYUV() override;

    virtual void SetOutputTexture(ID3D11Texture2D* outputTexture) override;

    virtual int GetCaptureFrameIndex() override;

private:
    DeckLinkDeviceDiscovery* deckLinkDiscovery = nullptr;
    DeckLinkDevice* deckLinkDevice = nullptr;
    IDeckLink* deckLink = nullptr;
    bool supportsBlackMagic = true;
};
#endif

