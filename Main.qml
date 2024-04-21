import QtQuick
import VideoPlayer
import QtQuick.Controls
import Qt.labs.platform

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("VPlayer")

    VideoPlayer {
        id: videoPlayer
        height: parent.height
        width: parent.width
    }

    Button {
        id: open
        text: '打开'
        onClicked: fileDialog.open()
    }

    Button {
        id: play
        text: '播放'
        x: 50
        onClicked: {
            videoPlayer.play()
        }
    }

    Button {
        id: stop
        text: '暂停'
        x: 100
        onClicked: {
            videoPlayer.stop()
        }
    }

    FileDialog {
        id: fileDialog
        title: "打开视频"
        nameFilters: ["*.mp4", "*.avi", "*.mkv", "*"]
        onAccepted: {
            console.log("Selected file: " + fileDialog.file)
            videoPlayer.loadVideo(fileDialog.file, true)
        }
    }
}
