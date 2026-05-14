import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Fusion
import QtQuick.Layouts

ApplicationWindow {
    id: app
    visible: true
    width: 900
    height: 650
    minimumWidth: 600
    minimumHeight: 400
    title: qsTr("DSDesk — Synology Download Station")

    Material.theme: Material.Light
    Material.accent: Material.Blue

    property string droppedTorrentPath
    property string pendingTorrent: ""

    Component { id: loginViewComp; LoginView {} }
    Component { id: downloadListViewComp; DownloadListView {} }

    Connections {
        target: ipc
        function onTorrentReceived(torrent) {
            if (stack.currentItem && stack.currentItem.openAddDialogWithTorrent) {
                if (torrent.startsWith("file:"))
                    stack.currentItem.openAddDialogWithTorrent(torrent);
                else
                    stack.currentItem.openAddDialogWithMagnet(torrent);
            } else {
                pendingTorrent = torrent;
            }
        }
    }

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: loginViewComp

        function navigateToDownloads() {
            stack.replace(downloadListViewComp);
            if (pendingTorrent !== "") {
                var torrent = pendingTorrent;
                pendingTorrent = "";
                Qt.callLater(function() {
                    if (stack.currentItem && stack.currentItem.openAddDialogWithTorrent) {
                        if (torrent.startsWith("file:"))
                            stack.currentItem.openAddDialogWithTorrent(torrent);
                        else
                            stack.currentItem.openAddDialogWithMagnet(torrent);
                    }
                });
            }
        }
        function navigateToLogin() {
            stack.replace(loginViewComp);
        }
    }

    DropArea {
        anchors.fill: parent
        keys: ["text/uri-list"]

        Rectangle {
            anchors.fill: parent
            color: Material.accentColor
            opacity: 0.3
            visible: parent.containsDrag
        }

        onEntered: function(drag) {
            drag.accept(Qt.LinkAction);
        }

        onDropped: function(drop) {
            if (drop.hasUrls) {
                for (var i = 0; i < drop.urls.length; i++) {
                    var url = drop.urls[i].toString();
                    if (url.endsWith(".torrent")) {
                        droppedTorrentPath = url;
                        handleDroppedTorrent(url);
                        return;
                    }
                }
            }
        }
    }

    function handleDroppedTorrent(path) {
        var currentItem = stack.currentItem;
        if (currentItem && currentItem.openAddDialogWithTorrent) {
            currentItem.openAddDialogWithTorrent(path);
        }
    }

    property alias stackView: stack
    property bool autoConnectDone: false
}
