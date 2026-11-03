// SPDX-FileCopyrightText: Copyright 2025 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Copyright 2018 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QWidget>
#include "network/network.h"

class QStandardItemModel;
class HostRoomWindow;
class ClientRoomWindow;
class DirectConnectWindow;
class ClickableLabel;

namespace Core {
class System;
}

class MultiplayerState : public QWidget {
    Q_OBJECT;

public:
    enum class NotificationStatus {
        Uninitialized,
        Disconnected,
        Connected,
        Notification,
    };

    explicit MultiplayerState(QWidget* parent, QStandardItemModel* game_list, QAction* leave_room,
                              QAction* show_room, Core::System& system_);
    ~MultiplayerState();

    /**
     * Close all open multiplayer related dialogs
     */
    void Close();

    void SetNotificationStatus(NotificationStatus state);

    void UpdateNotificationStatus();

    ClickableLabel* GetStatusText() const {
        return status_text;
    }

    ClickableLabel* GetStatusIcon() const {
        return status_icon;
    }

    void retranslateUi();

    /**
     * Updates the multiplayer dialogs with a new game list model.
     * This model should be the original model of the game list.
     */
    void UpdateGameList(QStandardItemModel* game_list);

public slots:
    void OnNetworkStateChanged(const Network::RoomMember::State& state);
    void OnNetworkError(const Network::RoomMember::Error& error);
    void OnCreateRoom();
    bool OnCloseRoom();
    void OnOpenNetworkRoom();
    void OnDirectConnectToRoom();
    void OnSaveConfig();
    void UpdateThemedIcons();
    void ShowNotification();
    void HideNotification();

signals:
    void NetworkStateChanged(const Network::RoomMember::State&);
    void NetworkError(const Network::RoomMember::Error&);
    void SaveConfig();

private:
    HostRoomWindow* host_room = nullptr;
    ClientRoomWindow* client_room = nullptr;
    DirectConnectWindow* direct_connect = nullptr;
    ClickableLabel* status_icon = nullptr;
    ClickableLabel* status_text = nullptr;
    QStandardItemModel* game_list_model = nullptr;
    QAction* leave_room;
    QAction* show_room;
    Network::RoomMember::State current_state = Network::RoomMember::State::Uninitialized;
    NotificationStatus notification_status = NotificationStatus::Uninitialized;
    bool has_mod_perms = false;
    Network::RoomMember::CallbackHandle<Network::RoomMember::State> state_callback_handle;
    Network::RoomMember::CallbackHandle<Network::RoomMember::Error> error_callback_handle;

    bool show_notification = false;
    Core::System& system;
};

Q_DECLARE_METATYPE(WebService::WebResult);
