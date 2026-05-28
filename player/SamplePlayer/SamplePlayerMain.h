//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

// #define ENABLE_CUSTOM_DATA_CHANNEL_SAMPLE
// #define ENABLE_USER_COORDINATE_SYSTEM_SAMPLE

#include "../common/Content/ErrorHelper.h"
#include "../common/Content/StatusDisplay.h"
#include "../common/IpAddressUpdater.h"
#include "../common/PlayerFrameStatisticsHelper.h"

#include <winrt/Microsoft.Holographic.AppRemoting.h>

#include <chrono>

#include <DeviceResourcesD3D11Holographic.h>
#include <SimpleCubeRenderer.h>

class SamplePlayerMain : public winrt::implements<
                             SamplePlayerMain,
                             winrt::Windows::ApplicationModel::Core::IFrameworkViewSource,
                             winrt::Windows::ApplicationModel::Core::IFrameworkView>,
                         public DXHelper::IDeviceNotify
{
public:
    SamplePlayerMain();
    ~SamplePlayerMain();

    void ConnectOrListen();
    winrt::fire_and_forget ConnectOrListenAfter(std::chrono::system_clock::duration time);

    winrt::Windows::Graphics::Holographic::HolographicFrame Update(
        float deltaTimeInSeconds,
        const winrt::Windows::Graphics::Holographic::HolographicFrame& prevHolographicFrame);

    void Render(const winrt::Windows::Graphics::Holographic::HolographicFrame& holographicFrame);

public:
    // IFrameworkViewSource methods
    winrt::Windows::ApplicationModel::Core::IFrameworkView CreateView();

    // IFrameworkView methods
    void Initialize(const winrt::Windows::ApplicationModel::Core::CoreApplicationView& applicationView);
    void SetWindow(const winrt::Windows::UI::Core::CoreWindow& window);
    void Load(const winrt::hstring& entryPoint);
    void Run();
    void Uninitialize();

    // IDeviceNotify methods
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

private:
    struct PlayerOptions
    {
        // ── ALTERAÇÃO: modo reverso como padrão ──────────────────────────────
        // ANTES: m_hostname = L"0.0.0.0", m_listen = true
        // DEPOIS: hostname vazio (carregado do storage), listen = false
        winrt::hstring m_hostname = L"";
        uint16_t m_port = 0;
        bool m_listen = false;          // ALTERADO: false = HoloLens conecta ao PC
        bool m_showStatistics = false;
        bool m_ipv6 = false;
    };

private:
    void LoadLogoImage();
    PlayerOptions ParseActivationArgs(
        const winrt::Windows::ApplicationModel::Activation::IActivatedEventArgs& activationArgs);
    void UpdateStatusDisplay();

    // ── ADICIONADO: Persistência do IP do PC entre sessões ───────────────────
    // Salva e carrega o IP do PC via ApplicationData.LocalSettings
    static winrt::hstring LoadSavedPcIp();
    static void SavePcIp(const winrt::hstring& ip);

#ifdef ENABLE_CUSTOM_DATA_CHANNEL_SAMPLE
    void OnCustomDataChannelDataReceived(winrt::array_view<const uint8_t> dataView);
    void OnCustomDataChannelClosed();
#endif

    // PlayerContext event handlers
    void OnConnected();
    void OnDisconnected(winrt::Microsoft::Holographic::AppRemoting::ConnectionFailureReason reason);
    void OnRequestRenderTargetSize(
        winrt::Windows::Foundation::Size requestedSize,
        winrt::Windows::Foundation::Size providedSize);

    // SpatialLocator event handlers
    void OnLocatabilityChanged(
        const winrt::Windows::Perception::Spatial::SpatialLocator& sender,
        const winrt::Windows::Foundation::IInspectable& args);

    // Application lifecycle event handlers
    void OnViewActivated(
        const winrt::Windows::ApplicationModel::Core::CoreApplicationView& sender,
        const winrt::Windows::ApplicationModel::Activation::IActivatedEventArgs& args);
    void OnSuspending(
        const winrt::Windows::Foundation::IInspectable& sender,
        const winrt::Windows::ApplicationModel::SuspendingEventArgs& args);

    // CoreWindow event handlers
    void OnVisibilityChanged(
        const winrt::Windows::UI::Core::CoreWindow& sender,
        const winrt::Windows::UI::Core::VisibilityChangedEventArgs& args);
    void OnWindowClosed(
        const winrt::Windows::UI::Core::CoreWindow& sender,
        const winrt::Windows::UI::Core::CoreWindowEventArgs& args);

private:
    std::shared_ptr<DXHelper::DeviceResourcesD3D11Holographic> m_deviceResources;

    winrt::Windows::Perception::Spatial::SpatialLocator m_spatialLocator = nullptr;
    winrt::Windows::Perception::Spatial::SpatialLocatorAttachedFrameOfReference m_attachedFrameOfReference = nullptr;

    winrt::Microsoft::Holographic::AppRemoting::PlayerContext m_playerContext = nullptr;

    PlayerOptions m_playerOptions = {};

    std::unique_ptr<StatusDisplay> m_statusDisplay;

#ifdef ENABLE_USER_COORDINATE_SYSTEM_SAMPLE
    std::unique_ptr<SimpleCubeRenderer> m_simpleCubeRenderer;
    winrt::Windows::Perception::Spatial::SpatialStationaryFrameOfReference m_userSpatialFrameOfReference = nullptr;
#endif

    winrt::com_ptr<ID3D11Resource> m_logoImage;

    // Mantido para compatibilidade com modo listen (quando ativado via args)
    winrt::hstring m_deviceIp = L"127.0.0.1";
    std::shared_ptr<IIpAddressUpdater> m_ipAddressUpdater;

    PlayerFrameStatisticsHelper m_statisticsHelper;
    ErrorHelper m_errorHelper;

#ifdef ENABLE_CUSTOM_DATA_CHANNEL_SAMPLE
    std::mutex m_customDataChannelLock;
    winrt::Microsoft::Holographic::AppRemoting::IDataChannel2 m_customDataChannel = nullptr;
    winrt::Microsoft::Holographic::AppRemoting::IDataChannel2::OnDataReceived_revoker m_customChannelDataReceivedEventRevoker;
    winrt::Microsoft::Holographic::AppRemoting::IDataChannel2::OnClosed_revoker m_customChannelClosedEventRevoker;
#endif

    bool m_trackingLost = false;
    bool m_windowClosed = false;
    bool m_windowVisible = false;
    bool m_needRenderTargetSizeChange = false;
    winrt::Windows::Foundation::Size m_newRenderTargetSize;
    std::mutex m_renderTargetSizeChangeMutex;
    bool m_canCommitDirect3D11DepthBuffer = false;

    winrt::Windows::Perception::Spatial::SpatialLocator::LocatabilityChanged_revoker m_locatabilityChangedRevoker;
    winrt::Windows::ApplicationModel::Core::CoreApplication::Suspending_revoker m_suspendingEventRevoker;
    winrt::Windows::ApplicationModel::Core::CoreApplicationView::Activated_revoker m_viewActivatedRevoker;
    winrt::Windows::UI::Core::CoreWindow::Closed_revoker m_windowClosedEventRevoker;
    winrt::Windows::UI::Core::CoreWindow::VisibilityChanged_revoker m_visibilityChangedEventRevoker;

    bool m_failedToCreatePlayerContext = false;
    bool m_shownFeedbackToUser = false;
    bool m_firstRemoteFrameWasBlitted = false;
};
