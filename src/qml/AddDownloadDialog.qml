import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs

Dialog {
    id: root
    title: qsTr("Add download")
    modal: true
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    width: Math.min(parent.width - 32, 480)

    background: Rectangle {
        color: Material.dialogColor
        radius: 12
    }

    property int modeIndex: 0

    function openWithTorrent(torrentUrl) {
        tabBar.currentIndex = 1;
        torrentPathField.text = torrentUrl;
        open();
    }

    function openWithMagnet(magnetUrl) {
        tabBar.currentIndex = 0;
        urlField.text = magnetUrl;
        open();
    }

    function clearFields() {
        urlField.text = "";
        torrentPathField.text = "";
        errorLabel.visible = false;
    }

    Connections {
        target: synoClient
        function onTaskCreated(taskId) {
            addingIndicator.visible = false;
            clearFields();
            root.close();
        }
        function onTaskCreateFailed(errorMessage) {
            addingIndicator.visible = false;
            errorLabel.text = errorMessage;
            errorLabel.visible = true;
        }
        function onDownloadStationConfigReceived(defaultDest) {
            if (destField.text === "")
                destField.text = defaultDest;
        }
    }

    Component.onCompleted: synoClient.getDownloadStationConfig()

    footer: Pane {
        padding: 12

        RowLayout {
            anchors.fill: parent
            spacing: 8

            BusyIndicator {
                id: addingIndicator
                visible: false
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
            }

            Item { Layout.fillWidth: true }

            Button {
                text: qsTr("Cancel")
                onClicked: root.reject()
            }

            Button {
                text: qsTr("Add")
                highlighted: true
                enabled: (tabBar.currentIndex === 0 && urlField.text !== "") ||
                         (tabBar.currentIndex === 1 && torrentPathField.text !== "" && destField.text !== "")
                onClicked: {
                    errorLabel.visible = false;
                    addingIndicator.visible = true;

                    if (tabBar.currentIndex === 0) {
                        synoClient.createTaskUrl(urlField.text, destField.text);
                    } else {
                        synoClient.createTaskTorrent(
                            torrentPathField.text,
                            destField.text
                        );
                    }
                }
            }
        }
    }

    ColumnLayout {
        spacing: 16
        width: parent.width

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            onCurrentIndexChanged: root.modeIndex = currentIndex

            TabButton { text: qsTr("URL / Magnet") }
            TabButton { text: qsTr("Torrent file") }
        }

        StackLayout {
            currentIndex: tabBar.currentIndex
            Layout.fillWidth: true

            ColumnLayout {
                spacing: 12
                width: parent.width

                Label {
                    text: qsTr("Enter a URL, magnet link, or HTTP download link:")
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                }

                TextField {
                    id: urlField
                    placeholderText: qsTr("https://... / magnet:?...")
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                }
            }

            ColumnLayout {
                spacing: 12
                width: parent.width

                Label {
                    text: qsTr("Select a .torrent file:")
                    font.pixelSize: 14
                }

                RowLayout {
                    spacing: 8
                    Layout.fillWidth: true

                    TextField {
                        id: torrentPathField
                        placeholderText: qsTr("No file selected")
                        readOnly: true
                        Layout.fillWidth: true
                    }

                    Button {
                        text: qsTr("Browse")
                        onClicked: torrentFileDialog.open()
                    }
                }

                FileDialog {
                    id: torrentFileDialog
                    nameFilters: [qsTr("Torrent files (*.torrent)")]
                    onAccepted: {
                        torrentPathField.text = selectedFile;
                    }
                }
            }
        }

        TextField {
            id: destField
            placeholderText: qsTr("Destination folder (optional)")
            Layout.fillWidth: true
        }

        Label {
            id: errorLabel
            visible: false
            color: Material.color(Material.Red)
            font.pixelSize: 14
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
