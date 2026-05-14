import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Dialog {
    id: root
    modal: true
    standardButtons: Dialog.Close
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    title: qsTr("Task details")

    background: Rectangle {
        color: Material.dialogColor
        radius: 12
    }

    property string currentTaskId: ""
    property var currentDetail: ({})

    property bool pendingOpen: false

    Connections {
        target: synoClient
        function onTaskDetailReceived(detail) {
            root.currentDetail = detail;
            root.updateView();
            if (pendingOpen) {
                pendingOpen = false;
                open();
            }
        }
    }

    function openFor(taskId) {
        currentTaskId = taskId;
        pendingOpen = true;
        synoClient.getTaskDetail(taskId);
    }

    function updateView() {
        if (!currentDetail || !currentDetail.id) return;
        titleLabel.text = currentDetail.title || "";
        statusLabel.text = currentDetail.status || "";
        progressBar.value = 0;

        var detail = currentDetail.additional ? currentDetail.additional.detail : null;
        if (detail) {
            destLabel.text = qsTr("Destination: %1").arg(detail.destination || "");
            uriLabel.text = detail.uri || "";
        }

        var transfer = currentDetail.additional ? currentDetail.additional.transfer : null;
        var totalSize = parseFloat(currentDetail.size) || 0;
        if (transfer) {
            var downloadedSize = parseFloat(transfer.size_downloaded) || 0;
            sizeLabel.text = qsTr("Size: %1 / %2").arg(
                downloadModel.formatSize(downloadedSize)).arg(downloadModel.formatSize(totalSize));
            if (totalSize > 0)
                progressBar.value = downloadedSize / totalSize;
            speedLabel.text = qsTr("Down: %1  Up: %2").arg(
                downloadModel.formatSpeed(transfer.speed_download || 0)).arg(
                downloadModel.formatSpeed(transfer.speed_upload || 0));
        } else {
            sizeLabel.text = totalSize > 0
                ? qsTr("Size: %1").arg(downloadModel.formatSize(totalSize))
                : "";
            speedLabel.text = "";
        }

        filesModel.clear();
        var files = currentDetail.additional ? currentDetail.additional.file : null;
        if (files) {
            for (var i = 0; i < files.length; i++) {
                filesModel.append({
                    name: files[i].filename || qsTr("Unknown"),
                    size: downloadModel.formatSize(files[i].size || 0)
                });
            }
        }

        trackersLabel.text = "";
        var trackers = currentDetail.additional ? currentDetail.additional.tracker : null;
        if (trackers) {
            var trackerText = "";
            for (i = 0; i < trackers.length; i++) {
                trackerText += trackers[i].url + "\n";
            }
            trackersLabel.text = trackerText;
        }
    }



    ListModel { id: filesModel }

    ScrollView {
        anchors.fill: parent
        clip: true
        padding: 8

        ColumnLayout {
            spacing: 12
            width: parent.width

            Pane {
                Layout.fillWidth: true
                Material.elevation: 1

                ColumnLayout {
                    spacing: 8
                    width: parent.width

                    Label {
                        id: titleLabel
                        font.pixelSize: 16
                        font.weight: Font.Bold
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Label {
                        id: statusLabel
                        font.pixelSize: 14
                        color: Material.accentColor
                    }

                    ProgressBar {
                        id: progressBar
                        from: 0; to: 1
                        Layout.fillWidth: true
                        Layout.preferredHeight: 4
                    }

                    Label { id: sizeLabel; font.pixelSize: 12; color: Material.color(Material.Grey) }
                    Label { id: speedLabel; font.pixelSize: 12; color: Material.color(Material.Grey) }
                    Label { id: destLabel; font.pixelSize: 12; color: Material.color(Material.Grey) }
                    Label { id: uriLabel; font.pixelSize: 12; color: Material.color(Material.Grey); wrapMode: Text.WordWrap; Layout.fillWidth: true }
                }
            }

            Pane {
                Layout.fillWidth: true
                Material.elevation: 1
                visible: filesModel.count > 0

                ColumnLayout {
                    spacing: 8
                    width: parent.width

                    Label {
                        text: qsTr("Files (%1)").arg(filesModel.count)
                        font.pixelSize: 14
                        font.weight: Font.Bold
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.min(200, filesModel.count * 40)
                        model: filesModel
                        interactive: false

                        delegate: RowLayout {
                            width: parent.width
                            spacing: 8

                            Label {
                                text: model.name
                                font.pixelSize: 12
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            Label {
                                text: model.size
                                font.pixelSize: 12
                                color: Material.color(Material.Grey)
                            }
                        }
                    }
                }
            }

            Pane {
                Layout.fillWidth: true
                Material.elevation: 1
                visible: trackersLabel.text !== ""

                ColumnLayout {
                    spacing: 8
                    width: parent.width

                    Label {
                        text: qsTr("Trackers")
                        font.pixelSize: 14
                        font.weight: Font.Bold
                    }

                    Label {
                        id: trackersLabel
                        font.pixelSize: 12
                        color: Material.color(Material.Grey)
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}
