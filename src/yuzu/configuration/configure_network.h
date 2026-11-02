// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <QWidget>
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
};
