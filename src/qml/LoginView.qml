import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Page {
    id: root

    Component.onCompleted: {
        hostField.text = synoSettings.host;
        portField.text = synoSettings.port;
        usernameField.text = synoSettings.username;
        sslSwitch.checked = synoSettings.useSsl;
        trustCertCheck.checked = synoSettings.trustCert;

        if (!ApplicationWindow.window.autoConnectDone && synoSettings.host !== "") {
            var savedPass = synoSettings.loadPassword();
            if (savedPass !== "") {
                passwordField.text = savedPass;
                ApplicationWindow.window.autoConnectDone = true;
                doLogin();
            }
        }
    }

    function navigateToDownloads() {
        var sv = ApplicationWindow.window.stackView;
        if (sv) sv.navigateToDownloads();
    }

    Connections {
        target: synoClient
        function onLoginSuccess() {
            loginBtn.enabled = true;
            loginBtn.text = qsTr("Connected");
            synoSettings.saveCredentials(passwordField.text);
            navigateToDownloads();
        }
        function onLoginFailed(errorMessage) {
            loginBtn.enabled = true;
            loginBtn.text = qsTr("Connect");
            errorLabel.text = errorMessage;
            errorLabel.visible = true;
        }
        function onNetworkError(errorMessage) {
            if (!synoClient.connected) {
                loginBtn.enabled = true;
                loginBtn.text = qsTr("Connect");
                errorLabel.text = errorMessage;
                errorLabel.visible = true;
            }
        }
    }

    background: Rectangle {
        color: Material.backgroundColor
    }

    Flickable {
        anchors.fill: parent
        contentHeight: formColumn.implicitHeight + 32
        boundsBehavior: Flickable.OvershootBounds

        ColumnLayout {
            id: formColumn
            spacing: 16
            width: Math.min(parent.width - 32, 420)
            anchors.centerIn: parent

            Item { height: 8; width: 1 }

            Label {
                text: qsTr("DSDesk")
                font.pixelSize: 20
                font.weight: Font.Bold
                color: Material.accentColor
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: qsTr("Synology Download Station Client")
                font.pixelSize: 14
                color: Material.color(Material.Grey)
                Layout.alignment: Qt.AlignHCenter
                bottomPadding: 8
            }

            Pane {
                Layout.fillWidth: true
                Material.elevation: 2

                ColumnLayout {
                    spacing: 12
                    width: parent.width

                    Label {
                        text: qsTr("Connection")
                        font.weight: Font.Bold
                        font.pixelSize: 14
                        color: Material.accentColor
                    }

                    GridLayout {
                        columns: 2
                        columnSpacing: 12
                        rowSpacing: 12
                        Layout.fillWidth: true

                        Label { text: qsTr("Host"); font.pixelSize: 14 }
                        TextField {
                            id: hostField
                            placeholderText: "192.168.1.100"
                            Layout.fillWidth: true
                            onTextChanged: synoSettings.host = text
                        }

                        Label { text: qsTr("Port"); font.pixelSize: 14 }
                        TextField {
                            id: portField
                            text: sslSwitch.checked ? "5001" : "5000"
                            Layout.preferredWidth: 100
                            validator: IntValidator { bottom: 1; top: 65535 }
                            onTextChanged: synoSettings.port = parseInt(text)
                        }

                        Label { text: qsTr("Username"); font.pixelSize: 14 }
                        TextField {
                            id: usernameField
                            placeholderText: qsTr("Username")
                            Layout.fillWidth: true
                            onTextChanged: synoSettings.username = text
                        }

                        Label { text: qsTr("Password"); font.pixelSize: 14 }
                        TextField {
                            id: passwordField
                            placeholderText: qsTr("Password")
                            echoMode: TextInput.Password
                            Layout.fillWidth: true
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label { text: qsTr("Use HTTPS"); font.pixelSize: 14 }
                        Switch {
                            id: sslSwitch
                            onCheckedChanged: {
                                synoSettings.useSsl = checked;
                                if (checked && portField.text === "5000")
                                    portField.text = "5001";
                                else if (!checked && portField.text === "5001")
                                    portField.text = "5000";
                            }
                        }

                        Item { Layout.fillWidth: true }

                        CheckBox {
                            id: trustCertCheck
                            text: qsTr("Trust this certificate")
                            font.pixelSize: 14
                            visible: sslSwitch.checked
                            checked: synoSettings.trustCert
                            onToggled: synoSettings.trustCert = checked
                        }
                    }
                }
            }

            Label {
                id: errorLabel
                visible: false
                color: Material.color(Material.Red)
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.topMargin: 8
            }

            Button {
                id: loginBtn
                text: qsTr("Connect")
                enabled: hostField.text !== "" && usernameField.text !== "" && passwordField.text !== ""
                highlighted: true
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                font.pixelSize: 16
                font.weight: Font.Medium

                onClicked: doLogin()
            }

            Item { height: 8; width: 1 }
        }
    }

    function doLogin() {
        errorLabel.visible = false;
        loginBtn.enabled = false;
        loginBtn.text = qsTr("Connecting...");
        synoClient.login(
            hostField.text,
            parseInt(portField.text),
            usernameField.text,
            passwordField.text,
            sslSwitch.checked,
            trustCertCheck.checked
        );
    }
}
