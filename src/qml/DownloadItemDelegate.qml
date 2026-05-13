import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Pane {
    id: root

    required property var model
    required property int index

    signal deleteClicked(string taskId, string title)
    signal detailClicked(string taskId)

    function doPauseResume() {
        if (root.model.status === "paused")
            synoClient.resumeTask(root.model.taskId);
        else
            synoClient.pauseTask(root.model.taskId);
    }

    function doDelete() {
        root.deleteClicked(root.model.taskId, root.model.title);
    }

    function doDetail() {
        root.detailClicked(root.model.taskId);
    }

    Material.elevation: mouseArea.containsMouse ? 4 : 1
    padding: 12
    implicitHeight: contentColumn.implicitHeight + topPadding + bottomPadding

    function statusColor(status) {
        switch (status) {
        case "downloading": return Material.color(Material.Blue);
        case "paused": return Material.color(Material.Amber);
        case "finishing": return Material.color(Material.Green);
        case "seeding": return Material.color(Material.Teal);
        case "waiting": return Material.color(Material.Grey);
        case "error": return Material.color(Material.Red);
        default: return Material.color(Material.Grey);
        }
    }

    function statusIcon(status) {
        switch (status) {
        case "downloading": return "↓";
        case "paused": return "⏸";
        case "finishing": return "✓";
        case "seeding": return "↑";
        case "waiting": return "⏳";
        case "error": return "✗";
        default: return "●";
        }
    }

    function statusLabel(status) {
        switch (status) {
        case "downloading": return qsTr("Downloading");
        case "paused": return qsTr("Paused");
        case "finishing": return qsTr("Finishing");
        case "seeding": return qsTr("Seeding");
        case "waiting": return qsTr("Waiting");
        case "error": return qsTr("Error");
        default: return status;
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.doDetail()
    }

    ColumnLayout {
        id: contentColumn
        anchors { left: parent.left; right: parent.right; top: parent.top }
        spacing: 8

        RowLayout {
            spacing: 12

            Rectangle {
                implicitWidth: 36
                implicitHeight: 36
                radius: 18
                color: statusColor(root.model.status)
                Layout.topMargin: -10

                Label {
                    anchors.centerIn: parent
                    text: statusIcon(root.model.status)
                    font.pixelSize: 16
                    color: "white"
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: root.model.title
                        font.pixelSize: 14
                        font.weight: Font.Medium
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        maximumLineCount: 1
                    }

                    Label {
                        text: statusLabel(root.model.status)
                        font.pixelSize: 12
                        color: statusColor(root.model.status)
                        font.weight: Font.Bold
                    }
                }

                ProgressBar {
                    value: root.model.progress
                    Layout.fillWidth: true
                    Layout.preferredHeight: 12
                    from: 0
                    to: 1
                }

                RowLayout {
                    spacing: 8
                    Layout.fillWidth: true

                    Label {
                        text: qsTr("%1 of %2").arg(root.model.sizeDownloadedFormatted).arg(root.model.sizeTotalFormatted)
                        font.pixelSize: 12
                        color: Material.color(Material.Grey)
                    }

                    Label {
                        text: root.model.speedDownFormatted
                        font.pixelSize: 12
                        color: Material.color(Material.Blue)
                        visible: root.model.status === "downloading"
                    }

                    Item { Layout.fillWidth: true }

                    Label {
                        text: qsTr("ETA: %1").arg(root.model.etaFormatted)
                        font.pixelSize: 12
                        color: Material.color(Material.Grey)
                        visible: root.model.status === "downloading"
                    }

                    ToolButton {
                        implicitWidth: 36
                        implicitHeight: 36
                        icon.source: root.model.status === "paused" ? "qrc:/icons/play.svg" : "qrc:/icons/pause.svg"
                        icon.width: 20
                        icon.height: 20
                        visible: root.model.status === "downloading" || root.model.status === "paused"
                        enabled: root.model.status === "downloading" || root.model.status === "paused"
                        onClicked: root.doPauseResume()
                    }

                    ToolButton {
                        implicitWidth: 36
                        implicitHeight: 36
                        icon.source: "qrc:/icons/delete.svg"
                        icon.width: 20
                        icon.height: 20
                        Material.foreground: Material.color(Material.Red)
                        onClicked: root.doDelete()
                    }
                }
            }
        }
    }
}
