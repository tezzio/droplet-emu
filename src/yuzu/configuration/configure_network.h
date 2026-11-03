// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <QString>
#include <QWidget>
#include "core/hle/service/sockets/online_friends.h"
#include "core/hle/service/sockets/online_profile.h"

namespace Ui {
class ConfigureNetwork;
}

class ConfigureNetwork : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureNetwork(const Core::System& system_, QWidget* parent = nullptr);
    ~ConfigureNetwork() override;

    void ApplyConfiguration();

private:
    void changeEvent(QEvent*) override;
    void RetranslateUI();
    void SetConfiguration();
    void UpdateOnlineFields();
    void ChooseServerCa();
    void ShowServerCaHint();

    // Friend requests through the server's friends API, off this thread
    struct FriendCallResult {
        Service::Sockets::OnlineRedirect::FriendLists lists;
        std::vector<std::string> done; // codes the server took the action for
        QString errors;                // why it refused the others
    };
    void RefreshFriends();
    void GetServerCa();
    void RunFriendCalls(std::vector<std::pair<std::string, std::string>> calls,
                        std::function<void(const FriendCallResult&)> done);
    void UseServerLists(const Service::Sockets::OnlineRedirect::FriendLists& lists);
    void AnswerRequests(const std::string& action);
    void RemoveFriends();
    void UpdateFriendButtons();
    std::string ServerCaPath() const;

    // The friend code and friends shown belong to the server in the server field
    void LoadProfileView();
    void ShowProfile();
    void EditProfile(const std::function<void(Service::Sockets::OnlineRedirect::Profile&)>& edit);
    void AddFriends();
    void ChangeCode();
    void BackUpProfile();
    void RestoreProfile();

    std::unique_ptr<Ui::ConfigureNetwork> ui;

    const Core::System& system;

    std::string profile_server;
    std::optional<Service::Sockets::OnlineRedirect::Profile> profile;
    // The server's friends and requests while it answers; without them the list shows the codes
    // this emulator added, which its next sign-in sends
    std::optional<Service::Sockets::OnlineRedirect::FriendLists> server_lists;
    int friend_calls = 0; // the latest call; an earlier one's answer is dropped
    bool friend_busy = false;
};
