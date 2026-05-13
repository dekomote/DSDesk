import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Page {
    id: root

    function openAddDialogWithTorrent(path) {
        addDialog.openWithTorrent(path);
    }

    function openAddDialogWithMagnet(url) {
        addDialog.openWithMagnet(url);
    }

    header: ToolBar {
        Material.elevation: 2

        RowLayout {
            spacing: 8
            width: parent.width

            ToolButton {
                text: "☰"
                onClicked: drawer.open()
            }

            Label {
                text: qsTr("Downloads")
                font.pixelSize: 16
                font.weight: Font.Bold
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("%1 tasks").arg(downloadModel.count)
                font.pixelSize: 12
                color: Material.color(Material.Grey)
            }

            ToolButton {
                text: "↻"
                onClicked: downloadModel.refresh()
            }

            Button {
                text: qsTr("Disconnect")
                flat: true
                onClicked: {
                    synoSettings.clearCredentials();
                    synoClient.logout();
                    ApplicationWindow.window.stackView.navigateToLogin();
                }
            }
        }
    }

    Drawer {
        id: drawer
        width: Math.min(parent.width * 0.7, 300)
        height: parent.height

        Pane {
            anchors.fill: parent
            padding: 16

            ColumnLayout {
                spacing: 8
                anchors.fill: parent

                Label {
                    text: qsTr("DSDesk")
                    font.pixelSize: 20
                    font.weight: Font.Bold
                    color: Material.accentColor
                }

                Rectangle {
                    height: 1
                    color: Material.color(Material.Grey, Material.Shade700)
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("Connected to:")
                    font.pixelSize: 12
                    color: Material.color(Material.Grey)
                }
                Label {
                    text: "%1:%2".arg(synoSettings.host).arg(synoSettings.port)
                    font.pixelSize: 14
                }
                Label {
                    text: synoSettings.username
                    font.pixelSize: 14
                    color: Material.color(Material.Grey)
                }

                Item { Layout.fillHeight: true }

                Button {
                    text: qsTr("Logout")
                    flat: true
                    Layout.fillWidth: true
                    onClicked: {
                        drawer.close();
                        synoSettings.clearCredentials();
                        synoClient.logout();
                        ApplicationWindow.window.stackView.navigateToLogin();
                    }
                }
            }
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        anchors.topMargin: 8
        anchors.bottomMargin: 72
        model: downloadModel
        spacing: 8
        clip: true
        boundsBehavior: Flickable.OvershootBounds

        delegate: DownloadItemDelegate {
            width: listView.width - 16
            anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined

            onDeleteClicked: function(taskId, title) { deleteConfirm.openFor(taskId, title); }
            onDetailClicked: function(taskId) { detailSheet.openFor(taskId); }
        }

        ScrollBar.vertical: ScrollBar {}

        Label {
            anchors.centerIn: parent
            text: qsTr("No downloads yet")
            font.pixelSize: 14
            color: Material.color(Material.Grey)
            visible: downloadModel.count === 0
        }
    }

    Button {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 16
        text: "+"
        highlighted: true
        Material.elevation: 6
        width: 56
        height: 56
        font.pixelSize: 24
        onClicked: addDialog.open()
    }

    AddDownloadDialog {
        id: addDialog
    }

    TaskDetailsSheet {
        id: detailSheet
    }

    Dialog {
        id: deleteConfirm
        title: qsTr("Delete task")
        standardButtons: Dialog.Yes | Dialog.No
        modal: true
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        property string taskId: ""
        property string taskTitle: ""

        background: Rectangle {
            color: Material.dialogColor
            radius: 12
        }

        function openFor(id, title) {
            taskId = id;
            taskTitle = title;
            open();
        }

        Label {
            text: qsTr("Are you sure you want to delete \"%1\"?").arg(deleteConfirm.taskTitle)
            font.pixelSize: 14
            wrapMode: Text.WordWrap
            width: parent.width
        }

        onAccepted: synoClient.deleteTask(deleteConfirm.taskId)
    }
}
