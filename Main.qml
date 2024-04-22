import QtQuick
import VideoPlayer
import QtQuick.Controls
import Qt.labs.platform

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("VPlayer")

    function formatTime(seconds) {
        const hours = Math.floor(seconds / 3600)
        const minutes = Math.floor((seconds % 3600) / 60)
        const remainingSeconds = seconds % 60

        const formattedHours = String(hours).padStart(2, '0')
        const formattedMinutes = String(minutes).padStart(2, '0')
        const formattedSeconds = String(remainingSeconds).padStart(2, '0')

        return `${formattedHours}:${formattedMinutes}:${formattedSeconds}`
    }

    VideoPlayer {
        id: videoPlayer
        height: parent.height
        width: parent.width
        onPlayTime: {
            playTime.text = formatTime(Math.floor(time / 1000000))
        }
    }

    Button {
        id: open
        text: '打开'
        onClicked: {
            fileDialog.open()
        }
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

    Text {
        id: playTime
        text: '00:00:00'
        color: '#FFFFFF'
        x: 150
    }

    Text {
        id: totleTime
        text: '00:00:00'
        color: '#FFFFFF'
        x: 250
    }

    FileDialog {
        id: fileDialog
        title: "打开视频"
        nameFilters: ["*.mp4", "*.avi", "*.mkv", "*"]
        onAccepted: {
            console.log("Selected file: " + fileDialog.file)
            if (videoPlayer.loadVideo(fileDialog.file, true)) {
                totleTime.text = formatTime(videoPlayer.getVideoTotleTime())
            }
        }
    }
}
