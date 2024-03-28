import QtQuick
import VideoPlayer
import QtQuick.Controls
import Qt.labs.platform

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    VideoPlayer {
        id: videoPlayer
    }

    Button {
        id: button
        text: '打开'
        onClicked: fileDialog.open()
    }

    FileDialog {
        id: fileDialog
        title: "打开视频"
        nameFilters: ["*.mp4", "*.avi", "*.mkv", "*"]
        onAccepted: {
            console.log("Selected file: " + fileDialog.file)
            videoPlayer.loadVideo(fileDialog.file)
        }
    }
}
